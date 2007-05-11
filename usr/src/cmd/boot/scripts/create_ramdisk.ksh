#!/bin/ksh -p
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

# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.

# ident	"%Z%%M%	%I%	%E% SMI"

format=ufs
ALT_ROOT=
compress=yes
SPLIT=unknown
ERROR=0
dirsize32=0
dirsize64=0

BOOT_ARCHIVE=platform/i86pc/boot_archive
BOOT_ARCHIVE_64=platform/i86pc/amd64/boot_archive

export PATH=$PATH:/usr/sbin:/usr/bin:/sbin

#
# Parse options
#
while [ "$1" != "" ]
do
        case $1 in
        -R)	shift
		ALT_ROOT="$1"
		if [ "$ALT_ROOT" != "/" ]; then
			echo "Creating ram disk for $ALT_ROOT"
		fi
		;;
	-n|--nocompress) compress=no
		;;
        *)      echo Usage: ${0##*/}: [-R \<root\>] [--nocompress]
		exit
		;;
        esac
	shift
done

if [ -x /usr/bin/mkisofs -o -x /tmp/bfubin/mkisofs ] ; then
	format=isofs
fi

#
# mkisofs on s8 doesn't support functionality used by GRUB boot.
# Use ufs format for boot archive instead.
#
release=`uname -r`
if [ "$release" = "5.8" ]; then
	format=ufs
fi

shift `expr $OPTIND - 1`

if [ $# -eq 1 ]; then
	ALT_ROOT="$1"
	echo "Creating ram disk for $ALT_ROOT"
fi

rundir=`dirname $0`
if [ ! -x "$rundir"/symdef ]; then
	# Shouldn't happen
	echo "Warning: $rundir/symdef not present."
	echo "Creating single archive at $ALT_ROOT/platform/i86pc/boot_archive"
	SPLIT=no
	compress=no
elif "$rundir"/symdef "$ALT_ROOT"/platform/i86pc/kernel/unix \
    dboot_image 2>/dev/null; then
	SPLIT=yes
else
	SPLIT=no
	compress=no
fi

[ -x /usr/bin/gzip ] || compress=no

function cleanup
{
	umount -f "$rdmnt32" 2>/dev/null
	umount -f "$rdmnt64" 2>/dev/null
	lofiadm -d "$rdfile32" 2>/dev/null
	lofiadm -d "$rdfile64" 2>/dev/null
	[ -n "$rddir" ] && rm -fr "$rddir" 2> /dev/null
	[ -n "$new_rddir" ] && rm -fr "$new_rddir" 2>/dev/null
}

function getsize
{
	# Estimate image size and add 10% overhead for ufs stuff.
	# Note, we can't use du here in case we're on a filesystem, e.g. zfs,
	# in which the disk usage is less than the sum of the file sizes.
	# The nawk code 
	#
	#	{t += ($5 % 1024) ? (int($5 / 1024) + 1) * 1024 : $5}
	#
	# below rounds up the size of a file/directory, in bytes, to the
	# next multiple of 1024.  This mimics the behavior of ufs especially
	# with directories.  This results in a total size that's slightly
	# bigger than if du was called on a ufs directory.
	size32=$(cat "$list32" | xargs -I {} ls -lLd "{}" | nawk '
		{t += ($5 % 1024) ? (int($5 / 1024) + 1) * 1024 : $5}
		END {print int(t * 1.10 / 1024)}')
	(( size32 += dirsize32 ))
	size64=$(cat "$list64" | xargs -I {} ls -lLd "{}" | nawk '
		{t += ($5 % 1024) ? (int($5 / 1024) + 1) * 1024 : $5}
		END {print int(t * 1.10 / 1024)}')
	(( size64 += dirsize64 ))
	(( total_size = size32 + size64 ))
}

#
# Copies all desired files to a target directory.  One argument should be
# passed: the file containing the list of files to copy.  This function also
# depends on several variables that must be set before calling:
#
# $ALT_ROOT - the target directory
# $compress - whether or not the files in the archives should be compressed
# $rdmnt - the target directory
#
function copy_files
{
	list="$1"

	#
	# If compress is set, the files are gzip'd and put in the correct
	# location in the loop.  Nothing is printed, so the pipe and cpio
	# at the end is a nop.
	#
	# If compress is not set, the file names are printed, which causes
	# the cpio at the end to do the copy.
	#
	while read path
	do
		if [ $compress = yes ]; then
			dir="${path%/*}"
			mkdir -p "$rdmnt/$dir"
			/usr/bin/gzip -c "$path" > "$rdmnt/$path"
		else
			print "$path"
		fi
	done <"$list" | cpio -pdum "$rdmnt" 2>/dev/null
}

#
# The first argument can be:
#
# "both" - create an archive with both 32-bit and 64-bit binaries
# "32-bit" - create an archive with only 32-bit binaries
# "64-bit" - create an archive with only 64-bit binaries
#
function create_ufs
{
	which=$1
	archive=$2
	lofidev=$3

	# should we exclude amd64 binaries?
	if [ "$which" = "32-bit" ]; then
		rdfile="$rdfile32"
		rdmnt="$rdmnt32"
		list="$list32"
	elif [ "$which" = "64-bit" ]; then
		rdfile="$rdfile64"
		rdmnt="$rdmnt64"
		list="$list64"
	else
		rdfile="$rdfile32"
		rdmnt="$rdmnt32"
		list="$list32"
	fi

	newfs $lofidev < /dev/null 2> /dev/null
	mkdir "$rdmnt"
	mount -F mntfs mnttab /etc/mnttab > /dev/null 2>&1
	mount -o nologging $lofidev "$rdmnt"
	files=

	# do the actual copy
	copy_files "$list"
	umount "$rdmnt"
	rmdir "$rdmnt"

	#
	# Check if gzip exists in /usr/bin, so we only try to run gzip
	# on systems that have gzip. Then run gzip out of the patch to
	# pick it up from bfubin or something like that if needed.
	#
	# If compress is set, the individual files in the archive are
	# compressed, and the final compression will accomplish very
	# little.  To save time, we skip the gzip in this case.
	#
	if [ $compress = no ] && [ -x /usr/bin/gzip ] ; then
		gzip -c "$rdfile" > "${archive}-new"
	else
		cat "$rdfile" > "${archive}-new"
	fi
}

#
# The first argument can be:
#
# "both" - create an archive with both 32-bit and 64-bit binaries
# "32-bit" - create an archive with only 32-bit binaries
# "64-bit" - create an archive with only 64-bit binaries
#
function create_isofs
{
	which=$1
	archive=$2

	# should we exclude amd64 binaries?
	if [ "$which" = "32-bit" ]; then
		rdmnt="$rdmnt32"
		errlog="$errlog32"
		list="$list32"
	elif [ "$which" = "64-bit" ]; then
		rdmnt="$rdmnt64"
		errlog="$errlog64"
		list="$list64"
	else
		rdmnt="$rdmnt32"
		errlog="$errlog32"
		list="$list32"
	fi

	# create image directory seed with graft points
	mkdir "$rdmnt"
	files=
	isocmd="mkisofs -quiet -graft-points -dlrDJN -relaxed-filenames"

	copy_files "$list"
	isocmd="$isocmd \"$rdmnt\""
	rm -f "$errlog"

	#
	# Check if gzip exists in /usr/bin, so we only try to run gzip
	# on systems that have gzip. Then run gzip out of the patch to
	# pick it up from bfubin or something like that if needed.
	#
	# If compress is set, the individual files in the archive are
	# compressed, and the final compression will accomplish very
	# little.  To save time, we skip the gzip in this case.
	#
	if [ $compress = no ] && [ -x /usr/bin/gzip ] ; then
		ksh -c "$isocmd" 2> "$errlog" | \
		    gzip > "${archive}-new"
	else
		ksh -c "$isocmd" 2> "$errlog" > "${archive}-new"
	fi

	if [ -s "$errlog" ]; then
		grep Error: "$errlog" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			grep Error: "$errlog"
			rm -f "${archive}-new"
		fi
	fi
	rm -f "$errlog"
}

function create_archive
{
	which=$1
	archive=$2
	lofidev=$3

	echo "updating $archive...this may take a minute"

	if [ "$format" = "ufs" ]; then
		create_ufs "$which" "$archive" "$lofidev"
	else
		create_isofs "$which" "$archive"
	fi

	# sanity check the archive before moving it into place
	#
	ARCHIVE_SIZE=`ls -l "${archive}-new" | nawk '{ print $5 }'`
	if [ $compress = yes ]
	then
		#
		# 'file' will report "English text" for uncompressed
		# boot_archives.  Checking for that doesn't seem stable,
		# so we just check that the file exists.
		#
		ls "${archive}-new" >/dev/null 2>&1
	else
		#
		# the file type check also establishes that the
		# file exists at all
		#
		LC_MESSAGES=C file "${archive}-new" | grep gzip > /dev/null
	fi

	if [ $? = 1 ] && [ -x /usr/bin/gzip ] || [ $ARCHIVE_SIZE -lt 5000 ]
	then
		#
		# Two of these functions may be run in parallel.  We
		# need to allow the other to clean up, so we can't
		# exit immediately.  Instead, we set a flag.
		#
		echo "update of $archive failed"
		ERROR=1
	else
		lockfs -f "/$ALT_ROOT" 2>/dev/null
		mv "${archive}-new" "$archive"
		lockfs -f "/$ALT_ROOT" 2>/dev/null
	fi

}

function fatal_error
{
	print -u2 $*
	exit 1
}

#
# get filelist
#
if [ ! -f "$ALT_ROOT/boot/solaris/filelist.ramdisk" ] &&
    [ ! -f "$ALT_ROOT/etc/boot/solaris/filelist.ramdisk" ]
then
	print -u2 "Can't find filelist.ramdisk"
	exit 1
fi
filelist=$(cat "$ALT_ROOT/boot/solaris/filelist.ramdisk" \
    "$ALT_ROOT/etc/boot/solaris/filelist.ramdisk" 2>/dev/null | sort -u)

#
# We use /tmp/ for scratch space now.  This may be changed later if there
# is insufficient space in /tmp/.
#
rddir="/tmp/create_ramdisk.$$.tmp"
new_rddir=
rm -rf "$rddir"
mkdir "$rddir" || fatal_error "Could not create temporary directory $rddir"

# Clean up upon exit.
trap 'cleanup' EXIT

list32="$rddir/filelist.32"
list64="$rddir/filelist.64"

#
# This loop creates the 32-bit and 64-bit lists of files.  The 32-bit list
# is written to stdout, which is redirected at the end of the loop.  The
# 64-bit list is appended with each write.
#
cd "/$ALT_ROOT"
find $filelist -print 2>/dev/null | while read path
do
	if [ $SPLIT = no ]; then
		print "$path"
	elif [ -d "$path" ]; then
		if [ $format = ufs ]; then
			size=`ls -lLd "$path" | nawk '
			    {print ($5 % 1024) ? (int($5 / 1024) + 1) * 1024 : $5}'`
			if [ `basename "$path"` != "amd64" ]; then
				(( dirsize32 += size ))
			fi
			(( dirsize64 += size ))
		fi
	else
		filetype=`LC_MESSAGES=C file "$path" 2>/dev/null |\
		    awk '/ELF/ { print $3 }'`
		if [ "$filetype" = "64-bit" ]; then
			print "$path" >> "$list64"
		elif [ "$filetype" = "32-bit" ]; then
			print "$path"
		else
			# put in both lists
			print "$path"
			print "$path" >> "$list64"
		fi
	fi
done >"$list32"

if [ $format = ufs ] ; then
	# calculate image size
	getsize

	# check to see if there is sufficient space in tmpfs 
	#
	tmp_free=`df -b /tmp | tail -1 | awk '{ printf ($2) }'`
	(( tmp_free = tmp_free / 2 ))

	if [ $total_size -gt $tmp_free  ] ; then
		# assumes we have enough scratch space on $ALT_ROOT
		new_rddir="/$ALT_ROOT/create_ramdisk.$$.tmp"
		rm -rf "$new_rddir"
		mkdir "$new_rddir" || fatal_error \
		    "Could not create temporary directory $new_rddir"

		# Save the file lists
		mv "$list32" "$new_rddir"/
		mv "$list64" "$new_rddir"/
		list32="/$new_rddir/filelist.32"
		list64="/$new_rddir/filelist.64"

		# Remove the old $rddir and set the new value of rddir
		rm -rf "$rddir"
		rddir="$new_rddir"
		new_rddir=
	fi
fi

rdfile32="$rddir/rd.file.32"
rdfile64="$rddir/rd.file.64"
rdmnt32="$rddir/rd.mount.32"
rdmnt64="$rddir/rd.mount.64"
errlog32="$rddir/rd.errlog.32"
errlog64="$rddir/rd.errlog.64"
lofidev32=""
lofidev64=""

if [ $SPLIT = yes ]; then
	#
	# We can't run lofiadm commands in parallel, so we have to do
	# them here.
	#
	if [ "$format" = "ufs" ]; then
		mkfile ${size32}k "$rdfile32"
		lofidev32=`lofiadm -a "$rdfile32"`
		mkfile ${size64}k "$rdfile64"
		lofidev64=`lofiadm -a "$rdfile64"`
	fi
	create_archive "32-bit" "$ALT_ROOT/$BOOT_ARCHIVE" $lofidev32 &
	create_archive "64-bit" "$ALT_ROOT/$BOOT_ARCHIVE_64" $lofidev64
	wait
	if [ "$format" = "ufs" ]; then
		lofiadm -d "$rdfile32"
		lofiadm -d "$rdfile64"
	fi
else
	if [ "$format" = "ufs" ]; then
		mkfile ${total_size}k "$rdfile32"
		lofidev32=`lofiadm -a "$rdfile32"`
	fi
	create_archive "both" "$ALT_ROOT/$BOOT_ARCHIVE" $lofidev32
	[ "$format" = "ufs" ] && lofiadm -d "$rdfile32"
fi
if [ $ERROR = 1 ]; then
	cleanup
	exit 1
fi

#
# For the diskless case, hardlink archive to /boot to make it
# visible via tftp. /boot is lofs mounted under /tftpboot/<hostname>.
# NOTE: this script must work on both client and server.
#
grep "[	 ]/[	 ]*nfs[	 ]" "$ALT_ROOT/etc/vfstab" > /dev/null
if [ $? = 0 ]; then
	rm -f "$ALT_ROOT/boot/boot_archive" "$ALT_ROOT/boot/amd64/boot_archive"
	ln "$ALT_ROOT/$BOOT_ARCHIVE" "$ALT_ROOT/boot/boot_archive"
	ln "$ALT_ROOT/$BOOT_ARCHIVE_64" "$ALT_ROOT/boot/amd64/boot_archive"
fi
[ -n "$rddir" ] && rm -rf "$rddir"
