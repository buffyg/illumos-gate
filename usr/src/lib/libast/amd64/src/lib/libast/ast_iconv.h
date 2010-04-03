
/* : : generated by proto : : */
/* : : generated from /home/gisburn/ksh93/ast_ksh_20100309/build_i386_64bit/src/lib/libast/features/iconv by iffe version 2009-12-04 : : */
                  
#ifndef _def_iconv_ast
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

#define _def_iconv_ast	1
#define _sys_types	1	/* #include <sys/types.h> ok */
#define _hdr_iconv	1	/* #include <iconv.h> ok */
#define _lib_iconv_open	1	/* iconv_open() in default lib(s) */
#define _lib_iconv_close	1	/* iconv_close() in default lib(s) */
#define _lib_iconv	1	/* iconv() in default lib(s) */
#define _nxt_iconv <../include/iconv.h>	/* include path for the native <iconv.h> */
#define _nxt_iconv_str "../include/iconv.h"	/* include string for the native <iconv.h> */
#include <ast_common.h>
#include <ccode.h>
#include <../include/iconv.h>	/* the native iconv.h */

#define CC_ICONV	(-1)
#define CC_UCS		(-2)
#define CC_SCU		(-3)
#define CC_UTF		(-4)
#define CC_UME		(-5)

#ifndef _ICONV_LIST_PRIVATE_
#undef	iconv_t
#define	iconv_t		_ast_iconv_t
#undef	iconv_f
#define	iconv_f		_ast_iconv_f
#undef	iconv_list_t
#define	iconv_list_t	_ast_iconv_list_t
#undef	iconv_open
#define iconv_open	_ast_iconv_open
#undef	iconv
#define	iconv		_ast_iconv
#undef	iconv_close
#define iconv_close	_ast_iconv_close
#undef	iconv_list
#define iconv_list	_ast_iconv_list
#undef	iconv_move
#define iconv_move	_ast_iconv_move
#undef	iconv_name
#define iconv_name	_ast_iconv_name
#undef	iconv_write
#define iconv_write	_ast_iconv_write
#endif

typedef Ccmap_t _ast_iconv_list_t;
typedef __V_* _ast_iconv_t;
typedef size_t (*_ast_iconv_f) __PROTO__((_ast_iconv_t, char**, size_t*, char**, size_t*));

#if _BLD_ast && defined(__EXPORT__)
#undef __MANGLE__
#define __MANGLE__ __LINKAGE__		__EXPORT__
#endif

extern __MANGLE__ _ast_iconv_t	_ast_iconv_open __PROTO__((const char*, const char*));
extern __MANGLE__ size_t		_ast_iconv __PROTO__((_ast_iconv_t, char**, size_t*, char**, size_t*));
extern __MANGLE__ int		_ast_iconv_close __PROTO__((_ast_iconv_t));
extern __MANGLE__ _ast_iconv_list_t*	_ast_iconv_list __PROTO__((_ast_iconv_list_t*));
extern __MANGLE__ int		_ast_iconv_name __PROTO__((const char*, char*, size_t));
#if _SFIO_H
extern __MANGLE__ ssize_t		_ast_iconv_move __PROTO__((_ast_iconv_t, Sfio_t*, Sfio_t*, size_t, size_t*));
extern __MANGLE__ ssize_t		_ast_iconv_write __PROTO__((_ast_iconv_t, Sfio_t*, char**, size_t*, size_t*));
#else
#if _SFSTDIO_H
extern __MANGLE__ ssize_t		_ast_iconv_move __PROTO__((_ast_iconv_t, FILE*, FILE*, size_t, size_t*));
extern __MANGLE__ ssize_t		_ast_iconv_write __PROTO__((_ast_iconv_t, FILE*, char**, size_t*, size_t*));
#endif
#endif

#undef __MANGLE__
#define __MANGLE__ __LINKAGE__

#endif
