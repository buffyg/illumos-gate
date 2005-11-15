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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/systm.h>
#include <sys/cyclic.h>
#include <sys/cyclic_impl.h>
#include <sys/spl.h>
#include <sys/x_call.h>
#include <sys/kmem.h>
#include <sys/machsystm.h>
#include <sys/smp_impldefs.h>
#include <sys/psm_types.h>
#include <sys/atomic.h>
#include <sys/clock.h>
#include <sys/ddi_impldefs.h>
#include <sys/ddi_intr.h>

static int cbe_vector;
static int cbe_ticks = 0;

static cyc_func_t volatile cbe_xcall_func;
static cpu_t *volatile cbe_xcall_cpu;
static void *cbe_xcall_farg;
static cpuset_t cbe_enabled;

static ddi_softint_hdl_impl_t cbe_low_hdl =
	{0, NULL, NULL, 0, 0, NULL, NULL, NULL};
static ddi_softint_hdl_impl_t cbe_clock_hdl =
	{0, NULL, NULL, 0, 0, NULL, NULL, NULL};

cyclic_id_t cbe_hres_cyclic;
int cbe_psm_timer_mode = TIMER_ONESHOT;

void cbe_hres_tick(void);

int
cbe_softclock(void)
{
	cyclic_softint(CPU, CY_LOCK_LEVEL);
	return (1);
}

int
cbe_low_level(void)
{
	cpu_t *cpu = CPU;

	cyclic_softint(cpu, CY_LOW_LEVEL);
	return (1);
}

/*
 * We can be in cbe_fire() either due to a cyclic-induced cross call, or due
 * to the timer firing at level-14.  Because cyclic_fire() can tolerate
 * spurious calls, it would not matter if we called cyclic_fire() in both
 * cases.
 *
 */
int
cbe_fire(void)
{
	cpu_t *cpu = CPU;
	processorid_t me = cpu->cpu_id, i;
	int cross_call = (cbe_xcall_func != NULL && cbe_xcall_cpu == cpu);

	cyclic_fire(cpu);

	if (cbe_psm_timer_mode != TIMER_ONESHOT && me == 0 && !cross_call) {
		for (i = 1; i < NCPU; i++) {
			if (CPU_IN_SET(cbe_enabled, i))
				send_dirint(i, CBE_HIGH_PIL);
		}
	}

	if (cross_call) {
		ASSERT(cbe_xcall_func != NULL && cbe_xcall_cpu == cpu);
		(*cbe_xcall_func)(cbe_xcall_farg);
		cbe_xcall_func = NULL;
		cbe_xcall_cpu = NULL;
	}

	return (1);
}

/*ARGSUSED*/
void
cbe_softint(void *arg, cyc_level_t level)
{
	switch (level) {
	case CY_LOW_LEVEL:
		cbe_low_hdl.ih_pending = 1;
		(*setsoftint)(CBE_LOW_PIL);
		break;
	case CY_LOCK_LEVEL:
		cbe_clock_hdl.ih_pending = 1;
		(*setsoftint)(CBE_LOCK_PIL);
		break;
	default:
		panic("cbe_softint: unexpected soft level %d", level);
	}
}

/*ARGSUSED*/
void
cbe_reprogram(void *arg, hrtime_t time)
{
	if (cbe_psm_timer_mode == TIMER_ONESHOT)
		(*psm_timer_reprogram)(time);
}

/*ARGSUSED*/
cyc_cookie_t
cbe_set_level(void *arg, cyc_level_t level)
{
	int ipl;

	switch (level) {
	case CY_LOW_LEVEL:
		ipl = CBE_LOW_PIL;
		break;
	case CY_LOCK_LEVEL:
		ipl = CBE_LOCK_PIL;
		break;
	case CY_HIGH_LEVEL:
		ipl = CBE_HIGH_PIL;
		break;
	default:
		panic("cbe_set_level: unexpected level %d", level);
	}

	return (splr(ipltospl(ipl)));
}

/*ARGSUSED*/
void
cbe_restore_level(void *arg, cyc_cookie_t cookie)
{
	splx(cookie);
}

/*ARGSUSED*/
void
cbe_xcall(void *arg, cpu_t *dest, cyc_func_t func, void *farg)
{
	kpreempt_disable();

	if (dest == CPU) {
		(*func)(farg);
		kpreempt_enable();
		return;
	}

	ASSERT(cbe_xcall_func == NULL);

	cbe_xcall_farg = farg;
	membar_producer();
	cbe_xcall_cpu = dest;
	cbe_xcall_func = func;

	send_dirint(dest->cpu_id, CBE_HIGH_PIL);

	while (cbe_xcall_func != NULL || cbe_xcall_cpu != NULL)
		continue;

	kpreempt_enable();

	ASSERT(cbe_xcall_func == NULL && cbe_xcall_cpu == NULL);
}

void *
cbe_configure(cpu_t *cpu)
{
	return (cpu);
}

void
cbe_enable(void *arg)
{
	processorid_t me = ((cpu_t *)arg)->cpu_id;

	if ((cbe_psm_timer_mode != TIMER_ONESHOT) && (me == 0))
		return;

	ASSERT(!CPU_IN_SET(cbe_enabled, me));
	CPUSET_ADD(cbe_enabled, me);
	if (cbe_psm_timer_mode == TIMER_ONESHOT)
		(*psm_timer_enable)();
}

void
cbe_disable(void *arg)
{
	processorid_t me = ((cpu_t *)arg)->cpu_id;

	if (me == 0) {
		/*
		 * If this is the boot CPU, we'll quietly refuse to disable
		 * our clock interrupt.
		 */
		return;
	}

	ASSERT(CPU_IN_SET(cbe_enabled, me));
	CPUSET_DEL(cbe_enabled, me);
	if (cbe_psm_timer_mode == TIMER_ONESHOT)
		(*psm_timer_disable)();
}

/*
 * Called only on CPU 0. This is done since TSCs can have deltas between
 * different cpus see tsc_tick()
 */
void
cbe_hres_tick(void)
{
	int s;

	dtrace_hres_tick();

	/*
	 * Because hres_tick effectively locks hres_lock, we must be at the
	 * same PIL as that used for CLOCK_LOCK.
	 */
	s = splr(ipltospl(XC_HI_PIL));
	hres_tick();
	splx(s);

	if ((cbe_ticks % hz) == 0)
		(*hrtime_tick)();

	cbe_ticks++;

}

void
cbe_init(void)
{
	cyc_backend_t cbe = {
		cbe_configure,		/* cyb_configure */
		NULL,			/* cyb_unconfigure */
		cbe_enable,		/* cyb_enable */
		cbe_disable,		/* cyb_disable */
		cbe_reprogram,		/* cyb_reprogram */
		cbe_softint,		/* cyb_softint */
		cbe_set_level,		/* cyb_set_level */
		cbe_restore_level,	/* cyb_restore_level */
		cbe_xcall,		/* cyb_xcall */
		NULL,			/* cyb_suspend */
		NULL			/* cyb_resume */
	};
	hrtime_t resolution;
	cyc_handler_t hdlr;
	cyc_time_t when;

	cbe_vector = (*psm_get_clockirq)(CBE_HIGH_PIL);

	CPUSET_ZERO(cbe_enabled);

	resolution = (*clkinitf)(TIMER_ONESHOT, &cbe_psm_timer_mode);

	mutex_enter(&cpu_lock);
	cyclic_init(&cbe, resolution);
	mutex_exit(&cpu_lock);

	(void) add_avintr(NULL, CBE_HIGH_PIL, (avfunc)cbe_fire,
	    "cbe_fire_master", cbe_vector, 0, NULL, NULL, NULL);

	if (psm_get_ipivect != NULL) {
		(void) add_avintr(NULL, CBE_HIGH_PIL, (avfunc)cbe_fire,
		    "cbe_fire_slave",
		    (*psm_get_ipivect)(CBE_HIGH_PIL, PSM_INTR_IPI_HI),
		    0, NULL, NULL, NULL);
	}

	(void) add_avsoftintr((void *)&cbe_clock_hdl, CBE_LOCK_PIL,
	    (avfunc)cbe_softclock, "softclock", NULL, NULL);

	(void) add_avsoftintr((void *)&cbe_low_hdl, CBE_LOW_PIL,
	    (avfunc)cbe_low_level, "low level", NULL, NULL);

	mutex_enter(&cpu_lock);

	hdlr.cyh_level = CY_HIGH_LEVEL;
	hdlr.cyh_func = (cyc_func_t)cbe_hres_tick;
	hdlr.cyh_arg = NULL;

	when.cyt_when = 0;
	when.cyt_interval = nsec_per_tick;

	cbe_hres_cyclic = cyclic_add(&hdlr, &when);

	/* bind to cpu 0, which is also the boot cpu */
	cyclic_bind(cbe_hres_cyclic, CPU, NULL);

	if (psm_post_cyclic_setup != NULL)
		(*psm_post_cyclic_setup)(NULL);

	mutex_exit(&cpu_lock);

}
