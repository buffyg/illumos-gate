/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * This file is included in library source files (other than libc) when it
 * is desired to call libc functions by their restricted names rather than
 * by their public names, to avoid a namespace collision with applications.
 */

#ifndef _COMMON_INC_SYNONYMS_H
#define	_COMMON_INC_SYNONYMS_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/feature_tests.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__lint)

/* external data */
#define	altzone				_altzone
#define	_ctype				__ctype
#define	daylight			_daylight
#define	lone				_lone
#define	lten				_lten
#define	lzero				_lzero
#define	timezone			_timezone
#define	tzname				_tzname

/* functions */
#define	a64l				_a64l
#define	access				_access
#define	acct				_acct
#define	acctctl				_acctctl
#define	acl				_acl
#define	addseverity			_addseverity
#define	adjtime				_adjtime
#define	alarm				_alarm
#define	alphasort64			_alphasort64
#define	alphasort			_alphasort
#define	ascftime			_ascftime
#define	asctime_r			_asctime_r
#define	_assert				__assert
#define	_assert_c99			__assert_c99
#define	atoll				_atoll
#define	atomic_add_16			_atomic_add_16
#define	atomic_add_16_nv		_atomic_add_16_nv
#define	atomic_add_32			_atomic_add_32
#define	atomic_add_32_nv		_atomic_add_32_nv
#define	atomic_add_64			_atomic_add_64
#define	atomic_add_64_nv		_atomic_add_64_nv
#define	atomic_add_8			_atomic_add_8
#define	atomic_add_8_nv			_atomic_add_8_nv
#define	atomic_add_char			_atomic_add_char
#define	atomic_add_char_nv		_atomic_add_char_nv
#define	atomic_add_int			_atomic_add_int
#define	atomic_add_int_nv		_atomic_add_int_nv
#define	atomic_add_long			_atomic_add_long
#define	atomic_add_long_nv		_atomic_add_long_nv
#define	atomic_add_ptr			_atomic_add_ptr
#define	atomic_add_ptr_nv		_atomic_add_ptr_nv
#define	atomic_add_short		_atomic_add_short
#define	atomic_add_short_nv		_atomic_add_short_nv
#define	atomic_and_16			_atomic_and_16
#define	atomic_and_16_nv		_atomic_and_16_nv
#define	atomic_and_32			_atomic_and_32
#define	atomic_and_32_nv		_atomic_and_32_nv
#define	atomic_and_64			_atomic_and_64
#define	atomic_and_64_nv		_atomic_and_64_nv
#define	atomic_and_8			_atomic_and_8
#define	atomic_and_8_nv			_atomic_and_8_nv
#define	atomic_and_uchar		_atomic_and_uchar
#define	atomic_and_uchar_nv		_atomic_and_uchar_nv
#define	atomic_and_uint			_atomic_and_uint
#define	atomic_and_uint_nv		_atomic_and_uint_nv
#define	atomic_and_ulong		_atomic_and_ulong
#define	atomic_and_ulong_nv		_atomic_and_ulong_nv
#define	atomic_and_ushort		_atomic_and_ushort
#define	atomic_and_ushort_nv		_atomic_and_ushort_nv
#define	atomic_cas_16			_atomic_cas_16
#define	atomic_cas_32			_atomic_cas_32
#define	atomic_cas_64			_atomic_cas_64
#define	atomic_cas_8			_atomic_cas_8
#define	atomic_cas_ptr			_atomic_cas_ptr
#define	atomic_cas_uchar		_atomic_cas_uchar
#define	atomic_cas_uint			_atomic_cas_uint
#define	atomic_cas_ulong		_atomic_cas_ulong
#define	atomic_cas_ushort		_atomic_cas_ushort
#define	atomic_clear_long_excl		_atomic_clear_long_excl
#define	atomic_dec_16			_atomic_dec_16
#define	atomic_dec_16_nv		_atomic_dec_16_nv
#define	atomic_dec_32			_atomic_dec_32
#define	atomic_dec_32_nv		_atomic_dec_32_nv
#define	atomic_dec_64			_atomic_dec_64
#define	atomic_dec_64_nv		_atomic_dec_64_nv
#define	atomic_dec_8			_atomic_dec_8
#define	atomic_dec_8_nv			_atomic_dec_8_nv
#define	atomic_dec_uchar		_atomic_dec_uchar
#define	atomic_dec_uchar_nv		_atomic_dec_uchar_nv
#define	atomic_dec_uint			_atomic_dec_uint
#define	atomic_dec_uint_nv		_atomic_dec_uint_nv
#define	atomic_dec_ulong		_atomic_dec_ulong
#define	atomic_dec_ulong_nv		_atomic_dec_ulong_nv
#define	atomic_dec_ushort		_atomic_dec_ushort
#define	atomic_dec_ushort_nv		_atomic_dec_ushort_nv
#define	atomic_inc_16			_atomic_inc_16
#define	atomic_inc_16_nv		_atomic_inc_16_nv
#define	atomic_inc_32			_atomic_inc_32
#define	atomic_inc_32_nv		_atomic_inc_32_nv
#define	atomic_inc_64			_atomic_inc_64
#define	atomic_inc_64_nv		_atomic_inc_64_nv
#define	atomic_inc_8			_atomic_inc_8
#define	atomic_inc_8_nv			_atomic_inc_8_nv
#define	atomic_inc_uchar		_atomic_inc_uchar
#define	atomic_inc_uchar_nv		_atomic_inc_uchar_nv
#define	atomic_inc_uint			_atomic_inc_uint
#define	atomic_inc_uint_nv		_atomic_inc_uint_nv
#define	atomic_inc_ulong		_atomic_inc_ulong
#define	atomic_inc_ulong_nv		_atomic_inc_ulong_nv
#define	atomic_inc_ushort		_atomic_inc_ushort
#define	atomic_inc_ushort_nv		_atomic_inc_ushort_nv
#define	atomic_or_16			_atomic_or_16
#define	atomic_or_16_nv			_atomic_or_16_nv
#define	atomic_or_32			_atomic_or_32
#define	atomic_or_32_nv			_atomic_or_32_nv
#define	atomic_or_64			_atomic_or_64
#define	atomic_or_64_nv			_atomic_or_64_nv
#define	atomic_or_8			_atomic_or_8
#define	atomic_or_8_nv			_atomic_or_8_nv
#define	atomic_or_uchar			_atomic_or_uchar
#define	atomic_or_uchar_nv		_atomic_or_uchar_nv
#define	atomic_or_uint			_atomic_or_uint
#define	atomic_or_uint_nv		_atomic_or_uint_nv
#define	atomic_or_ulong			_atomic_or_ulong
#define	atomic_or_ulong_nv		_atomic_or_ulong_nv
#define	atomic_or_ushort		_atomic_or_ushort
#define	atomic_or_ushort_nv		_atomic_or_ushort_nv
#define	atomic_set_long_excl		_atomic_set_long_excl
#define	atomic_swap_16			_atomic_swap_16
#define	atomic_swap_32			_atomic_swap_32
#define	atomic_swap_64			_atomic_swap_64
#define	atomic_swap_8			_atomic_swap_8
#define	atomic_swap_ptr			_atomic_swap_ptr
#define	atomic_swap_uchar		_atomic_swap_uchar
#define	atomic_swap_uint		_atomic_swap_uint
#define	atomic_swap_ulong		_atomic_swap_ulong
#define	atomic_swap_ushort		_atomic_swap_ushort
#define	attropen			_attropen
#define	brk				_brk
#define	catclose			_catclose
#define	catgets				_catgets
#define	catopen				_catopen
#define	cfgetispeed			_cfgetispeed
#define	cfgetospeed			_cfgetospeed
#define	cfsetispeed			_cfsetispeed
#define	cfsetospeed			_cfsetospeed
#define	cftime				_cftime
#define	chdir				_chdir
#define	chmod				_chmod
#define	chown				_chown
#define	chroot				_chroot
#define	_cladm				__cladm
#define	clock_getres			_clock_getres
#define	clock_gettime			_clock_gettime
#define	clock_nanosleep			_clock_nanosleep
#define	clock_settime			_clock_settime
#define	close				_close
#define	closedir			_closedir
#define	closefrom			_closefrom
#define	closelog			_closelog
#define	cond_broadcast			_cond_broadcast
#define	cond_destroy			_cond_destroy
#define	cond_init			_cond_init
#define	cond_reltimedwait		_cond_reltimedwait
#define	cond_signal			_cond_signal
#define	cond_timedwait			_cond_timedwait
#define	cond_wait			_cond_wait
#define	core_get_default_content	_core_get_default_content
#define	core_get_default_path		_core_get_default_path
#define	core_get_global_content		_core_get_global_content
#define	core_get_global_path		_core_get_global_path
#define	core_get_options		_core_get_options
#define	core_get_process_content	_core_get_process_content
#define	core_get_process_path		_core_get_process_path
#define	core_set_default_content	_core_set_default_content
#define	core_set_default_path		_core_set_default_path
#define	core_set_global_content		_core_set_global_content
#define	core_set_global_path		_core_set_global_path
#define	core_set_options		_core_set_options
#define	core_set_process_content	_core_set_process_content
#define	core_set_process_path		_core_set_process_path
#define	creat				_creat
#define	crypt				_crypt
#define	ctermid				_ctermid
#define	ctermid_r			_ctermid_r
#define	ctime_r				_ctime_r
#define	cuserid				_cuserid
#define	decimal_to_double		_decimal_to_double
#define	decimal_to_extended		_decimal_to_extended
#define	decimal_to_quadruple		_decimal_to_quadruple
#define	decimal_to_single		_decimal_to_single
#define	defcntl				_defcntl
#define	defopen				_defopen
#define	defread				_defread
#define	dgettext			_dgettext
#define	dladdr1				_dladdr1
#define	dladdr				_dladdr
#define	dlamd64getunwind		_dlamd64getunwind
#define	dlclose				_dlclose
#define	dldump				_dldump
#define	dlerror				_dlerror
#define	dlinfo				_dlinfo
#define	dlmopen				_dlmopen
#define	dlopen				_dlopen
#define	dlsym				_dlsym
#define	door_bind			_door_bind
#define	door_call			_door_call
#define	door_create			_door_create
#define	door_cred			_door_cred
#define	door_getparam			_door_getparam
#define	door_info			_door_info
#define	door_return			_door_return
#define	door_revoke			_door_revoke
#define	door_server_create		_door_server_create
#define	door_setparam			_door_setparam
#define	door_ucred			_door_ucred
#define	door_unbind			_door_unbind
#define	double_to_decimal		_double_to_decimal
#define	drand48				_drand48
#define	dup2				_dup2
#define	dup				_dup
#define	econvert			_econvert
#define	ecvt				_ecvt
#define	encrypt				_encrypt
#define	endgrent			_endgrent
#define	endpwent			_endpwent
#define	endspent			_endspent
#define	endutent			_endutent
#define	endutxent			_endutxent
#define	environ				_environ
#define	erand48				_erand48
#define	execle				_execle
#define	execl				_execl
#define	execlp				_execlp
#define	execve				_execve
#define	execv				_execv
#define	execvp				_execvp
#define	exportfs			_exportfs
#define	extended_to_decimal		_extended_to_decimal
#define	facl				_facl
#define	fattach				_fattach
#define	fchdir				_fchdir
#define	fchmod				_fchmod
#define	fchownat			_fchownat
#define	fchown				_fchown
#define	fchroot				_fchroot
#define	fcntl				_fcntl
#define	fconvert			_fconvert
#define	fcvt				_fcvt
#define	fdetach				_fdetach
#define	fdopendir			_fdopendir
#define	fdopen				_fdopen
#define	fdwalk				_fdwalk
#define	ffs				_ffs
#define	fgetgrent			_fgetgrent
#define	fgetgrent_r			_fgetgrent_r
#define	fgetpwent			_fgetpwent
#define	fgetpwent_r			_fgetpwent_r
#define	fgetspent			_fgetspent
#define	fgetspent_r			_fgetspent_r
#define	fileno				_fileno
#define	file_to_decimal			_file_to_decimal
#define	finite				_finite
#define	flockfile			_flockfile
#define	fmtmsg				_fmtmsg
#define	fork1				_fork1
#define	forkall				_forkall
#define	forkallx			_forkallx
#define	fork				_fork
#define	forkx				_forkx
#define	fpathconf			_fpathconf
#define	fpclass				_fpclass
#define	fpgetmask			_fpgetmask
#define	fpgetround			_fpgetround
#define	fpgetsticky			_fpgetsticky
#define	fprintf				_fprintf
#define	fpsetmask			_fpsetmask
#define	fpsetround			_fpsetround
#define	fpsetsticky			_fpsetsticky
#define	fstatat				_fstatat
#define	fstatfs				_fstatfs
#define	fstatvfs			_fstatvfs
#define	fsync				_fsync
#define	ftok				_ftok
#define	ftruncate			_ftruncate
#define	ftrylockfile			_ftrylockfile
#define	ftw				_ftw
#define	func_to_decimal			_func_to_decimal
#define	funlockfile			_funlockfile
#define	futimesat			_futimesat
#define	gconvert			_gconvert
#define	gcvt				_gcvt
#define	getacct				_getacct
#define	getcontext			_getcontext
#define	getcpuid			_getcpuid
#define	getcwd				_getcwd
#define	getdate				_getdate
#define	getdents			_getdents
#define	getegid				_getegid
#define	geteuid				_geteuid
#define	getexecname			_getexecname
#define	getgid				_getgid
#define	getgrent			_getgrent
#define	getgrent_r			_getgrent_r
#define	getgrgid			_getgrgid
#define	getgrgid_r			_getgrgid_r
#define	getgrnam			_getgrnam
#define	getgrnam_r			_getgrnam_r
#define	getgroups			_getgroups
#define	gethomelgroup			_gethomelgroup
#define	getisax				_getisax
#define	getitimer			_getitimer
#define	getlogin			_getlogin
#define	getlogin_r			_getlogin_r
#define	getmntany			_getmntany
#define	getmntent			_getmntent
#define	getmsg				_getmsg
#define	getopt_clip			_getopt_clip
#define	getopt				_getopt
#define	getopt_long			_getopt_long
#define	getopt_long_only		_getopt_long_only
#define	getpass				_getpass
#define	getpeerucred			_getpeerucred
#define	getpflags			_getpflags
#define	getpgid				_getpgid
#define	getpgrp				_getpgrp
#define	getpid				_getpid
#define	getpmsg				_getpmsg
#define	getppid				_getppid
#define	getppriv			_getppriv
#define	getprivimplinfo			_getprivimplinfo
#define	getprojid			_getprojid
#define	getpwent			_getpwent
#define	getpwent_r			_getpwent_r
#define	getpw				_getpw
#define	getpwnam			_getpwnam
#define	getpwnam_r			_getpwnam_r
#define	getpwuid			_getpwuid
#define	getpwuid_r			_getpwuid_r
#define	getrctl				_getrctl
#define	getrlimit			_getrlimit
#define	getsid				_getsid
#define	getspent			_getspent
#define	getspent_r			_getspent_r
#define	getspnam			_getspnam
#define	getspnam_r			_getspnam_r
#define	getsubopt			_getsubopt
#define	gettaskid			_gettaskid
#define	gettimeofday			_gettimeofday
#define	gettxt				_gettxt
#define	getuid				_getuid
#define	getustack			_getustack
#define	getutent			_getutent
#define	getutid				_getutid
#define	getutline			_getutline
#define	getutmp				_getutmp
#define	getutmpx			_getutmpx
#define	getutxent			_getutxent
#define	getutxid			_getutxid
#define	getutxline			_getutxline
#define	getvfsany			_getvfsany
#define	getvfsent			_getvfsent
#define	getvfsfile			_getvfsfile
#define	getvfsspec			_getvfsspec
#define	getw				_getw
#define	getzoneidbyname			_getzoneidbyname
#define	getzoneid			_getzoneid
#define	getzonenamebyid			_getzonenamebyid
#define	gmtime_r			_gmtime_r
#define	grantpt				_grantpt
#define	gsignal				_gsignal
#define	gtty				_gtty
#define	hasmntopt			_hasmntopt
#define	hcreate				_hcreate
#define	hdestroy			_hdestroy
#define	hsearch				_hsearch
#define	iconv_close			_iconv_close
#define	iconv				_iconv
#define	iconv_open			_iconv_open
#define	initgroups			_initgroups
#define	insque				_insque
#define	install_utrap			_install_utrap
#define	inst_sync			_inst_sync
#define	_iob				__iob
#define	ioctl				_ioctl
#define	isastream			_isastream
#define	isatty				_isatty
#define	issetugid			_issetugid
#define	is_system_labeled		_is_system_labeled
#define	jrand48				_jrand48
#define	kill				_kill
#define	l64a				_l64a
#define	ladd				_ladd
#define	lchown				_lchown
#define	lckpwdf				_lckpwdf
#define	lcong48				_lcong48
#define	ldivide				_ldivide
#define	lexp10				_lexp10
#define	lfind				_lfind
#define	link				_link
#define	llabs				_llabs
#define	lldiv				_lldiv
#define	llog10				_llog10
#define	llseek				_llseek
#define	lltostr				_lltostr
#define	lmul				_lmul
#define	localtime_r			_localtime_r
#define	lockf				_lockf
#define	longjmp				_longjmp
#define	lrand48				_lrand48
#define	lsearch				_lsearch
#define	lseek				_lseek
#define	lshiftl				_lshiftl
#define	lsub				_lsub
#define	_lwp_cond_broadcast		__lwp_cond_broadcast
#define	_lwp_cond_reltimedwait		__lwp_cond_reltimedwait
#define	_lwp_cond_signal		__lwp_cond_signal
#define	_lwp_cond_timedwait		__lwp_cond_timedwait
#define	_lwp_cond_wait			__lwp_cond_wait
#define	_lwp_continue			__lwp_continue
#define	_lwp_info			__lwp_info
#define	_lwp_kill			__lwp_kill
#define	_lwp_mutex_lock			__lwp_mutex_lock
#define	_lwp_mutex_trylock		__lwp_mutex_trylock
#define	_lwp_mutex_unlock		__lwp_mutex_unlock
#define	_lwp_self			__lwp_self
#define	_lwp_sema_init			__lwp_sema_init
#define	_lwp_sema_post			__lwp_sema_post
#define	_lwp_sema_trywait		__lwp_sema_trywait
#define	_lwp_sema_wait			__lwp_sema_wait
#define	_lwp_suspend			__lwp_suspend
#define	madvise				_madvise
#define	makecontext			_makecontext
#define	makeut				_makeut
#define	makeutx				_makeutx
#define	membar_consumer			_membar_consumer
#define	membar_enter			_membar_enter
#define	membar_exit			_membar_exit
#define	membar_producer			_membar_producer
#define	memccpy				_memccpy
#define	memcmp				_memcmp
#define	memcntl				_memcntl
#define	memcpy				_memcpy
#define	meminfo				_meminfo
#define	memmove				_memmove
#define	memset				_memset
#define	mincore				_mincore
#define	mkdir				_mkdir
#define	mkdtemp				_mkdtemp
#define	mkfifo				_mkfifo
#define	mkstemp				_mkstemp
#define	mkstemps			_mkstemps
#define	mktemp				_mktemp
#define	mlockall			_mlockall
#define	mlock				_mlock
#define	mmap				_mmap
#define	modutx				_modutx
#define	monitor				_monitor
#define	mount				_mount
#define	mprotect			_mprotect
#define	mrand48				_mrand48
#define	msgctl64			_msgctl64
#define	msgctl				_msgctl
#define	msgget				_msgget
#define	msgids				_msgids
#define	msgrcv				_msgrcv
#define	msgsnap				_msgsnap
#define	msgsnd				_msgsnd
#define	msync				_msync
#define	munlockall			_munlockall
#define	munlock				_munlock
#define	munmap				_munmap
#define	_mutex_destroy			__mutex_destroy
#define	mutex_destroy			__mutex_destroy
#define	_mutex_held			__mutex_held
#define	mutex_held			__mutex_held
#define	_mutex_init			__mutex_init
#define	mutex_init			__mutex_init
#define	_mutex_lock			__mutex_lock
#define	mutex_lock			__mutex_lock
#define	_mutex_trylock			__mutex_trylock
#define	mutex_trylock			__mutex_trylock
#define	_mutex_unlock			__mutex_unlock
#define	mutex_unlock			__mutex_unlock
#define	nanosleep			_nanosleep
#define	nfs_getfh			_nfs_getfh
#define	nfssvc				_nfssvc
#define	nftw				_nftw
#define	nice				_nice
#define	nl_langinfo			_nl_langinfo
#define	nrand48				_nrand48
#define	nss_default_finders		_nss_default_finders
#define	nss_delete			_nss_delete
#define	nss_endent			_nss_endent
#define	nss_getent			_nss_getent
#define	nss_search			_nss_search
#define	nss_setent			_nss_setent
#define	ntp_adjtime			_ntp_adjtime
#define	ntp_gettime			_ntp_gettime
#define	openat				_openat
#define	opendir				_opendir
#define	openlog				_openlog
#define	open				_open
#define	pathconf			_pathconf
#define	pause				_pause
#define	pclose				_pclose
#define	pipe				_pipe
#define	plock				_plock
#define	poll				_poll
#define	popen				_popen
#define	port_alert			_port_alert
#define	port_associate			_port_associate
#define	port_create			_port_create
#define	port_dissociate			_port_dissociate
#define	port_getn			_port_getn
#define	port_get			_port_get
#define	port_sendn			_port_sendn
#define	port_send			_port_send
#define	posix_openpt			_posix_openpt
#define	posix_spawnattr_destroy		_posix_spawnattr_destroy
#define	posix_spawnattr_getflags	_posix_spawnattr_getflags
#define	posix_spawnattr_getpgroup	_posix_spawnattr_getpgroup
#define	posix_spawnattr_getschedparam	_posix_spawnattr_getschedparam
#define	posix_spawnattr_getschedpolicy	_posix_spawnattr_getschedpolicy
#define	posix_spawnattr_getsigdefault	_posix_spawnattr_getsigdefault
#define	posix_spawnattr_getsigmask	_posix_spawnattr_getsigmask
#define	posix_spawnattr_init		_posix_spawnattr_init
#define	posix_spawnattr_setflags	_posix_spawnattr_setflags
#define	posix_spawnattr_setpgroup	_posix_spawnattr_setpgroup
#define	posix_spawnattr_setschedparam	_posix_spawnattr_setschedparam
#define	posix_spawnattr_setschedpolicy	_posix_spawnattr_setschedpolicy
#define	posix_spawnattr_setsigdefault	_posix_spawnattr_setsigdefault
#define	posix_spawnattr_setsigmask	_posix_spawnattr_setsigmask
#define	posix_spawn_file_actions_addclose _posix_spawn_file_actions_addclose
#define	posix_spawn_file_actions_adddup2 _posix_spawn_file_actions_adddup2
#define	posix_spawn_file_actions_addopen _posix_spawn_file_actions_addopen
#define	posix_spawn_file_actions_destroy _posix_spawn_file_actions_destroy
#define	posix_spawn_file_actions_init	_posix_spawn_file_actions_init
#define	posix_spawn			_posix_spawn
#define	posix_spawnp			_posix_spawnp
#define	pread				_pread
#define	printstack			_printstack
#define	priv_addset			_priv_addset
#define	priv_allocset			_priv_allocset
#define	priv_copyset			_priv_copyset
#define	priv_delset			_priv_delset
#define	priv_emptyset			_priv_emptyset
#define	priv_fillset			_priv_fillset
#define	priv_freeset			_priv_freeset
#define	priv_getbyname			_priv_getbyname
#define	priv_getbynum			_priv_getbynum
#define	priv_getsetbyname		_priv_getsetbyname
#define	priv_getsetbynum		_priv_getsetbynum
#define	priv_gettext			_priv_gettext
#define	priv_ineffect			_priv_ineffect
#define	priv_intersect			_priv_intersect
#define	priv_inverse			_priv_inverse
#define	priv_isemptyset			_priv_isemptyset
#define	priv_isequalset			_priv_isequalset
#define	priv_isfullset			_priv_isfullset
#define	priv_ismember			_priv_ismember
#define	priv_issubset			_priv_issubset
#define	priv_set			_priv_set
#define	priv_set_to_str			_priv_set_to_str
#define	priv_str_to_set			_priv_str_to_set
#define	priv_union			_priv_union
#define	profil				_profil
#define	pselect				_pselect
#define	psiginfo			_psiginfo
#define	psignal				_psignal
#define	pthread_atfork			_pthread_atfork
#define	pthread_attr_destroy		_pthread_attr_destroy
#define	pthread_attr_getdetachstate	_pthread_attr_getdetachstate
#define	pthread_attr_getguardsize	_pthread_attr_getguardsize
#define	pthread_attr_getinheritsched	_pthread_attr_getinheritsched
#define	pthread_attr_getschedparam	_pthread_attr_getschedparam
#define	pthread_attr_getschedpolicy	_pthread_attr_getschedpolicy
#define	pthread_attr_getscope		_pthread_attr_getscope
#define	pthread_attr_getstackaddr	_pthread_attr_getstackaddr
#define	pthread_attr_getstack		_pthread_attr_getstack
#define	pthread_attr_getstacksize	_pthread_attr_getstacksize
#define	pthread_attr_init		_pthread_attr_init
#define	pthread_attr_setdetachstate	_pthread_attr_setdetachstate
#define	pthread_attr_setguardsize	_pthread_attr_setguardsize
#define	pthread_attr_setinheritsched	_pthread_attr_setinheritsched
#define	pthread_attr_setschedparam	_pthread_attr_setschedparam
#define	pthread_attr_setschedpolicy	_pthread_attr_setschedpolicy
#define	pthread_attr_setscope		_pthread_attr_setscope
#define	pthread_attr_setstackaddr	_pthread_attr_setstackaddr
#define	pthread_attr_setstack		_pthread_attr_setstack
#define	pthread_attr_setstacksize	_pthread_attr_setstacksize
#define	pthread_barrierattr_destroy	_pthread_barrierattr_destroy
#define	pthread_barrierattr_getpshared	_pthread_barrierattr_getpshared
#define	pthread_barrierattr_init	_pthread_barrierattr_init
#define	pthread_barrierattr_setpshared	_pthread_barrierattr_setpshared
#define	pthread_barrier_destroy		_pthread_barrier_destroy
#define	pthread_barrier_init		_pthread_barrier_init
#define	pthread_barrier_wait		_pthread_barrier_wait
#define	pthread_cancel			_pthread_cancel
#define	pthread_condattr_destroy	_pthread_condattr_destroy
#define	pthread_condattr_getclock	_pthread_condattr_getclock
#define	pthread_condattr_getpshared	_pthread_condattr_getpshared
#define	pthread_condattr_init		_pthread_condattr_init
#define	pthread_condattr_setclock	_pthread_condattr_setclock
#define	pthread_condattr_setpshared	_pthread_condattr_setpshared
#define	pthread_cond_broadcast		_pthread_cond_broadcast
#define	pthread_cond_destroy		_pthread_cond_destroy
#define	pthread_cond_init		_pthread_cond_init
#define	pthread_cond_reltimedwait_np	_pthread_cond_reltimedwait_np
#define	pthread_cond_signal		_pthread_cond_signal
#define	pthread_cond_timedwait		_pthread_cond_timedwait
#define	pthread_cond_wait		_pthread_cond_wait
#define	pthread_create			_pthread_create
#define	pthread_detach			_pthread_detach
#define	pthread_equal			_pthread_equal
#define	pthread_exit			_pthread_exit
#define	pthread_getconcurrency		_pthread_getconcurrency
#define	pthread_getschedparam		_pthread_getschedparam
#define	pthread_getspecific		_pthread_getspecific
#define	pthread_join			_pthread_join
#define	pthread_key_create		_pthread_key_create
#define	pthread_key_create_once_np	_pthread_key_create_once_np
#define	pthread_key_delete		_pthread_key_delete
#define	pthread_kill			_pthread_kill
#define	pthread_mutexattr_destroy	_pthread_mutexattr_destroy
#define	pthread_mutexattr_getprioceiling _pthread_mutexattr_getprioceiling
#define	pthread_mutexattr_getprotocol	_pthread_mutexattr_getprotocol
#define	pthread_mutexattr_getpshared	_pthread_mutexattr_getpshared
#define	pthread_mutexattr_getrobust_np	_pthread_mutexattr_getrobust_np
#define	pthread_mutexattr_gettype	_pthread_mutexattr_gettype
#define	pthread_mutexattr_init		_pthread_mutexattr_init
#define	pthread_mutexattr_setprioceiling _pthread_mutexattr_setprioceiling
#define	pthread_mutexattr_setprotocol	_pthread_mutexattr_setprotocol
#define	pthread_mutexattr_setpshared	_pthread_mutexattr_setpshared
#define	pthread_mutexattr_setrobust_np	_pthread_mutexattr_setrobust_np
#define	pthread_mutexattr_settype	_pthread_mutexattr_settype
#define	pthread_mutex_consistent_np	_pthread_mutex_consistent_np
#define	pthread_mutex_destroy		_pthread_mutex_destroy
#define	pthread_mutex_getprioceiling	_pthread_mutex_getprioceiling
#define	pthread_mutex_init		_pthread_mutex_init
#define	pthread_mutex_lock		_pthread_mutex_lock
#define	pthread_mutex_reltimedlock_np	_pthread_mutex_reltimedlock_np
#define	pthread_mutex_setprioceiling	_pthread_mutex_setprioceiling
#define	pthread_mutex_timedlock		_pthread_mutex_timedlock
#define	pthread_mutex_trylock		_pthread_mutex_trylock
#define	pthread_mutex_unlock		_pthread_mutex_unlock
#define	pthread_once			_pthread_once
#define	pthread_rwlockattr_destroy	_pthread_rwlockattr_destroy
#define	pthread_rwlockattr_getpshared	_pthread_rwlockattr_getpshared
#define	pthread_rwlockattr_init		_pthread_rwlockattr_init
#define	pthread_rwlockattr_setpshared	_pthread_rwlockattr_setpshared
#define	pthread_rwlock_destroy		_pthread_rwlock_destroy
#define	pthread_rwlock_init		_pthread_rwlock_init
#define	pthread_rwlock_rdlock		_pthread_rwlock_rdlock
#define	pthread_rwlock_reltimedrdlock_np _pthread_rwlock_reltimedrdlock_np
#define	pthread_rwlock_reltimedwrlock_np _pthread_rwlock_reltimedwrlock_np
#define	pthread_rwlock_timedrdlock	_pthread_rwlock_timedrdlock
#define	pthread_rwlock_timedwrlock	_pthread_rwlock_timedwrlock
#define	pthread_rwlock_tryrdlock	_pthread_rwlock_tryrdlock
#define	pthread_rwlock_trywrlock	_pthread_rwlock_trywrlock
#define	pthread_rwlock_unlock		_pthread_rwlock_unlock
#define	pthread_rwlock_wrlock		_pthread_rwlock_wrlock
#define	pthread_self			_pthread_self
#define	pthread_setcancelstate		_pthread_setcancelstate
#define	pthread_setcanceltype		_pthread_setcanceltype
#define	pthread_setconcurrency		_pthread_setconcurrency
#define	pthread_setschedparam		_pthread_setschedparam
#define	pthread_setschedprio		_pthread_setschedprio
#define	pthread_setspecific		_pthread_setspecific
#define	pthread_sigmask			_pthread_sigmask
#define	pthread_spin_destroy		_pthread_spin_destroy
#define	pthread_spin_init		_pthread_spin_init
#define	pthread_spin_lock		_pthread_spin_lock
#define	pthread_spin_trylock		_pthread_spin_trylock
#define	pthread_spin_unlock		_pthread_spin_unlock
#define	pthread_testcancel		_pthread_testcancel
#define	ptsname				_ptsname
#define	putacct				_putacct
#define	putenv				_putenv
#define	putmsg				_putmsg
#define	putpmsg				_putpmsg
#define	putpwent			_putpwent
#define	putspent			_putspent
#define	pututline			_pututline
#define	pututxline			_pututxline
#define	putw				_putw
#define	pwrite				_pwrite
#define	qeconvert			_qeconvert
#define	qecvt				_qecvt
#define	qfconvert			_qfconvert
#define	qfcvt				_qfcvt
#define	qgconvert			_qgconvert
#define	qgcvt				_qgcvt
#define	quadruple_to_decimal		_quadruple_to_decimal
#define	rand_r				_rand_r
#define	rctlctl				_rctlctl
#define	rctllist			_rctllist
#define	readdir				_readdir
#define	readdir_r			_readdir_r
#define	readlink			_readlink
#define	read				_read
#define	readv				_readv
#define	realpath			_realpath
#define	remque				_remque
#define	resolvepath			_resolvepath
#define	rmdir				_rmdir
#define	rwlock_destroy			_rwlock_destroy
#define	rwlock_init			_rwlock_init
#define	rw_rdlock			_rw_rdlock
#define	rw_read_held			_rw_read_held
#define	rw_tryrdlock			_rw_tryrdlock
#define	rw_trywrlock			_rw_trywrlock
#define	rw_unlock			_rw_unlock
#define	rw_write_held			_rw_write_held
#define	rw_wrlock			_rw_wrlock
#define	sbrk				_sbrk
#define	scandir64			_scandir64
#define	scandir				_scandir
#define	schedctl_exit			_schedctl_exit
#define	schedctl_init			_schedctl_init
#define	schedctl_lookup			_schedctl_lookup
#define	seconvert			_seconvert
#define	seed48				_seed48
#define	seekdir				_seekdir
#define	select				_select
#define	sema_destroy			_sema_destroy
#define	sema_held			_sema_held
#define	sema_init			_sema_init
#define	sema_post			_sema_post
#define	sema_reltimedwait		_sema_reltimedwait
#define	sema_timedwait			_sema_timedwait
#define	sema_trywait			_sema_trywait
#define	sema_wait			_sema_wait
#define	sem_close			_sem_close
#define	semctl64			_semctl64
#define	semctl				_semctl
#define	sem_destroy			_sem_destroy
#define	semget				_semget
#define	sem_getvalue			_sem_getvalue
#define	semids				_semids
#define	sem_init			_sem_init
#define	sem_open			_sem_open
#define	semop				_semop
#define	sem_post			_sem_post
#define	sem_reltimedwait_np		_sem_reltimedwait_np
#define	semtimedop			_semtimedop
#define	sem_timedwait			_sem_timedwait
#define	sem_trywait			_sem_trywait
#define	sem_unlink			_sem_unlink
#define	sem_wait			_sem_wait
#define	setcontext			_setcontext
#define	setegid				_setegid
#define	setenv				_setenv
#define	seteuid				_seteuid
#define	setgid				_setgid
#define	setgrent			_setgrent
#define	setgroups			_setgroups
#define	setitimer			_setitimer
#define	setjmp				_setjmp
#define	setkey				_setkey
#define	setlogmask			_setlogmask
#define	setpflags			_setpflags
#define	setpgid				_setpgid
#define	setpgrp				_setpgrp
#define	setppriv			_setppriv
#define	setpwent			_setpwent
#define	setrctl				_setrctl
#define	setprojrctl			_setprojrctl
#define	setregid			_setregid
#define	setreuid			_setreuid
#define	setrlimit			_setrlimit
#define	setsid				_setsid
#define	setspent			_setspent
#define	settaskid			_settaskid
#define	settimeofday			_settimeofday
#define	setuid				_setuid
#define	setustack			_setustack
#define	setutent			_setutent
#define	setutxent			_setutxent
#define	sfconvert			_sfconvert
#define	sgconvert			_sgconvert
#define	shmat				_shmat
#define	shmctl64			_shmctl64
#define	shmctl				_shmctl
#define	shmdt				_shmdt
#define	shmget				_shmget
#define	shmids				_shmids
#define	sig2str				_sig2str
#define	sigaction			_sigaction
#define	sigaddset			_sigaddset
#define	sigaltstack			_sigaltstack
#define	sigdelset			_sigdelset
#define	sigemptyset			_sigemptyset
#define	sigfillset			_sigfillset
#define	sigflag				_sigflag
#define	sigfpe				_sigfpe
#define	sighold				_sighold
#define	sigignore			_sigignore
#define	siginterrupt			_siginterrupt
#define	sigismember			_sigismember
#define	siglongjmp			_siglongjmp
#define	signal				_signal
#define	sigpause			_sigpause
#define	sigpending			_sigpending
#define	sigprocmask			_sigprocmask
#define	sigqueue			_sigqueue
#define	sigrelse			_sigrelse
#define	sigsendset			_sigsendset
#define	sigsend				_sigsend
#define	sigsetjmp			_sigsetjmp
#define	sigset				_sigset
#define	sigstack			_sigstack
#define	sigsuspend			_sigsuspend
#define	sigtimedwait			_sigtimedwait
#define	sigwaitinfo			_sigwaitinfo
#define	sigwait				_sigwait
#define	single_to_decimal		_single_to_decimal
#define	sleep				_sleep
#define	srand48				_srand48
#define	ssignal				_ssignal
#define	stack_getbounds			_stack_getbounds
#define	stack_inbounds			_stack_inbounds
#define	stack_setbounds			_stack_setbounds
#define	stack_violation			_stack_violation
#define	statfs				_statfs
#define	statvfs				_statvfs
#define	stime				_stime
#define	str2sig				_str2sig
#define	strdup				_strdup
#define	strerror_r			_strerror_r
#define	strerror			_strerror
#define	string_to_decimal		_string_to_decimal
#define	strsignal			_strsignal
#define	strtok_r			_strtok_r
#define	strtoll				_strtoll
#define	strtoull			_strtoull
#define	stty				_stty
#define	swab				_swab
#define	swapcontext			_swapcontext
#define	swapctl				_swapctl
#define	symlink				_symlink
#define	sync				_sync
#define	syscall				_syscall
#define	sysconf				_sysconf
#define	sysfs				_sysfs
#define	sysinfo				_sysinfo
#define	syslog				_syslog
#define	tcdrain				_tcdrain
#define	tcflow				_tcflow
#define	tcflush				_tcflush
#define	tcgetattr			_tcgetattr
#define	tcgetpgrp			_tcgetpgrp
#define	tcgetsid			_tcgetsid
#define	tcsendbreak			_tcsendbreak
#define	tcsetattr			_tcsetattr
#define	tcsetpgrp			_tcsetpgrp
#define	tdelete				_tdelete
#define	telldir				_telldir
#define	tell				_tell
#define	tempnam				_tempnam
#define	tfind				_tfind
#define	thr_continue_allmutators	_thr_continue_allmutators
#define	thr_continue_mutator		_thr_continue_mutator
#define	thr_continue			_thr_continue
#define	thr_create			_thr_create
#define	thr_exit			_thr_exit
#define	thr_getconcurrency		_thr_getconcurrency
#define	thr_getprio			_thr_getprio
#define	thr_getspecific			_thr_getspecific
#define	thr_getstate			_thr_getstate
#define	thr_join			_thr_join
#define	thr_keycreate			_thr_keycreate
#define	thr_keycreate_once		_thr_keycreate_once
#define	thr_kill			_thr_kill
#define	thr_main			_thr_main
#define	thr_min_stack			_thr_min_stack
#define	thr_mutators_barrier		_thr_mutators_barrier
#define	thr_self			_thr_self
#define	thr_setconcurrency		_thr_setconcurrency
#define	thr_setmutator			_thr_setmutator
#define	thr_setprio			_thr_setprio
#define	thr_setspecific			_thr_setspecific
#define	thr_setstate			_thr_setstate
#define	thr_sighndlrinfo		_thr_sighndlrinfo
#define	thr_sigsetmask			_thr_sigsetmask
#define	thr_stksegment			_thr_stksegment
#define	thr_suspend_allmutators		_thr_suspend_allmutators
#define	thr_suspend_mutator		_thr_suspend_mutator
#define	thr_suspend			_thr_suspend
#define	thr_wait_mutator		_thr_wait_mutator
#define	thr_yield			_thr_yield
#define	timer_create			_timer_create
#define	timer_delete			_timer_delete
#define	timer_getoverrun		_timer_getoverrun
#define	timer_gettime			_timer_gettime
#define	timer_settime			_timer_settime
#define	times				_times
#define	time				_time
#define	tmpnam_r			_tmpnam_r
#define	tmpnam				_tmpnam
#define	truncate			_truncate
#define	tsearch				_tsearch
#define	ttyname_r			_ttyname_r
#define	ttyname				_ttyname
#define	ttyslot				_ttyslot
#define	twalk				_twalk
#define	tzset				_tzset
#define	uadmin				_uadmin
#define	ucred_free			_ucred_free
#define	ucred_getamask			_ucred_getamask
#define	ucred_getasid			_ucred_getasid
#define	ucred_getatid			_ucred_getatid
#define	ucred_getauid			_ucred_getauid
#define	ucred_getegid			_ucred_getegid
#define	ucred_geteuid			_ucred_geteuid
#define	ucred_getgroups			_ucred_getgroups
#define	ucred_getpflags			_ucred_getpflags
#define	ucred_getpid			_ucred_getpid
#define	ucred_getprivset		_ucred_getprivset
#define	ucred_getprojid			_ucred_getprojid
#define	ucred_getrgid			_ucred_getrgid
#define	ucred_getruid			_ucred_getruid
#define	ucred_getsgid			_ucred_getsgid
#define	ucred_getsuid			_ucred_getsuid
#define	ucred_get			_ucred_get
#define	ucred_getzoneid			_ucred_getzoneid
#define	ucred_size			_ucred_size
#define	ulckpwdf			_ulckpwdf
#define	ulimit				_ulimit
#define	ulltostr			_ulltostr
#define	umask				_umask
#define	umount2				_umount2
#define	umount				_umount
#define	unlinkat			_unlinkat
#define	unlink				_unlink
#define	unlockpt			_unlockpt
#define	unordered			_unordered
#define	unsetenv			_unsetenv
#define	updwtmp				_updwtmp
#define	updwtmpx			_updwtmpx
#define	usleep				_usleep
#define	ustat				_ustat
#define	utimes				_utimes
#define	utime				_utime
#define	utmpname			_utmpname
#define	utmpxname			_utmpxname
#define	utssys				_utssys
#define	vfork				_vfork
#define	vforkx				_vforkx
#define	vfscanf				_vfscanf
#define	vhangup				_vhangup
#define	vscanf				_vscanf
#define	vsscanf				_vsscanf
#define	vsyslog				_vsyslog
#define	wait3				_wait3
#define	waitid				_waitid
#define	waitpid				_waitpid
#define	wait				_wait
#define	walkcontext			_walkcontext
#define	wctomb				_wctomb
#define	wracct				_wracct
#define	writev				_writev
#define	write				_write
#define	yield				_yield

/* accommodation for _*xstat() and _nuname() interfaces */
#if !defined(__i386) || _FILE_OFFSET_BITS == 64 || defined(_LP64)
#define	fstat				_fstat
#define	lstat				_lstat
#define	mknod				_mknod
#define	stat				_stat
#endif
#if !defined(__i386)
#define	uname				_uname
#endif

/* transitional large file and other 32-bit only interfaces */
#if !defined(_LP64)
#define	attropen64			_attropen64
#define	creat64				_creat64
#define	fstat64				_fstat64
#define	fstatat64			_fstatat64
#define	fstatvfs64			_fstatvfs64
#define	ftruncate64			_ftruncate64
#define	ftw64				_ftw64
#define	getdents64			_getdents64
#define	getrlimit64			_getrlimit64
#define	lockf64				_lockf64
#define	lseek64				_lseek64
#define	lstat64				_lstat64
#define	mkstemp64			_mkstemp64
#define	mmap64				_mmap64
#define	nftw64				_nftw64
#define	open64				_open64
#define	openat64			_openat64
#define	pread64				_pread64
#define	pwrite64			_pwrite64
#define	readdir64			_readdir64
#define	readdir64_r			_readdir64_r
#define	setrlimit64			_setrlimit64
#define	stat64				_stat64
#define	statvfs64			_statvfs64
#define	tell64				_tell64
#define	truncate64			_truncate64
#define	ptrace				_ptrace
#define	s_fcntl				_s_fcntl
#define	sys_errlist			_sys_errlist
#define	sys_nerr			_sys_nerr
#endif	/* !_LP64 */

/* 64-bit only interfaces */
#if defined(_LP64)
#define	getc_unlocked			_getc_unlocked
#define	putc_unlocked			_putc_unlocked
#endif	/* _LP64 */

#endif	/* !defined(__lint) */

#ifdef __cplusplus
}
#endif

#endif /* _COMMON_INC_SYNONYMS_H */
