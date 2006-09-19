#!/sbin/sh
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#
# Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#pragma ident	"%Z%%M%	%I%	%E% SMI"
#
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved
#
#

usage () {
	if [ -n "$1" ]; then
		echo "umountall: $1" 1>&2
	fi
	echo "Usage:\n\tumountall [-k] [-s] [-F FSType] [-l|-r] [-n]" 1>&2
	echo "\tumountall [-k] [-s] [-h host] [-n]" 1>&2
	exit 2
}

MNTTAB=/etc/mnttab

# This script is installed as both /sbin/umountall (as used in some
# /sbin/rc? and /etc/init.d scripts) _and_ as /usr/sbin/umountall (typically
# PATHed from the command line).  As such it should not depend on /usr
# being mounted (if /usr is a separate filesystem).
#
# /sbin/sh Bourne shell builtins we use:
#	echo
#	exit
#	getopts
#	test, [ ]
#	exec
#	read
#
# /sbin commands we use:
#	/sbin/uname
#	/sbin/umount
#
# The following /usr based commands may be used by this script (depending on
# command line options).  We will set our PATH to find them, but where they
# are not present (eg, if /usr is not mounted) we will catch references to
# them via shell functions conditionally defined after option processing
# (don't use any of these commands before then).
#
#	Command		Command line option and use
# /usr/bin/sleep	-k, to sleep after an fuser -c -k on the mountpoint
# /usr/sbin/fuser	-k, to kill processes keeping a mount point busy
#
# In addition, we use /usr/bin/tail if it is available; if not we use
# slower shell constructs to reverse a file.

PATH=/sbin:/usr/sbin:/usr/bin

# Clear these in case they were already set in our inherited environment.
FSType=
FFLAG=
HOST=
HFLAG=
RFLAG=
LFLAG=
SFLAG=
KFLAG=
NFLAG=
LOCALNAME=
UMOUNTFLAG=


while getopts ?rslkF:h:n c
do
	case $c in
	r)	RFLAG="r";;
	l)	LFLAG="l";;
	s)	SFLAG="s";;
	k) 	KFLAG="k";;
	h)	if [ -n "$HFLAG" ]; then
			usage "more than one host specified"
		fi
		HOST=$OPTARG
		HFLAG="h"
		LOCALNAME=`uname -n`
		;; 
	F)	if [ -n "$FFLAG" ]; then
			usage "more than one FStype specified"
		fi
		FSType=$OPTARG 
		FFLAG="f"
		case $FSType in
		?????????*) 
			usage "FSType ${FSType} exceeds 8 characters"
		esac;
		;;
	n)	NFLAG="n"
		# Alias any commands that would perform real actions to
		# something that tells what action would have been performed
		UMOUNTFLAG="-V"
		fuser () {
			echo "fuser $*" 1>&2
		}
		sleep () {
			: # No need to show where we'd sleep
		}
		;;
	\?)	usage ""
		;;
	esac
done

# Sanity checking:
#	1) arguments beyond those supported
#	2) can't specify both remote and local
#	3) can't specify a host with -r or -l
#	4) can't specify a fstype with -h
#	5) can't specify this host with -h (checks only uname -n)
#	6) can't be fstype nfs and local
#	7) only fstype nfs is remote

if [ $# -ge $OPTIND ]; then						# 1
	usage "additional arguments not supported"
fi

if [ -n "$RFLAG" -a -n "$LFLAG" ]; then					# 2
	usage "options -r and -l are incompatible"
fi

if [ \( -n "$RFLAG" -o -n "$LFLAG" \) -a "$HFLAG" = "h" ]; then		# 3
	usage "option -${RFLAG}${LFLAG} incompatible with -h option"
fi

if [ -n "$FFLAG" -a "$HFLAG" = "h" ]; then				# 4
	usage "Specifying FStype incompatible with -h option"
fi

if [ -n "$HFLAG" -a "$HOST" = "$LOCALNAME" ]; then			# 5
	usage "Specifying local host illegal for -h option"
fi

if [ "$FSType" = "nfs" -a "$LFLAG" = "l" ]; then			# 6
	usage "option -l and FSType nfs are incompatible"
fi

if [ -n "$FFLAG" -a "$FSType" != "nfs"  -a -n "$RFLAG" ]; then		# 7
	usage "option -r and FSType ${FSType} are incompatible"
fi

ZONENAME=`zonename`

# Check and if needed sync the boot archive before unmounting everything.
#
if [ -z "${RFLAG}${NFLAG}${HFLAG}${FSType}" -a "$ZONENAME" = "global" -a \
    `uname -p` = "i386" -a -x /sbin/bootadm ] ; then
	/sbin/bootadm -a update_all
fi

#
# Take advantage of parallel unmounting at this point if we have no
# criteria to match and we are in the global zone
#
if [ -z "${SFLAG}${LFLAG}${RFLAG}${HFLAG}${KFLAG}${FFLAG}" -a \
    "$ZONENAME" = "global" ]; then
	umount -a ${UMOUNTFLAG}
	exit			# with return code of the umount -a
fi

#
# Catch uses of /usr commands when /usr is not mounted
if [ -n "$KFLAG" -a -z "$NFLAG" ]; then
	if [ ! -x /usr/sbin/fuser ]; then
		fuser () {
			echo "umountall: fuser -k skipped (no /usr)" 1>&2
			# continue - not fatal
		}
		sleep () {
			: # no point in sleeping if fuser is doing nothing
		}
	else
		if [ ! -x /usr/bin/sleep ]; then
			sleep () {
				echo "umountall: sleep after fuser -k skipped (no /usr)" 1>&2
				# continue - not fatal
			}
		fi
	fi
fi

#
# Shell function to avoid using /usr/bin/cut.  Given a dev from a
# fstype=nfs line in mnttab (eg, "host:/export) extract the host
# component.
print_host () {
	OIFS=$IFS
	IFS=":"
	set -- $*
	echo $1
	IFS=$OIFS
}

#
# doumounts echos its return code to stdout, so commands used within
# this function should take care to produce no other output to stdout.
doumounts () {
	(
	rc=0
	fslist=""
	while read dev mountp fstype mode dummy
	do
		case "${mountp}" in
		/			| \
		/dev			| \
		/dev/fd			| \
		/devices		| \
		/etc/mnttab		| \
		/etc/svc/volatile	| \
		/lib			| \
		/proc			| \
		/sbin			| \
		/system/contract	| \
		/system/object		| \
		/tmp			| \
		/usr			| \
		/var			| \
		/var/adm		| \
		/var/run		| \
		'' )
			#
			# file systems possibly mounted in the kernel or
			# in the methods of some of the file system
			# services
			#
			continue
			;;
		* )
			if [ -n "$HFLAG" ]; then
				if [ "$fstype" = "nfs" ]; then
					thishost=`print_host $dev`
					if [ "$HOST" != "$thishost" ]; then
						continue
					fi
				else
					continue
				fi
			fi
			if [ -n "$FFLAG" -a "$FSType" != "$fstype" ]; then
				continue
			fi
			if [ -n "$LFLAG" -a "$fstype" = "nfs" ]; then
				continue
			fi
			if [ -n "$RFLAG" -a "$fstype" != "nfs" ]; then
				continue
			fi
			if [ "$ZONENAME" != "global" ]; then
				for option in `echo $mode | tr , '\012'`; do
					#
					# should not see any zone options
					# but our own
					#
					if [ "$option" = "zone=$ZONENAME" ]; then
						break
					fi
				done
				if [ "$option" != "zone=$ZONENAME" ]; then
					continue
				fi
			fi
			if [ -n "${KFLAG}" ]; then
				fuser -c -k $mountp 1>&2
				sleep 2
			fi
			if [ -n "$SFLAG" ]; then
				umount ${UMOUNTFLAG} ${mountp} 1>&2
				trc=$?
				if [ $trc -ne 0 ]; then
					rc=$trc
				fi
			else
				# We want to umount in parallel
				fslist="$fslist $mountp"
			fi
		esac
	done

	if [ -n "$fslist" ]; then
		umount -a ${UMOUNTFLAG} $fslist 1>&2
		trc=$?
		if [ $trc -ne 0 ]; then
			rc=$trc
		fi
	fi

	echo $rc
	)
}

#
# /etc/mnttab has the most recent mounts last.  Reverse it so that we
# may umount in opposite order to the original mounts.
#

if [ ! -x /usr/bin/tail ]; then
	exec < $MNTTAB
	REVERSED=
	while read line; do
		if [ -n "$REVERSED" ]; then
        		REVERSED="$line\n$REVERSED"
		else
			REVERSED="$line"
		fi
	done

	error=`echo $REVERSED | doumounts`
else
	error=`tail -r $MNTTAB | doumounts`
fi

exit $error
