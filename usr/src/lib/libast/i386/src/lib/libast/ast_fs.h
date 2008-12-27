
/* : : generated by proto : : */
/* : : generated from /home/gisburn/ksh93/ast_ksh_20081104/build_i386_32bit/src/lib/libast/features/fs by iffe version 2008-01-31 : : */
                  
#ifndef _def_fs_ast
#if !defined(__PROTO__)
#  if defined(__STDC__) || defined(__cplusplus) || defined(_proto) || defined(c_plusplus)
#    if defined(__cplusplus)
#      define __LINKAGE__	"C"
#    else
#      define __LINKAGE__
#    endif
#    define __STDARG__
#    define __PROTO__(x)	x
#    define __OTORP__(x)
#    define __PARAM__(n,o)	n
#    if !defined(__STDC__) && !defined(__cplusplus)
#      if !defined(c_plusplus)
#      	define const
#      endif
#      define signed
#      define void		int
#      define volatile
#      define __V_		char
#    else
#      define __V_		void
#    endif
#  else
#    define __PROTO__(x)	()
#    define __OTORP__(x)	x
#    define __PARAM__(n,o)	o
#    define __LINKAGE__
#    define __V_		char
#    define const
#    define signed
#    define void		int
#    define volatile
#  endif
#  define __MANGLE__	__LINKAGE__
#  if defined(__cplusplus) || defined(c_plusplus)
#    define __VARARG__	...
#  else
#    define __VARARG__
#  endif
#  if defined(__STDARG__)
#    define __VA_START__(p,a)	va_start(p,a)
#  else
#    define __VA_START__(p,a)	va_start(p)
#  endif
#  if !defined(__INLINE__)
#    if defined(__cplusplus)
#      define __INLINE__	extern __MANGLE__ inline
#    else
#      if defined(_WIN32) && !defined(__GNUC__)
#      	define __INLINE__	__inline
#      endif
#    endif
#  endif
#endif
#if !defined(__LINKAGE__)
#define __LINKAGE__		/* 2004-08-11 transition */
#endif

#define _def_fs_ast	1
#define _sys_types	1	/* #include <sys/types.h> ok */
#define _sys_stat	1	/* #include <sys/stat.h> ok */
#define _lib__fxstat	1	/* _fxstat() in default lib(s) */
#define _lib__lxstat	1	/* _lxstat() in default lib(s) */
#define _lib__xmknod	1	/* _xmknod() in default lib(s) */
#define _lib__xstat	1	/* _xstat() in default lib(s) */
#define _lib_lstat	1	/* lstat() in default lib(s) */
#define _lib_mknod	1	/* mknod() in default lib(s) */
#define _lib_sync	1	/* sync() in default lib(s) */
#include <sys/stat.h>
#include <sys/mkdev.h>
#define FS_default	"ufs"
#if defined(__STDPP__directive) && defined(__STDPP__initial)
__STDPP__directive pragma pp:noinitial
#endif
#define _hdr_stdio	1	/* #include <stdio.h> ok */
#define _sys_mntent	1	/* #include <sys/mntent.h> ok */
#define _sys_mnttab	1	/* #include <sys/mnttab.h> ok */
#define _mem_st_blocks_stat	1	/* st_blocks is a member of struct stat */
#define _mem_st_blksize_stat	1	/* st_blksize is a member of struct stat */
#define _mem_st_rdev_stat	1	/* st_rdev is a member of struct stat */
#define _sys_statfs	1	/* #include <sys/statfs.h> ok */
#define _mem_f_files_statfs	1	/* f_files is a member of struct statfs */
#define _sys_vfs	1	/* #include <sys/vfs.h> ok */
#define _sys_param	1	/* #include <sys/param.h> ok */
#define _sys_mount	1	/* #include <sys/mount.h> ok */
#define _sys_statvfs	1	/* #include <sys/statvfs.h> ok */
#define _mem_f_basetype_statvfs	1	/* f_basetype is a member of struct statvfs */
#define _mem_f_frsize_statvfs	1	/* f_frsize is a member of struct statvfs */
#define _lib_getmntent	1	/* getmntent() in default lib(s) */
#define _lib_statfs	1	/* statfs() in default lib(s) */
#define _lib_statvfs	1	/* statvfs() in default lib(s) */
#define _lib_statfs4	1	/* compile{\ passed */
#if _sys_statvfs
#include <sys/statvfs.h>
#if !_mem_statvfs_f_basetype
#if _ary_f_reserved7
#define f_basetype	f_reserved7
#endif
#endif
#else
#define _mem_f_basetype_statvfs	1
#define _mem_f_frsize_statvfs	1
struct statvfs
{
unsigned long	f_bsize;	/* fundamental file system block size */
unsigned long	f_frsize;	/* fragment size */
unsigned long	f_blocks;	/* total # of blocks of f_frsize on fs */
unsigned long	f_bfree;	/* total # of free blocks of f_frsize */
unsigned long	f_bavail;	/* # of free blocks avail to non-superuser */
unsigned long	f_files;	/* total # of file nodes (inodes) */
unsigned long	f_ffree;	/* total # of free file nodes */
unsigned long	f_favail;	/* # of free nodes avail to non-superuser */
unsigned long	f_fsid;		/* file system id (dev for now) */
char		f_basetype[16]; /* target fs type name, null-terminated */
unsigned long	f_flag;		/* bit-mask of flags */
unsigned long	f_namemax;	/* maximum file name length */
char		f_fstr[32];	/* filesystem-specific string */
unsigned long	f_filler[16];	/* reserved for future expansion */
};
extern __MANGLE__ int	fstatvfs __PROTO__((int, struct statvfs*));
extern __MANGLE__ int	statvfs __PROTO__((const char*, struct statvfs*));
#endif
#if _typ_off64_t
#undef	off_t
#define off_t	off64_t
#endif
#if _lib_statvfs64 && !defined(statvfs)
#define statvfs		statvfs64
#if !defined(__USE_LARGEFILE64)
extern __MANGLE__ int		statvfs64 __PROTO__((const char*, struct statvfs64*));
#endif
#endif
#if _lib_fstatvfs64 && !defined(fstatvfs)
#define fstatvfs	fstatvfs64
#if !defined(__USE_LARGEFILE64)
extern __MANGLE__ int		fstatvfs64 __PROTO__((int, struct statvfs64*));
#endif
#endif

#define _str_st_fstype	1	/* stat.st_fstype is a string */
#define _ary_st_pad4	1	/* stat.st_pad4 is an array */
#endif
