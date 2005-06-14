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

/*
 * Companion to kaif_idt.c - the implementation of the trap and interrupt
 * handlers.  For the most part, these handlers do the same thing - they
 * push a trap number onto the stack, followed by a jump to kaif_cmnint.
 * Each trap and interrupt has its own handler because each one pushes a
 * different number.
 */

#include <sys/asm_linkage.h>
#include <kmdb/kaif_asmutil.h>

/* Nothing in this file is of interest to lint. */
#if !defined(__lint)

/* 
 * The default ASM_ENTRY_ALIGN (16) wastes far too much space.  Pay no
 * attention to the fleet of nop's we're adding to each handler.
 */
#undef	ASM_ENTRY_ALIGN
#define	ASM_ENTRY_ALIGN	8

/*
 * We need the .align in ENTRY_NP (defined to be ASM_ENTRY_ALIGN) to match our
 * manual .align (KAIF_MSR_PATCHOFF) in order to ensure that the space reserved
 * at the beginning of the handler for code is exactly KAIF_MSR_PATCHOFF bytes
 * long.  Note that the #error below isn't supported by the preprocessor invoked
 * by as(1), and won't stop the build, but it'll emit a noticeable error message
 * which won't escape the filters.
 */
#if ASM_ENTRY_ALIGN != KAIF_MSR_PATCHOFF
#error "ASM_ENTRY_ALIGN != KAIF_MSR_PATCHOFF"
this won't assemble
#endif

/*
 * kaif_idt_patch will, on certain processors, replace the patch points below
 * with MSR-clearing code.  kaif_id_patch has intimate knowledge of the size of
 * the nop hole, as well as the structure of the handlers.  Do not change
 * anything here without also changing kaif_idt_patch.
 */

/*
 * Generic trap and interrupt handlers.
 */

#if defined(__amd64)

#define	TRAP_NOERR(trapno) \
	pushq	$trapno

#define	TRAP_ERR(trapno) \
	pushq	$0;			\
	pushq	$trapno

#else	/* __i386 */

#define	TRAP_NOERR(trapno) \
	pushl	$trapno

#define	TRAP_ERR(trapno) \
	pushl	$0;			\
	pushl	$trapno

#endif


#define	MKIVCT(n) \
	ENTRY_NP(kaif_ivct/**/n/**/);	\
	TRAP_ERR(n);			\
	.align	KAIF_MSR_PATCHOFF;	\
	KAIF_MSR_PATCH;			\
	jmp	kaif_cmnint;		\
	SET_SIZE(kaif_ivct/**/n/**/)

#define	MKTRAPHDLR(n) \
	ENTRY_NP(kaif_trap/**/n);	\
	TRAP_ERR(n);			\
	.align	KAIF_MSR_PATCHOFF;	\
	KAIF_MSR_PATCH;			\
	jmp	kaif_cmnint;		\
	SET_SIZE(kaif_trap/**/n/**/)

#define	MKTRAPERRHDLR(n) \
	ENTRY_NP(kaif_traperr/**/n);	\
	TRAP_NOERR(n);			\
	.align	KAIF_MSR_PATCHOFF;	\
	KAIF_MSR_PATCH;			\
	jmp	kaif_cmnint;		\
	SET_SIZE(kaif_traperr/**/n)

#define	MKNMIHDLR \
	ENTRY_NP(kaif_int2);		\
	TRAP_NOERR(2);			\
	.align	KAIF_MSR_PATCHOFF;	\
	KAIF_MSR_PATCH;			\
	jmp	kaif_nmiint;		\
	SET_SIZE(kaif_int2)

#define	MKINVALHDLR \
	ENTRY_NP(kaif_invaltrap);	\
	TRAP_NOERR(255);		\
	.align	KAIF_MSR_PATCHOFF;	\
	KAIF_MSR_PATCH;			\
	jmp	kaif_cmnint;		\
	SET_SIZE(kaif_invaltrap)

/*
 * The handlers themselves
 */

	MKINVALHDLR
	MKTRAPHDLR(0)
	MKTRAPHDLR(1)
	MKNMIHDLR/*2*/
	MKTRAPHDLR(3)
	MKTRAPHDLR(4)
	MKTRAPHDLR(5)
	MKTRAPHDLR(6)
	MKTRAPHDLR(7)
	MKTRAPHDLR(9)
	MKTRAPHDLR(15)
	MKTRAPHDLR(16)
	MKTRAPHDLR(17)
	MKTRAPHDLR(18)
	MKTRAPHDLR(19)
	MKTRAPHDLR(20)

	MKTRAPERRHDLR(8)
	MKTRAPERRHDLR(10)
	MKTRAPERRHDLR(11)
	MKTRAPERRHDLR(12)
	MKTRAPERRHDLR(13)
	MKTRAPERRHDLR(14)

	.globl	kaif_ivct_size
kaif_ivct_size:
	.NWORD [kaif_ivct33-kaif_ivct32]
	
	/* 10 billion and one interrupt handlers */
kaif_ivct_base:
	MKIVCT(32);	MKIVCT(33);	MKIVCT(34);	MKIVCT(35);
	MKIVCT(36);	MKIVCT(37);	MKIVCT(38);	MKIVCT(39);
	MKIVCT(40);	MKIVCT(41);	MKIVCT(42);	MKIVCT(43);
	MKIVCT(44);	MKIVCT(45);	MKIVCT(46);	MKIVCT(47);
	MKIVCT(48);	MKIVCT(49);	MKIVCT(50);	MKIVCT(51);
	MKIVCT(52);	MKIVCT(53);	MKIVCT(54);	MKIVCT(55);
	MKIVCT(56);	MKIVCT(57);	MKIVCT(58);	MKIVCT(59);
	MKIVCT(60);	MKIVCT(61);	MKIVCT(62);	MKIVCT(63);
	MKIVCT(64);	MKIVCT(65);	MKIVCT(66);	MKIVCT(67);
	MKIVCT(68);	MKIVCT(69);	MKIVCT(70);	MKIVCT(71);
	MKIVCT(72);	MKIVCT(73);	MKIVCT(74);	MKIVCT(75);
	MKIVCT(76);	MKIVCT(77);	MKIVCT(78);	MKIVCT(79);
	MKIVCT(80);	MKIVCT(81);	MKIVCT(82);	MKIVCT(83);
	MKIVCT(84);	MKIVCT(85);	MKIVCT(86);	MKIVCT(87);
	MKIVCT(88);	MKIVCT(89);	MKIVCT(90);	MKIVCT(91);
	MKIVCT(92);	MKIVCT(93);	MKIVCT(94);	MKIVCT(95);
	MKIVCT(96);	MKIVCT(97);	MKIVCT(98);	MKIVCT(99);
	MKIVCT(100);	MKIVCT(101);	MKIVCT(102);	MKIVCT(103);
	MKIVCT(104);	MKIVCT(105);	MKIVCT(106);	MKIVCT(107);
	MKIVCT(108);	MKIVCT(109);	MKIVCT(110);	MKIVCT(111);
	MKIVCT(112);	MKIVCT(113);	MKIVCT(114);	MKIVCT(115);
	MKIVCT(116);	MKIVCT(117);	MKIVCT(118);	MKIVCT(119);
	MKIVCT(120);	MKIVCT(121);	MKIVCT(122);	MKIVCT(123);
	MKIVCT(124);	MKIVCT(125);	MKIVCT(126);	MKIVCT(127);
	MKIVCT(128);	MKIVCT(129);	MKIVCT(130);	MKIVCT(131);
	MKIVCT(132);	MKIVCT(133);	MKIVCT(134);	MKIVCT(135);
	MKIVCT(136);	MKIVCT(137);	MKIVCT(138);	MKIVCT(139);
	MKIVCT(140);	MKIVCT(141);	MKIVCT(142);	MKIVCT(143);
	MKIVCT(144);	MKIVCT(145);	MKIVCT(146);	MKIVCT(147);
	MKIVCT(148);	MKIVCT(149);	MKIVCT(150);	MKIVCT(151);
	MKIVCT(152);	MKIVCT(153);	MKIVCT(154);	MKIVCT(155);
	MKIVCT(156);	MKIVCT(157);	MKIVCT(158);	MKIVCT(159);
	MKIVCT(160);	MKIVCT(161);	MKIVCT(162);	MKIVCT(163);
	MKIVCT(164);	MKIVCT(165);	MKIVCT(166);	MKIVCT(167);
	MKIVCT(168);	MKIVCT(169);	MKIVCT(170);	MKIVCT(171);
	MKIVCT(172);	MKIVCT(173);	MKIVCT(174);	MKIVCT(175);
	MKIVCT(176);	MKIVCT(177);	MKIVCT(178);	MKIVCT(179);
	MKIVCT(180);	MKIVCT(181);	MKIVCT(182);	MKIVCT(183);
	MKIVCT(184);	MKIVCT(185);	MKIVCT(186);	MKIVCT(187);
	MKIVCT(188);	MKIVCT(189);	MKIVCT(190);	MKIVCT(191);
	MKIVCT(192);	MKIVCT(193);	MKIVCT(194);	MKIVCT(195);
	MKIVCT(196);	MKIVCT(197);	MKIVCT(198);	MKIVCT(199);
	MKIVCT(200);	MKIVCT(201);	MKIVCT(202);	MKIVCT(203);
	MKIVCT(204);	MKIVCT(205);	MKIVCT(206);	MKIVCT(207);
	MKIVCT(208);	MKIVCT(209);	MKIVCT(210);	MKIVCT(211);
	MKIVCT(212);	MKIVCT(213);	MKIVCT(214);	MKIVCT(215);
	MKIVCT(216);	MKIVCT(217);	MKIVCT(218);	MKIVCT(219);
	MKIVCT(220);	MKIVCT(221);	MKIVCT(222);	MKIVCT(223);
	MKIVCT(224);	MKIVCT(225);	MKIVCT(226);	MKIVCT(227);
	MKIVCT(228);	MKIVCT(229);	MKIVCT(230);	MKIVCT(231);
	MKIVCT(232);	MKIVCT(233);	MKIVCT(234);	MKIVCT(235);
	MKIVCT(236);	MKIVCT(237);	MKIVCT(238);	MKIVCT(239);
	MKIVCT(240);	MKIVCT(241);	MKIVCT(242);	MKIVCT(243);
	MKIVCT(244);	MKIVCT(245);	MKIVCT(246);	MKIVCT(247);
	MKIVCT(248);	MKIVCT(249);	MKIVCT(250);	MKIVCT(251);
	MKIVCT(252);	MKIVCT(253);	MKIVCT(254);	MKIVCT(255);

#endif
