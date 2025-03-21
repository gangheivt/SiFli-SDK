/*
 * YAFFS: Yet another Flash File System . A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2018 Aleph One Ltd.
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 */


#ifndef __YPORTENV_H__
#define __YPORTENV_H__

#ifndef _MSC_VER
    #include <rtthread.h>
    #include <rtdevice.h>
#else
    #define CONFIG_YAFFS_NO_TRACE  1
    typedef signed long off_t;
    typedef int mode_t;
#endif /* _MSC_VER */

#define CONFIG_YAFFS_DIRECT 1
#define CONFIG_YAFFS_SHORT_NAMES_IN_RAM 1
#define CONFIG_YAFFS_YAFFS2 1
#define CONFIG_YAFFS_PROVIDE_DEFS 1
#define CONFIG_YAFFSFS_PROVIDE_VALUES 1
#define CONFIG_YAFFS_DEFINES_TYPES 1
#define CONFIG_YAFFS_USE_32_BIT_TIME_T 1
#define NO_Y_INLINE 1
#define loff_t off_t

#ifndef dev_t
    #define dev_t void*
#endif

/* Definition of types */
#ifdef CONFIG_YAFFS_DEFINES_TYPES
    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;
    typedef unsigned long long u64;
    typedef signed int s32;
#endif

#ifdef CONFIG_YAFFS_PROVIDE_DEFS
/* File types */


#ifndef DT_UNKNOWN
    #define DT_UNKNOWN  0
#endif
#define DT_FIFO     1
#define DT_CHR      2
#ifndef DT_DIR
    #define DT_DIR      4
#endif
#define DT_BLK      6
#ifndef DT_REG
    #define DT_REG      8
#endif
#define DT_LNK      10
#define DT_SOCK     12
#define DT_WHT      14


/*
 * Attribute flags.
 * These are or-ed together to select what has been changed.
 */
#define ATTR_MODE   1
#define ATTR_UID    2
#define ATTR_GID    4
#define ATTR_SIZE   8
#define ATTR_ATIME  16
#define ATTR_MTIME  32
#define ATTR_CTIME  64

struct iattr
{
    unsigned int ia_valid;
    unsigned ia_mode;
    unsigned ia_uid;
    unsigned ia_gid;
    unsigned ia_size;
    unsigned ia_atime;
    unsigned ia_mtime;
    unsigned ia_ctime;
    unsigned int ia_attr_flags;
};

#endif



#if defined CONFIG_YAFFS_WINCE

    #include "ywinceenv.h"


#elif defined CONFIG_YAFFS_DIRECT

    /* Direct interface */
    #include "ydirectenv.h"

#elif defined CONFIG_YAFFS_UTIL

    #include "yutilsenv.h"

#else
    /* Should have specified a configuration type */
    #error Unknown configuration

#endif

#if defined(CONFIG_YAFFS_DIRECT) || defined(CONFIG_YAFFS_WINCE)

    #ifdef CONFIG_YAFFSFS_PROVIDE_VALUES

        #ifndef O_RDONLY
            #define O_RDONLY    00
        #endif

        #ifndef O_WRONLY
            #define O_WRONLY    01
        #endif

        #ifndef O_RDWR
            #define O_RDWR      02
        #endif

        #ifndef O_CREAT
            #define O_CREAT     0100
        #endif

        #ifndef O_EXCL
            #define O_EXCL      0200
        #endif

        #ifndef O_TRUNC
            #define O_TRUNC     01000
        #endif

        #ifndef O_APPEND
            #define O_APPEND    02000
        #endif

        #ifndef SEEK_SET
            #define SEEK_SET    0
        #endif

        #ifndef SEEK_CUR
            #define SEEK_CUR    1
        #endif

        #ifndef SEEK_END
            #define SEEK_END    2
        #endif

        #ifndef EBUSY
            #define EBUSY   16
        #endif

        #ifndef ENODEV
            #define ENODEV  19
        #endif

        #ifndef EINVAL
            #define EINVAL  22
        #endif

        #ifndef ENFILE
            #define ENFILE  23
        #endif

        #ifndef EBADF
            #define EBADF   9
        #endif

        #ifndef EACCES
            #define EACCES  13
        #endif

        #ifndef EXDEV
            #define EXDEV   18
        #endif

        #ifndef ENOENT
            #define ENOENT  2
        #endif

        #ifndef ENOSPC
            #define ENOSPC  28
        #endif

        #ifndef EROFS
            #define EROFS   30
        #endif

        #ifndef ERANGE
            #define ERANGE 34
        #endif

        #ifndef ENODATA
            #define ENODATA 61
        #endif

        #ifndef ENOTEMPTY
            #define ENOTEMPTY 39
        #endif

        #ifndef ENAMETOOLONG
            #define ENAMETOOLONG 36
        #endif

        #ifndef ENOMEM
            #define ENOMEM 12
        #endif

        #ifndef EFAULT
            #define EFAULT 14
        #endif

        #ifndef EEXIST
            #define EEXIST 17
        #endif

        #ifndef ENOTDIR
            #define ENOTDIR 20
        #endif

        #ifndef EISDIR
            #define EISDIR 21
        #endif

        #ifndef ELOOP
            #define ELOOP   40
        #endif


        /* Mode flags */

        #ifndef S_IFMT
            #define S_IFMT      0170000
        #endif

        #ifndef S_IFSOCK
            #define S_IFSOCK    0140000
        #endif

        #ifndef S_IFIFO
            #define S_IFIFO     0010000
        #endif

        #ifndef S_IFCHR
            #define S_IFCHR     0020000
        #endif

        #ifndef S_IFBLK
            #define S_IFBLK     0060000
        #endif

        #ifndef S_IFLNK
            #define S_IFLNK     0120000
        #endif

        #ifndef S_IFDIR
            #define S_IFDIR     0040000
        #endif

        #ifndef S_IFREG
            #define S_IFREG     0100000
        #endif

        #ifndef S_ISSOCK
            #define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
        #endif
        #ifndef S_ISLNK
            #define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
        #endif
        #ifndef S_ISDIR
            #define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
        #endif
        #ifndef S_ISREG
            #define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
        #endif
        #ifndef S_ISBLK
            #define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
        #endif
        #ifndef S_ISCHR
            #define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
        #endif
        #ifndef S_ISFIFO
            #define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
        #endif


        #ifndef S_IRUSR
            #define S_IRUSR     0000400
        #endif

        #ifndef S_IWUSR
            #define S_IWUSR     0000200
        #endif

        #ifndef S_IXUSR
            #define S_IXUSR     0000100
        #endif

        #ifndef S_IREAD
            #define S_IREAD     0000400
        #endif

        #ifndef S_IWRITE
            #define S_IWRITE    0000200
        #endif

        #ifndef S_IEXEC
            #define S_IEXEC 0000100
        #endif


    #else
        #include <errno.h>
        #include <sys/stat.h>
        #include <fcntl.h>
    #endif

#endif

/* Create some less common define values if they don't exist */
#ifndef XATTR_CREATE
    #define XATTR_CREATE 1
#endif

#ifndef XATTR_REPLACE
    #define XATTR_REPLACE 2
#endif

#ifndef R_OK
    #define R_OK    4
    #define W_OK    2
    #define X_OK    1
    #define F_OK    0
#endif

#ifndef S_ISSOCK
    #define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif

#ifndef Y_DUMP_STACK
    #define Y_DUMP_STACK() do { } while (0)
#endif

#ifndef BUG
#define BUG() do {\
    yaffs_trace(YAFFS_TRACE_BUG,\
        "==>> yaffs bug: " __FILE__ " %d",\
        __LINE__);\
    Y_DUMP_STACK();\
} while (0)
#endif

#endif
