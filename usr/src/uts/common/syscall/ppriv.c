/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/cred_impl.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <sys/priv_impl.h>
#include <sys/policy.h>
#include <sys/ddi.h>
#include <sys/thread.h>
#include <c2/audit.h>

/*
 * System call support for manipulating privileges.
 *
 *
 * setppriv(2) - set process privilege set
 * getppriv(2) - get process privilege set
 * getprivimplinfo(2) - get process privilege implementation information
 * setpflags(2) - set process (privilege) flags
 * getpflags(2) - get process (privilege) flags
 */

/*
 * setppriv (priv_op_t, priv_ptype_t, priv_set_t)
 */
static int
setppriv(priv_op_t op, priv_ptype_t type, priv_set_t *in_pset)
{
	priv_set_t	pset, *target;
	cred_t		*cr, *pcr;
	proc_t		*p;
	boolean_t	donocd;

	if (!PRIV_VALIDSET(type) || !PRIV_VALIDOP(op))
		return (set_errno(EINVAL));

	if (copyin(in_pset, &pset, sizeof (priv_set_t)))
		return (set_errno(EFAULT));

	p = ttoproc(curthread);
	cr = cralloc();
	mutex_enter(&p->p_crlock);

	pcr = p->p_cred;

#ifdef C2_AUDIT
	if (audit_active)
		audit_setppriv(op, type, &pset, pcr);
#endif

	/*
	 * Filter out unallowed request (bad op and bad type)
	 */
	switch (op) {
	case PRIV_ON:
	case PRIV_SET:
		/*
		 * Turning on privileges; the limit set cannot grow,
		 * other sets can but only as long as they remain subsets
		 * of P.  Only immediately after exec holds that P <= L.
		 */
		if (((type == PRIV_LIMIT &&
			!priv_issubset(&pset, &CR_LPRIV(pcr))) ||
		    !priv_issubset(&pset, &CR_OPPRIV(pcr))) &&
		    !priv_issubset(&pset, priv_getset(pcr, type))) {
			mutex_exit(&p->p_crlock);
			crfree(cr);
			return (set_errno(EPERM));
		}
		break;

	case PRIV_OFF:
		/* PRIV_OFF is always allowed */
		break;
	}

	/*
	 * OK! everything is cool.
	 * Do cred COW.
	 */
	crcopy_to(pcr, cr);

	/*
	 * If we change the effective, permitted or limit set, we attain
	 * "privilege awareness".
	 */
	if (type != PRIV_INHERITABLE)
		priv_set_PA(cr);

	target = &(CR_PRIVS(cr)->crprivs[type]);

	switch (op) {
	case PRIV_ON:
		priv_union(&pset, target);
		break;
	case PRIV_OFF:
		priv_inverse(&pset);
		priv_intersect(target, &pset);

		/*
		 * Fall-thru to set target and change other process
		 * privilege sets.
		 */
		/*FALLTHRU*/

	case PRIV_SET:
		*target = pset;

		/*
		 * Take privileges no longer permitted out
		 * of other effective sets as well.
		 * Limit set is enforced at exec() time.
		 */
		if (type == PRIV_PERMITTED)
			priv_intersect(&pset, &CR_EPRIV(cr));
		break;
	}

	/*
	 * When we give up privileges not in the inheritable set,
	 * set SNOCD if not already set; first we compute the
	 * privileges removed from P using Diff = (~P') & P
	 * and then we check whether the removed privileges are
	 * a subset of I.  If we retain uid 0, all privileges
	 * are required anyway so don't set SNOCD.
	 */
	if (type == PRIV_PERMITTED && (p->p_flag & SNOCD) == 0 &&
	    cr->cr_uid != 0 && cr->cr_ruid != 0 && cr->cr_suid != 0) {
		priv_set_t diff = CR_OPPRIV(cr);
		priv_inverse(&diff);
		priv_intersect(&CR_OPPRIV(pcr), &diff);
		donocd = !priv_issubset(&diff, &CR_IPRIV(cr));
	} else {
		donocd = B_FALSE;
	}

	p->p_cred = cr;
	mutex_exit(&p->p_crlock);

	if (donocd) {
		mutex_enter(&p->p_lock);
		p->p_flag |= SNOCD;
		mutex_exit(&p->p_lock);
	}

	crset(p, cr);		/* broadcast to process threads */

	return (0);
}

/*
 * getppriv (priv_ptype_t, priv_set_t *)
 */
static int
getppriv(priv_ptype_t type, priv_set_t *pset)
{
	if (!PRIV_VALIDSET(type))
		return (set_errno(EINVAL));

	if (copyout(priv_getset(CRED(), type), pset, sizeof (priv_set_t)) != 0)
		return (set_errno(EFAULT));

	return (0);
}

static int
getprivimplinfo(void *buf, size_t bufsize)
{
	int err;

	err = copyout(priv_hold_implinfo(), buf, min(bufsize, privinfosize));

	priv_release_implinfo();

	if (err)
		return (set_errno(EFAULT));

	return (0);
}

/*
 * Set privilege flags
 *
 * For now we cheat: the flags are actually bit masks so we can simplify
 * some; we do make sure that the arguments are valid, though.
 */

static int
setpflags(uint_t flag, uint_t val)
{
	cred_t *cr, *pcr;
	proc_t *p = curproc;
	uint_t newflags;

	if (val > 1 || (flag != PRIV_DEBUG && flag != PRIV_AWARE &&
	    flag != __PROC_PROTECT)) {
		return (set_errno(EINVAL));
	}

	if (flag == __PROC_PROTECT) {
		mutex_enter(&p->p_lock);
		if (val == 0)
			p->p_flag &= ~SNOCD;
		else
			p->p_flag |= SNOCD;
		mutex_exit(&p->p_lock);
		return (0);
	}

	cr = cralloc();

	mutex_enter(&p->p_crlock);

	pcr = p->p_cred;

	newflags = CR_FLAGS(pcr);

	if (val != 0)
		newflags |= flag;
	else
		newflags &= ~flag;

	/* No change */
	if (CR_FLAGS(pcr) == newflags) {
		mutex_exit(&p->p_crlock);
		crfree(cr);
		return (0);
	}

	/* Trying to unset PA; if we can't, return an error */
	if (flag == PRIV_AWARE && val == 0 && !priv_can_clear_PA(pcr)) {
		mutex_exit(&p->p_crlock);
		crfree(cr);
		return (set_errno(EPERM));
	}

	/* Committed to changing the flag */
	crcopy_to(pcr, cr);
	if (flag == PRIV_AWARE) {
		if (val != 0)
			priv_set_PA(cr);
		else
			priv_adjust_PA(cr);
	} else {
		CR_FLAGS(cr) = newflags;
	}

	p->p_cred = cr;

	mutex_exit(&p->p_crlock);

	crset(p, cr);

	return (0);
}

/*
 * Getpflags.  Currently only implements single bit flags.
 */
static uint_t
getpflags(uint_t flag)
{
	if (flag != PRIV_DEBUG && flag != PRIV_AWARE)
		return (set_errno(EINVAL));

	return ((CR_FLAGS(CRED()) & flag) != 0);
}

/*
 * Privilege system call entry point
 */
int
privsys(int code, priv_op_t op, priv_ptype_t type, void *buf, size_t bufsize)
{
	switch (code) {
	case PRIVSYS_SETPPRIV:
		if (bufsize < sizeof (priv_set_t))
			return (set_errno(ENOMEM));
		return (setppriv(op, type, buf));
	case PRIVSYS_GETPPRIV:
		if (bufsize < sizeof (priv_set_t))
			return (set_errno(ENOMEM));
		return (getppriv(type, buf));
	case PRIVSYS_GETIMPLINFO:
		return (getprivimplinfo(buf, bufsize));
	case PRIVSYS_SETPFLAGS:
		return (setpflags((uint_t)op, (uint_t)type));
	case PRIVSYS_GETPFLAGS:
		return ((int)getpflags((uint_t)op));

	}
	return (set_errno(EINVAL));
}

#ifdef _SYSCALL32_IMPL
int
privsys32(int code, priv_op_t op, priv_ptype_t type, caddr32_t *buf,
    size32_t bufsize)
{
	return (privsys(code, op, type, (void *)buf, (size_t)bufsize));
}
#endif
