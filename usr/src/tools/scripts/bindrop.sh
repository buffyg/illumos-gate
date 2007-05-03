#! /usr/bin/ksh -p
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
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#ident	"%Z%%M%	%I%	%E% SMI"
#
# Create an encumbered binaries tarball from a full build proto area,
# less the contents of an OpenSolaris proto area.  Special handling
# for crypto binaries that need to be signed by Sun Release
# Engineering.
#

usage="bindrop [-n] full-root open-root"

isa=`uname -p`
if [ $isa = sparc ]; then
	isa_short=s
else
	isa_short=x
fi

#
# Netinstall server and path to RE-signed packages (for crypto).
# It might be better to mechanically derive the "Solaris_11" part of
# the path from RELEASE.  But it's not clear that such a derivation
# would be reliable.
#
nisrv=${NISRV:-jurassic.eng}
nipath=/net/$nisrv/export/ni-2/nv/${isa_short}/latest/Solaris_11/Product

# URL to Encryption Kit binaries.
ekurl=http://nana.eng/pub/nv

PATH="$PATH:/usr/bin:/usr/sfw/bin"

fail() {
	echo $*
	exit 1
}

[ -n "$SRC" ] || fail "Please set SRC."
[ -n "$CODEMGR_WS" ] || fail "Please set CODEMGR_WS."
[ -d $nipath ] || fail "Can't find RE-signed packages ($nipath)."

#
# Create the README from boilerplate and the contents of the closed
# binary tree.
#
# usage: mkreadme targetdir
#
mkreadme () {
	targetdir=$1
	readme=README.ON-BINARIES.$isa
	sed -e s/@ISA@/$isa/ -e s/@DELIVERY@/ON-BINARIES/ \
	    $SRC/tools/opensolaris/README.binaries.tmpl > $targetdir/$readme
	(cd $targetdir; find $dir -type f -print | \
	    sort >> $targetdir/$readme)
}

nondebug=n
while getopts n flag; do
	case $flag in
	n)
		nondebug=y
		;;
	?)
		echo "usage: $usage"
		exit 1
		;;
	esac
done
shift $(($OPTIND - 1))

[ $# -eq 2 ] || fail "usage: $usage"

full=$1
open=$2

dir=root_$isa
if [ $nondebug = y ]; then
	dir=root_$isa-nd
fi

[ -d "$full" ] || fail "bindrop: can't find $full."
[ -d "$open" ] || fail "bindrop: can't find $open."

tmpdir=$(mktemp -dt bindropXXXXX)
[ -n "$tmpdir" ] || fail "Can't create temporary directory."
mkdir -p $tmpdir/closed/$dir || exit 1

#
# This will hold a temp list of directories that must be kept, even if
# empty.
#
needdirs=$(mktemp -t needdirsXXXXX)
[ -n "$needdirs" ] || fail "Can't create temporary directory list file."

#
# Copy the full tree into a temp directory.
#
(cd $full; tar cf - .) | (cd $tmpdir/closed/$dir; tar xpf -)

#
# Remove internal ON crypto signing certs
#
delete="
	etc/certs/SUNWosnetSE
	etc/certs/SUNWosnetSolaris
	etc/crypto/certs/SUNWosnet
	etc/crypto/certs/SUNWosnetLimited
	etc/crypto/certs/SUNWosnetCF
	etc/crypto/certs/SUNWosnetCFLimited
	"

#
# Remove miscellaneous files that we don't want to ship.
#

# SUNWsvvs (SVVS test drivers).
delete="$delete
	usr/include/sys/lo.h
	usr/include/sys/tidg.h
	usr/include/sys/tivc.h
	usr/include/sys/tmux.h
	usr/kernel/drv/amd64/lo
	usr/kernel/drv/amd64/tidg
	usr/kernel/drv/amd64/tivc
	usr/kernel/drv/amd64/tmux
	usr/kernel/drv/lo
	usr/kernel/drv/lo.conf
	usr/kernel/drv/sparcv9/lo
	usr/kernel/drv/sparcv9/tidg
	usr/kernel/drv/sparcv9/tivc
	usr/kernel/drv/sparcv9/tmux
	usr/kernel/drv/tidg
	usr/kernel/drv/tidg.conf
	usr/kernel/drv/tivc
	usr/kernel/drv/tivc.conf
	usr/kernel/drv/tmux
	usr/kernel/drv/tmux.conf
	usr/kernel/strmod/amd64/lmodb
	usr/kernel/strmod/amd64/lmode
	usr/kernel/strmod/amd64/lmodr
	usr/kernel/strmod/amd64/lmodt
	usr/kernel/strmod/lmodb
	usr/kernel/strmod/lmode
	usr/kernel/strmod/lmodr
	usr/kernel/strmod/lmodt
	usr/kernel/strmod/sparcv9/lmodb
	usr/kernel/strmod/sparcv9/lmode
	usr/kernel/strmod/sparcv9/lmodr
	usr/kernel/strmod/sparcv9/lmodt
"
# encumbered binaries and associated files
delete="$delete
	etc/dmi/
	etc/smartcard/
	kernel/drv/amd64/audioens
	kernel/drv/amd64/pcn
	kernel/drv/audioens
	kernel/drv/audioens.conf
	kernel/drv/ifp.conf
	kernel/drv/pcn
	kernel/drv/pcn.conf
	kernel/drv/sparcv9/audioens
	kernel/drv/sparcv9/ifp
	kernel/drv/sparcv9/isp
	kernel/drv/spwr
	kernel/drv/spwr.conf
	kernel/kmdb/sparcv9/isp
	kernel/misc/amd64/phx
	kernel/misc/phx
	kernel/misc/sparcv9/phx
	platform/SUNW,Sun-Blade-100/kernel/drv/grppm.conf
	platform/SUNW,Sun-Blade-100/kernel/drv/sparcv9/grfans
	platform/SUNW,Sun-Blade-100/kernel/drv/sparcv9/grppm
	platform/i86pc/kernel/drv/amd64/bmc
	platform/i86pc/kernel/drv/bmc
	platform/i86pc/kernel/drv/bmc.conf
	platform/i86pc/kernel/drv/sbpro
	platform/i86pc/kernel/drv/sbpro.conf
	platform/sun4u/kernel/drv/sparcv9/scmi2c
	platform/sun4u/kernel/misc/sparcv9/i2c_svc
	usr/bin/ksh
	usr/bin/pfksh
	usr/bin/rksh
	usr/bin/smartcard
	usr/ccs/bin/dis
	usr/include/smartcard/
	usr/include/sys/audio/audioens.h
	usr/include/sys/phx.h
	usr/include/sys/sbpro.h
	usr/include/sys/scsi/adapters/ifpcmd.h
	usr/include/sys/scsi/adapters/ifpio.h
	usr/include/sys/scsi/adapters/ifpmail.h
	usr/include/sys/scsi/adapters/ifpreg.h
	usr/include/sys/scsi/adapters/ifpvar.h
	usr/include/sys/scsi/adapters/ispcmd.h
	usr/include/sys/scsi/adapters/ispmail.h
	usr/include/sys/scsi/adapters/ispreg.h
	usr/include/sys/scsi/adapters/ispvar.h
	usr/lib/mdb/disasm/sparc.so
	usr/lib/mdb/disasm/sparcv9/sparc.so
	usr/lib/mdb/kvm/sparcv9/isp.so
	usr/lib/smartcard/
	usr/platform/SUNW,Netra-T12/
	usr/platform/sun4u/include/sys/i2c/clients/scmi2c.h
	usr/platform/sun4u/include/sys/i2c/misc/i2c_svc.h
	usr/platform/sun4u/include/sys/memtestio.h
	usr/platform/sun4u/include/sys/memtestio_ch.h
	usr/platform/sun4u/include/sys/memtestio_chp.h
	usr/platform/sun4u/include/sys/memtestio_ja.h
	usr/platform/sun4u/include/sys/memtestio_jg.h
	usr/platform/sun4u/include/sys/memtestio_sf.h
	usr/platform/sun4u/include/sys/memtestio_sr.h
	usr/platform/sun4u/include/sys/memtestio_u.h
	usr/platform/sun4u/include/sys/memtestio_pn.h
	usr/platform/sun4v/include/sys/memtestio.h
	usr/platform/sun4v/include/sys/memtestio_ni.h
	usr/platform/sun4v/include/sys/memtestio_v.h
	usr/share/javadoc/smartcard/
	usr/share/lib/smartcard/
"
# memory fault injector test framework
delete="$delete
	usr/bin/mtst
	platform/sun4u/kernel/drv/sparcv9/memtest
	platform/sun4u/kernel/drv/memtest.conf
	platform/sun4v/kernel/drv/sparcv9/memtest
	platform/sun4v/kernel/drv/memtest.conf
	platform/i86pc/kernel/drv/memtest.conf
	platform/i86pc/kernel/drv/memtest
	platform/i86pc/kernel/drv/amd64/memtest
	usr/platform/i86pc/lib/mtst/mtst_AuthenticAMD_15.so
"
# pci test tool
delete="$delete
	usr/share/man/man1m/pcitool.1m
	usr/sbin/pcitool
"
for f in $delete; do
	rm -rf $tmpdir/closed/$dir/$f
done

#
# Remove files that the open tree already has.
#
rmfiles=`(cd $open; find . -type f -print -o -type l -print)`
(cd $tmpdir/closed/$dir; rm -f $rmfiles)

#
# Remove any header files.  If they're in the closed tree, they're
# probably not freely redistributable.
#
(cd $tmpdir/closed/$dir; find . -name \*.h -exec rm -f {} \;)

#
# Remove empty directories that the open tree doesn't need.
#
# Step 1: walk the usr/src/pkgdefs files to find out which directories
# are specified in the open packages; save that list to a temporary
# file $needdirs.
#
(cd $SRC/pkgdefs; \
	find . -name prototype\* -exec grep "^d" {} \; | awk '{print $3}' > \
	$needdirs)
#
# Step 2: go to our closed directory, and find all the subdirectories,
# filtering out the ones needed by the open packages (saved in that
# temporary file).  Sort in reverse order, so that parent directories
# come after any subdirectories, and pipe that to rmdir.  If there are
# still any lingering files, rmdir will complain.  That's fine--we
# only want to delete empty directories--so redirect the complaints to
# /dev/null.
#
(cd $tmpdir/closed/$dir; \
	find * -type d -print | /usr/xpg4/bin/grep -xv -f $needdirs | \
	sort -r | xargs -l rmdir 2>/dev/null )

rm $needdirs

#
# Up above we removed the files that were already in the open tree.
# But that blew away the minimal closed binaries that are needed to do
# an open build, so restore them here.
#

mkclosed $isa $full $tmpdir/closed/$dir || \
    fail "Can't restore minimal binaries."

#
# Replace the crypto binaries with ones that have been signed by RE.
# Get these from a local netinstall server.
#

# List of files to copy, in the form "pkgname file [file ...]"
# common files
cfiles="
	SUNWcsl
	usr/lib/security/pkcs11_kernel.so.1
	usr/lib/security/pkcs11_softtoken.so.1
"
# sparc-only
csfiles="
	SUNWcakr.u
	platform/sun4u-us3/kernel/crypto/sparcv9/aes
	platform/sun4u/kernel/crypto/sparcv9/arcfour
	platform/sun4u/kernel/crypto/sparcv9/des
	SUNWckr
	kernel/crypto/sparcv9/aes
	kernel/crypto/sparcv9/arcfour
	kernel/crypto/sparcv9/blowfish
	kernel/crypto/sparcv9/des
	SUNWcsl
	usr/lib/security/sparcv9/pkcs11_kernel.so.1
	usr/lib/security/sparcv9/pkcs11_softtoken.so.1
	SUNWdcar
	kernel/drv/sparcv9/dca
"
# x86-only
cxfiles="
	SUNWckr
	kernel/crypto/aes
	kernel/crypto/arcfour
	kernel/crypto/blowfish
	kernel/crypto/des
	kernel/crypto/amd64/aes
	kernel/crypto/amd64/arcfour
	kernel/crypto/amd64/blowfish
	kernel/crypto/amd64/des
	SUNWcsl
	usr/lib/security/amd64/pkcs11_kernel.so.1
	usr/lib/security/amd64/pkcs11_softtoken.so.1
	SUNWdcar
	kernel/drv/dca
	kernel/drv/amd64/dca
"
# These all have hard links from crypto/foo to misc/foo.
linkedfiles="
	platform/sun4u/kernel/crypto/sparcv9/des
	kernel/crypto/des
	kernel/crypto/amd64/des
	kernel/crypto/sparcv9/des
"

if [ $isa = sparc ]; then
	cfiles="$cfiles $csfiles"
else
	cfiles="$cfiles $cxfiles"
fi

# Extract $pkgfiles from $pkg (no-op if they're empty).
pkgextract()
{
	[ -d $nipath/$pkg ] || fail "$nipath/$pkg doesn't exist."
	if [[ -n "$pkg" && -n "$pkgfiles" ]]; then
		archive=$nipath/$pkg/archive/none.bz2
		bzcat $archive | \
		    (cd $tmpdir/closed/$dir; cpio -idum $pkgfiles)
		# Doesn't look like we can rely on $? here.
		for f in $pkgfiles; do
			[ -f $tmpdir/closed/$dir/$f ] || 
			    echo "Warning: can't extract $f from $archive."
		done
	fi
}

pkg=""
pkgfiles=""
for cf in $cfiles; do
	if [[ $cf = SUNW* ]]; then
		pkgextract
		pkg=$cf
		pkgfiles=""
		continue
	else
		pkgfiles="$pkgfiles $cf"
	fi
done
pkgextract			# last package in $cfiles

# Patch up the crypto hard links.
for f in $linkedfiles; do
	[ -f $tmpdir/closed/$dir/$f ] || continue
	link=`echo $f | sed -e s=crypto=misc=`
	(cd $tmpdir/closed/$dir; rm $link; ln $f $link)
done

#
# Copy over the EK (Encryption Kit) binaries.  This is a slightly different
# procedure than the above code for handling the other crypto binaries, as
# SUNWcry & SUNWcryr aren't accessible by NFS.
# We might want to add an option to let the user pick a different
# (e.g., older) Encryption Kit.
#
wgeterrs=/tmp/wget$$
latest_RE_signed_EK=`wget -O - $ekurl 2>$wgeterrs | \
    nawk -F "\"" \
'/HREF=\"crypt.nv.crypt_[^m]+\"/ { name = $2 } 
END {print name}'`

if [ -z "$latest_RE_signed_EK" ]; then
	echo "Can't find RE-signed Encryption Kit binaries."
	echo "wget errors:"
	cat $wgeterrs
	rm $wgeterrs
	exit 1
fi
rm $wgeterrs

echo "latest RE signed EK cpio archive: $latest_RE_signed_EK"

mkdir $tmpdir/EK
(cd $tmpdir/EK; \
    wget -O - $ekurl/$latest_RE_signed_EK 2>/dev/null | cpio -idum)

cfiles="
	SUNWcry
	usr/bin/des
	usr/lib/security/pkcs11_softtoken_extra.so.1
"
cxfiles="
	SUNWcry
	usr/lib/security/amd64/pkcs11_softtoken_extra.so.1
	SUNWcryr
	kernel/crypto/aes256
	kernel/crypto/arcfour2048
	kernel/crypto/blowfish448
	kernel/crypto/amd64/aes256
	kernel/crypto/amd64/arcfour2048
	kernel/crypto/amd64/blowfish448
"
csfiles="
	SUNWcry
	usr/lib/security/sparcv9/pkcs11_softtoken_extra.so.1
	SUNWcryr
	kernel/crypto/sparcv9/aes256
	kernel/crypto/sparcv9/arcfour2048
	kernel/crypto/sparcv9/blowfish448
	platform/sun4u-us3/kernel/crypto/sparcv9/aes256
	platform/sun4u/kernel/crypto/sparcv9/arcfour2048
	platform/sun4v/kernel/crypto/sparcv9/arcfour2048
"

if [ $isa = sparc ]; then
	cfiles="$cfiles $csfiles"
else
	cfiles="$cfiles $cxfiles"
fi

nipath="$tmpdir/EK/Encryption_11/$isa/Packages"
pkg=""
pkgfiles=""
for cf in $cfiles; do
	if [[ $cf = SUNW* ]]; then
		pkgextract
		pkg=$cf
		pkgfiles=""
		continue
	else
		pkgfiles="$pkgfiles $cf"
	fi
done
pkgextract	# last package in $cfiles

#
# Add binary license files.
#

cp -p $SRC/tools/opensolaris/BINARYLICENSE.txt $tmpdir/closed || \
    fail "Can't add BINARYLICENSE.txt"
mkreadme $tmpdir/closed
cp -p $CODEMGR_WS/THIRDPARTYLICENSE.ON-BINARIES $tmpdir/closed || \
    fail "Can't add THIRDPARTYLICENSE.BFU-ARCHIVES."

tarfile=on-closed-bins.$isa.tar
if [ $nondebug = y ] ; then
	tarfile=on-closed-bins-nd.$isa.tar
fi
tarfile=$CODEMGR_WS/$tarfile
(cd $tmpdir; tar cf $tarfile closed) || fail "Can't create $tarfile."
bzip2 -f $tarfile || fail "Can't compress $tarfile".

rm -rf $tmpdir
