/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "rlottie_config.h"
#include "lottieitem.h"
#include "lottiemodel.h"
#include "rlottie.h"
#ifndef USING_MINI_RLOTTIE
#include <fstream>
#endif
using namespace rlottie;
using namespace rlottie::internal;
#ifndef USING_MINI_RLOTTIE
RLOTTIE_API void rlottie::configureModelCacheSize(size_t cacheSize)
{
    internal::model::configureModelCacheSize(cacheSize);
}
#endif

#ifdef MT_CPPLIB
struct RenderTask {
    RenderTask() { receiver = sender.get_future(); }
    std::promise<Surface> sender;
    std::future<Surface>  receiver;
    AnimationImpl *       playerImpl{nullptr};
    size_t                frameNo{0};
    Surface               surface;
    bool                  keepAspectRatio{true};
};
using SharedRenderTask = std::shared_ptr<RenderTask>;
#endif

class AnimationImpl {
public:
    void    init(std::shared_ptr<model::Composition> composition);
    bool    update(size_t frameNo, const VSize &size, bool keepAspectRatio);
    bool    updatePartial(size_t frameNo, const VSize &size, uint32_t offset);
    VSize   size() const { return mModel->size(); }
    double  duration() const { return mModel->duration(); }
    double  frameRate() const { return mModel->frameRate(); }
    size_t  totalFrame() const { return mModel->totalFrame(); }
#ifndef USING_MINI_RLOTTIE
    size_t  frameAtPos(double pos) const { return mModel->frameAtPos(pos); }
#endif
    bool    checkRender();
    Surface render(size_t frameNo, const Surface &surface,
                   bool keepAspectRatio);
    Surface renderPartial(size_t frameNo, const Surface &surface);
#ifndef USING_MINI_RLOTTIE
#ifdef MT_CPPLIB    
    std::future<Surface> renderAsync(size_t frameNo, Surface &&surface,
                                     bool keepAspectRatio);
#endif
    const LOTLayerNode * renderTree(size_t frameNo, const VSize &size);
#endif
    const LayerInfoList &layerInfoList() const
    {
        if (mLayerList.empty()) {
            mLayerList = mModel->layerInfoList();
        }
        return mLayerList;
    }
    const MarkerList &markers() const { return mModel->markers(); }
    void              setValue(const std::string &keypath, LOTVariant &&value);
    void              removeFilter(const std::string &keypath, Property prop);

private:
    mutable LayerInfoList                  mLayerList;
    model::Composition *                   mModel;
#ifdef MT_CPPLIB    
    SharedRenderTask                       mTask;
    std::atomic<bool>                      mRenderInProgress;
#endif    
    std::unique_ptr<renderer::Composition> mRenderer{nullptr};
};

void AnimationImpl::setValue(const std::string &keypath, LOTVariant &&value)
{
    if (keypath.empty()) return;
    mRenderer->setValue(keypath, value);
}
#ifndef USING_MINI_RLOTTIE
const LOTLayerNode *AnimationImpl::renderTree(size_t frameNo, const VSize &size)
{
    if (update(frameNo, size, true)) {
        mRenderer->buildRenderTree();
    }
    return mRenderer->renderTree();
}
#endif
bool AnimationImpl::update(size_t frameNo, const VSize &size,
                           bool keepAspectRatio)
{
    frameNo += mModel->startFrame();

    if (frameNo > mModel->endFrame()) frameNo = mModel->endFrame();

    if (frameNo < mModel->startFrame()) frameNo = mModel->startFrame();

    return mRenderer->update(int(frameNo), size, keepAspectRatio);
}

bool AnimationImpl::updatePartial(size_t frameNo, const VSize &size, uint32_t offset)
{
    frameNo += mModel->startFrame();

    if (frameNo > mModel->endFrame()) frameNo = mModel->endFrame();

    if (frameNo < mModel->startFrame()) frameNo = mModel->startFrame();

    return mRenderer->updatePartial(int(frameNo), size, offset);
}


bool AnimationImpl::checkRender()
{
#ifdef MT_CPPLIB  
    bool renderInProgress = mRenderInProgress.load();
    if (renderInProgress) {
        vCritical << "Already Rendering Scheduled for this Animation";
        return false;
    }
    mRenderInProgress.store(true);
#endif
    return true;
}

Surface AnimationImpl::render(size_t frameNo, const Surface &surface,
                              bool keepAspectRatio)
{
    if (!checkRender()) return surface;
#ifdef MT_CPPLIB  
    bool renderInProgress = mRenderInProgress.load();
    if (renderInProgress) {
        vCritical << "Already Rendering Scheduled for this Animation";
        return surface;
    }

    mRenderInProgress.store(true);
#endif
    update(
        frameNo,
        VSize(int(surface.drawRegionWidth()), int(surface.drawRegionHeight())),
        keepAspectRatio);
    mRenderer->render(surface);
#ifdef MT_CPPLIB  
    mRenderInProgress.store(false);
#endif
    return surface;
}

Surface AnimationImpl::renderPartial(size_t frameNo, const Surface &surface)
{
    if (!checkRender()) return surface;

    updatePartial(
        frameNo,
        VSize(int(surface.drawRegionWidth()), int(surface.drawRegionHeight())),
        uint32_t(surface.drawRegionPosY()));
    mRenderer->renderPartial(surface);
#ifdef MT_CPPLIB  	
    mRenderInProgress.store(false);
#endif
    return surface;
}

void AnimationImpl::init(std::shared_ptr<model::Composition> composition)
{
    mModel = composition.get();
    mRenderer = std::make_unique<renderer::Composition>(composition);
    
#ifdef MT_CPPLIB  
    mRenderInProgress = false;
#endif
}

#ifdef LOTTIE_THREAD_SUPPORT

#include <thread>
#include "vtaskqueue.h"

/*
 * Implement a task stealing schduler to perform render task
 * As each player draws into its own buffer we can delegate this
 * task to a slave thread. The scheduler creates a threadpool depending
 * on the number of cores available in the system and does a simple fair
 * scheduling by assigning the task in a round-robin fashion. Each thread
 * in the threadpool has its own queue. once it finishes all the task on its
 * own queue it goes through rest of the queue and looks for task if it founds
 * one it steals the task from it and executes. if it couldn't find one then it
 * just waits for new task on its own queue.
 */
class RenderTaskScheduler {
    const unsigned           _count{std::thread::hardware_concurrency()};
    std::vector<std::thread> _threads;
    std::vector<TaskQueue<SharedRenderTask>> _q{_count};
    std::atomic<unsigned>                    _index{0};

    void run(unsigned i)
    {
        while (true) {
            bool             success = false;
            SharedRenderTask task;
            for (unsigned n = 0; n != _count * 2; ++n) {
                if (_q[(i + n) % _count].try_pop(task)) {
                    success = true;
                    break;
                }
            }
            if (!success && !_q[i].pop(task)) break;

            auto result = task->playerImpl->render(task->frameNo, task->surface,
                                                   task->keepAspectRatio);
            task->sender.set_value(result);
        }
    }

    RenderTaskScheduler()
    {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n] { run(n); });
        }

        IsRunning = true;
    }

public:
    static bool IsRunning;

    static RenderTaskScheduler &instance()
    {
        static RenderTaskScheduler singleton;
        return singleton;
    }

    ~RenderTaskScheduler() { stop(); }

    void stop()
    {
        if (IsRunning) {
            IsRunning = false;

            for (auto &e : _q) e.done();
            for (auto &e : _threads) e.join();
        }
    }

    std::future<Surface> process(SharedRenderTask task)
    {
        auto receiver = std::move(task->receiver);
        auto i = _index++;

        for (unsigned n = 0; n != _count; ++n) {
            if (_q[(i + n) % _count].try_push(std::move(task))) return receiver;
        }

        if (_count > 0) {
            _q[i % _count].push(std::move(task));
        }

        return receiver;
    }
};

#else
#ifdef MT_CPPLIB
class RenderTaskScheduler {
public:
    static bool IsRunning;

    static RenderTaskScheduler &instance()
    {
        static RenderTaskScheduler singleton;
        return singleton;
    }

    void stop() {}

    std::future<Surface> process(SharedRenderTask task)
    {
        auto result = task->playerImpl->render(task->frameNo, task->surface,
                                               task->keepAspectRatio);
        task->sender.set_value(result);
        return std::move(task->receiver);
    }
};
#endif    

#endif

#ifdef MT_CPPLIB
bool RenderTaskScheduler::IsRunning{false};

std::future<Surface> AnimationImpl::renderAsync(size_t    frameNo,
                                                Surface &&surface,
                                                bool      keepAspectRatio)
{
    if (!mTask) {
        mTask = std::make_shared<RenderTask>();
    } else {
        mTask->sender = std::promise<Surface>();
        mTask->receiver = mTask->sender.get_future();
    }
    mTask->playerImpl = this;
    mTask->frameNo = frameNo;
    mTask->surface = std::move(surface);
    mTask->keepAspectRatio = keepAspectRatio;

    return RenderTaskScheduler::instance().process(mTask);
}
#endif

/**
 * \breif Brief abput the Api.
 * Description about the setFilePath Api
 * @param path  add the details
 */
std::unique_ptr<Animation> Animation::loadFromData(
    std::string jsonData, const std::string &key,
    const std::string &resourcePath, bool cachePolicy)
{
    if (jsonData.empty()) {
        vWarning << "jason data is empty";
        return nullptr;
    }

    auto composition = model::loadFromData(std::move(jsonData), key,
                                           resourcePath, cachePolicy);
    if (composition) {
        auto animation = std::unique_ptr<Animation>(new Animation);
        animation->d->init(std::move(composition));
        return animation;
    }

    return nullptr;
}
#ifndef USING_MINI_RLOTTIE
std::unique_ptr<Animation> Animation::loadFromData(std::string jsonData,
                                                   std::string resourcePath,
                                                   ColorFilter filter)
{
    if (jsonData.empty()) {
        //vWarning << "jason data is empty";
        return nullptr;
    }

    auto composition = model::loadFromData(
        std::move(jsonData), std::move(resourcePath), std::move(filter));
    if (composition) {
        auto animation = std::unique_ptr<Animation>(new Animation);
        animation->d->init(std::move(composition));
        return animation;
    }
    return nullptr;
}
#endif
std::unique_ptr<Animation> Animation::loadFromROData(const char * data, const size_t len, 
                                                     const char * resourcePath)
{
    if (!data || !len) {
        //vWarning << "json data is empty";
        return nullptr;
    }

    auto composition = model::loadFromROData(data, len, resourcePath);
    if (composition) {
        auto animation = std::unique_ptr<Animation>(new Animation);
        animation->d->init(std::move(composition));
        return animation;
    }
    return nullptr;
}

std::unique_ptr<Animation> Animation::loadFromFile(const std::string &path,
                                                   bool cachePolicy)
{
    if (path.empty()) {
        //vWarning << "File path is empty";
        return nullptr;
    }

    auto composition = model::loadFromFile(path, cachePolicy);
    if (composition) {
        auto animation = std::unique_ptr<Animation>(new Animation);
        animation->d->init(std::move(composition));
        return animation;
    }
    return nullptr;
}

void Animation::size(size_t &width, size_t &height) const
{
    VSize sz = d->size();

    width = sz.width();
    height = sz.height();
}

double Animation::duration() const
{
    return d->duration();
}

double Animation::frameRate() const
{
    return d->frameRate();
}

size_t Animation::totalFrame() const
{
    return d->totalFrame();
}
#ifndef USING_MINI_RLOTTIE
size_t Animation::frameAtPos(double pos)
{
    return d->frameAtPos(pos);
}

const LOTLayerNode *Animation::renderTree(size_t frameNo, size_t width,
                                          size_t height) const
{
    return d->renderTree(frameNo, VSize(int(width), int(height)));
}
#endif
#ifdef MT_CPPLIB
std::future<Surface> Animation::render(size_t frameNo, Surface surface,
                                       bool keepAspectRatio)
{
    return d->renderAsync(frameNo, std::move(surface), keepAspectRatio);
}
#endif

void Animation::renderSync(size_t frameNo, Surface surface,
                           bool keepAspectRatio)
{
    d->render(frameNo, surface, keepAspectRatio);
}
void Animation::renderPartialSync(size_t frameNo, Surface surface)
{
    d->renderPartial(frameNo, surface);
}


const LayerInfoList &Animation::layers() const
{
    return d->layerInfoList();
}

const MarkerList &Animation::markers() const
{
    return d->markers();
}

void Animation::setValue(Color_Type, Property prop, const std::string &keypath,
                         Color value)
{
    d->setValue(keypath,
                LOTVariant(prop, [value](const FrameInfo &) { return value; }));
}

void Animation::setValue(Float_Type, Property prop, const std::string &keypath,
                         float value)
{
    d->setValue(keypath,
                LOTVariant(prop, [value](const FrameInfo &) { return value; }));
}

void Animation::setValue(Size_Type, Property prop, const std::string &keypath,
                         Size value)
{
    d->setValue(keypath,
                LOTVariant(prop, [value](const FrameInfo &) { return value; }));
}

void Animation::setValue(Point_Type, Property prop, const std::string &keypath,
                         Point value)
{
    d->setValue(keypath,
                LOTVariant(prop, [value](const FrameInfo &) { return value; }));
}

void Animation::setValue(Color_Type, Property prop, const std::string &keypath,
                         std::function<Color(const FrameInfo &)> &&value)
{
    d->setValue(keypath, LOTVariant(prop, value));
}

void Animation::setValue(Float_Type, Property prop, const std::string &keypath,
                         std::function<float(const FrameInfo &)> &&value)
{
    d->setValue(keypath, LOTVariant(prop, value));
}

void Animation::setValue(Size_Type, Property prop, const std::string &keypath,
                         std::function<Size(const FrameInfo &)> &&value)
{
    d->setValue(keypath, LOTVariant(prop, value));
}

void Animation::setValue(Point_Type, Property prop, const std::string &keypath,
                         std::function<Point(const FrameInfo &)> &&value)
{
    d->setValue(keypath, LOTVariant(prop, value));
}

Animation::~Animation() = default;
Animation::Animation() : d(std::make_unique<AnimationImpl>()) {}

Surface::Surface(uint32_t *buffer, size_t width, size_t height,
                 size_t bytesPerLine)
    : mBuffer(buffer),
      mWidth(width),
      mHeight(height),
      mBytesPerLine(bytesPerLine)
{
    mDrawArea.w = mWidth;
    mDrawArea.h = mHeight;
}

void Surface::setDrawRegion(size_t x, size_t y, size_t width, size_t height)
{
    if ((x + width > mWidth) || (y + height > mHeight)) return;

    mDrawArea.x = x;
    mDrawArea.y = y;
    mDrawArea.w = width;
    mDrawArea.h = height;
}

#ifdef MT_CPPLIB
namespace {
void lottieShutdownRenderTaskScheduler()
{
    if (RenderTaskScheduler::IsRunning) {
        RenderTaskScheduler::instance().stop();
    }
}
}  // namespace
#endif

// private apis exposed to c interface
void lottie_init_impl()
{
    // do nothing for now.
}


extern void lottieShutdownRasterTaskScheduler();

void lottie_shutdown_impl()
{
#ifdef MT_CPPLIB
    lottieShutdownRenderTaskScheduler();
    lottieShutdownRasterTaskScheduler();
#endif
}


#ifdef LOTTIE_LOGGING_SUPPORT
void initLogging()
{
#if defined(__ARM_NEON__)
    set_log_level(LogLevel::OFF);
#else
    initialize(GuaranteedLogger(), "/tmp/", "rlottie", 1);
    set_log_level(LogLevel::INFO);
#endif
}

V_CONSTRUCTOR_FUNCTION(initLogging)
#endif
