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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include "defs.h"
#include "tables.h"
#include <fcntl.h>

static void	initlog(void);
static void	run_timeouts(void);

static void	advertise(struct sockaddr_in6 *sin6, struct phyint *pi,
		    boolean_t no_prefixes);
static void	solicit(struct sockaddr_in6 *sin6, struct phyint *pi);
static void	initifs(boolean_t first);
static void	check_if_removed(struct phyint *pi);
static void	loopback_ra_enqueue(struct phyint *pi,
		    struct nd_router_advert *ra, int len);
static void	loopback_ra_dequeue(void);
static void	check_daemonize(void);

struct in6_addr all_nodes_mcast = { { 0xff, 0x2, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x1 } };

struct in6_addr all_routers_mcast = { { 0xff, 0x2, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x2 } };

static struct sockaddr_in6 v6allnodes = { AF_INET6, 0, 0,
				    { 0xff, 0x2, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x1 } };

static struct sockaddr_in6 v6allrouters = { AF_INET6, 0, 0,
				    { 0xff, 0x2, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x0,
				    0x0, 0x0, 0x0, 0x2 } };

static char **argv0;		/* Saved for re-exec on SIGHUP */

static uint64_t packet[(IP_MAXPACKET + 1)/8];

static int	show_ifs = 0;
static boolean_t	already_daemonized = _B_FALSE;
int		debug = 0;
int		no_loopback = 0; /* Do not send RA packets to ourselves */

/*
 * Size of routing socket message used by in.ndpd which includes the header,
 * space for the RTA_DST, RTA_GATEWAY and RTA_NETMASK (each a sockaddr_in6)
 * plus space for the RTA_IFP (a sockaddr_dl).
 */
#define	NDP_RTM_MSGLEN	sizeof (struct rt_msghdr) +	\
			sizeof (struct sockaddr_in6) +	\
			sizeof (struct sockaddr_in6) +	\
			sizeof (struct sockaddr_in6) +	\
			sizeof (struct sockaddr_dl)

/*
 * These are referenced externally in tables.c in order to fill in the
 * dynamic portions of the routing socket message and then to send the message
 * itself.
 */
int	rtsock = -1;			/* Routing socket */
struct	rt_msghdr	*rt_msg;	/* Routing socket message */
struct	sockaddr_in6	*rta_gateway;	/* RTA_GATEWAY sockaddr */
struct	sockaddr_dl	*rta_ifp;	/* RTA_IFP sockaddr */

/*
 * Return the current time in milliseconds truncated to
 * fit in an integer.
 */
uint_t
getcurrenttime(void)
{
	struct timeval tp;

	if (gettimeofday(&tp, NULL) < 0) {
		logperror("getcurrenttime: gettimeofday failed");
		exit(1);
	}
	return (tp.tv_sec * 1000 + tp.tv_usec / 1000);
}

/*
 * Output a preformated packet from the packet[] buffer.
 */
static void
sendpacket(struct sockaddr_in6 *sin6, int sock, int size, int flags)
{
	int cc;
	char abuf[INET6_ADDRSTRLEN];

	cc = sendto(sock, (char *)packet, size, flags,
		(struct sockaddr *)sin6, sizeof (*sin6));
	if (cc < 0 || cc != size) {
		if (cc < 0) {
			logperror("sendpacket: sendto");
		}
		logmsg(LOG_ERR, "sendpacket: wrote %s %d chars, ret=%d\n",
		    inet_ntop(sin6->sin6_family,
		    (void *)&sin6->sin6_addr,
		    abuf, sizeof (abuf)),
		    size, cc);
	}
}

/* Send a Router Solicitation */
static void
solicit(struct sockaddr_in6 *sin6, struct phyint *pi)
{
	int packetlen = 0;
	struct	nd_router_solicit *rs = (struct nd_router_solicit *)packet;
	char *pptr = (char *)packet;

	rs->nd_rs_type = ND_ROUTER_SOLICIT;
	rs->nd_rs_code = 0;
	rs->nd_rs_cksum = htons(0);
	rs->nd_rs_reserved = htonl(0);

	packetlen += sizeof (*rs);
	pptr += sizeof (*rs);

	/* Attach any options */
	if (pi->pi_hdw_addr_len != 0) {
		struct nd_opt_lla *lo = (struct nd_opt_lla *)pptr;
		int optlen;

		/* roundup to multiple of 8 and make padding zero */
		optlen = ((sizeof (struct nd_opt_hdr) +
		    pi->pi_hdw_addr_len + 7) / 8) * 8;
		bzero(pptr, optlen);

		lo->nd_opt_lla_type = ND_OPT_SOURCE_LINKADDR;
		lo->nd_opt_lla_len = optlen / 8;
		bcopy((char *)pi->pi_hdw_addr,
		    (char *)lo->nd_opt_lla_hdw_addr,
		    pi->pi_hdw_addr_len);
		packetlen += optlen;
		pptr += optlen;
	}

	if (debug & D_PKTOUT) {
		print_route_sol("Sending solicitation to ", pi, rs, packetlen,
		    sin6);
	}
	sendpacket(sin6, pi->pi_sock, packetlen, 0);
}

/*
 * Send a (set of) Router Advertisements and feed them back to ourselves
 * for processing. Unless no_prefixes is set all prefixes are included.
 * If there are too many prefix options to fit in one packet multiple
 * packets will be sent - each containing a subset of the prefix options.
 */
static void
advertise(struct sockaddr_in6 *sin6, struct phyint *pi, boolean_t no_prefixes)
{
	struct	nd_opt_prefix_info *po;
	char *pptr = (char *)packet;
	struct nd_router_advert *ra;
	struct adv_prefix *adv_pr;
	int packetlen = 0;

	ra = (struct nd_router_advert *)pptr;
	ra->nd_ra_type = ND_ROUTER_ADVERT;
	ra->nd_ra_code = 0;
	ra->nd_ra_cksum = htons(0);
	ra->nd_ra_curhoplimit = pi->pi_AdvCurHopLimit;
	ra->nd_ra_flags_reserved = 0;
	if (pi->pi_AdvManagedFlag)
		ra->nd_ra_flags_reserved |= ND_RA_FLAG_MANAGED;
	if (pi->pi_AdvOtherConfigFlag)
		ra->nd_ra_flags_reserved |= ND_RA_FLAG_OTHER;

	if (pi->pi_adv_state == FINAL_ADV)
		ra->nd_ra_router_lifetime = htons(0);
	else
		ra->nd_ra_router_lifetime = htons(pi->pi_AdvDefaultLifetime);
	ra->nd_ra_reachable = htonl(pi->pi_AdvReachableTime);
	ra->nd_ra_retransmit = htonl(pi->pi_AdvRetransTimer);

	packetlen = sizeof (*ra);
	pptr += sizeof (*ra);

	if (pi->pi_adv_state == FINAL_ADV) {
		if (debug & D_PKTOUT) {
			print_route_adv("Sending advert (FINAL) to ", pi,
			    ra, packetlen, sin6);
		}
		sendpacket(sin6, pi->pi_sock, packetlen, 0);
		/* Feed packet back in for router operation */
		loopback_ra_enqueue(pi, ra, packetlen);
		return;
	}

	/* Attach any options */
	if (pi->pi_hdw_addr_len != 0) {
		struct nd_opt_lla *lo = (struct nd_opt_lla *)pptr;
		int optlen;

		/* roundup to multiple of 8 and make padding zero */
		optlen = ((sizeof (struct nd_opt_hdr) +
		    pi->pi_hdw_addr_len + 7) / 8) * 8;
		bzero(pptr, optlen);

		lo->nd_opt_lla_type = ND_OPT_SOURCE_LINKADDR;
		lo->nd_opt_lla_len = optlen / 8;
		bcopy((char *)pi->pi_hdw_addr,
		    (char *)lo->nd_opt_lla_hdw_addr,
		    pi->pi_hdw_addr_len);
		packetlen += optlen;
		pptr += optlen;
	}

	if (pi->pi_AdvLinkMTU != 0) {
		struct nd_opt_mtu *mo = (struct nd_opt_mtu *)pptr;

		mo->nd_opt_mtu_type = ND_OPT_MTU;
		mo->nd_opt_mtu_len = sizeof (struct nd_opt_mtu) / 8;
		mo->nd_opt_mtu_reserved = 0;
		mo->nd_opt_mtu_mtu = htonl(pi->pi_AdvLinkMTU);

		packetlen += sizeof (struct nd_opt_mtu);
		pptr += sizeof (struct nd_opt_mtu);
	}

	if (no_prefixes) {
		if (debug & D_PKTOUT) {
			print_route_adv("Sending advert to ", pi,
			    ra, packetlen, sin6);
		}
		sendpacket(sin6, pi->pi_sock, packetlen, 0);
		/* Feed packet back in for router operation */
		loopback_ra_enqueue(pi, ra, packetlen);
		return;
	}

	po = (struct nd_opt_prefix_info *)pptr;
	for (adv_pr = pi->pi_adv_prefix_list; adv_pr != NULL;
	    adv_pr = adv_pr->adv_pr_next) {
		if (!adv_pr->adv_pr_AdvOnLinkFlag &&
		    !adv_pr->adv_pr_AdvAutonomousFlag) {
			continue;
		}

		/*
		 * If the prefix doesn't fit in packet send
		 * what we have so far and start with new packet.
		 */
		if (packetlen + sizeof (*po) >
		    pi->pi_LinkMTU - sizeof (struct ip6_hdr)) {
			if (debug & D_PKTOUT) {
				print_route_adv("Sending advert "
				    "(FRAG) to ",
				    pi, ra, packetlen, sin6);
			}
			sendpacket(sin6, pi->pi_sock, packetlen, 0);
			/* Feed packet back in for router operation */
			loopback_ra_enqueue(pi, ra, packetlen);
			packetlen = sizeof (*ra);
			pptr = (char *)packet + sizeof (*ra);
			po = (struct nd_opt_prefix_info *)pptr;
		}
		po->nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
		po->nd_opt_pi_len = sizeof (*po)/8;
		po->nd_opt_pi_flags_reserved = 0;
		if (adv_pr->adv_pr_AdvOnLinkFlag) {
			po->nd_opt_pi_flags_reserved |=
			    ND_OPT_PI_FLAG_ONLINK;
		}
		if (adv_pr->adv_pr_AdvAutonomousFlag) {
			po->nd_opt_pi_flags_reserved |=
			    ND_OPT_PI_FLAG_AUTO;
		}
		po->nd_opt_pi_prefix_len = adv_pr->adv_pr_prefix_len;
		/*
		 * If both Adv*Expiration and Adv*Lifetime are
		 * set we prefer the former and make the lifetime
		 * decrement in real time.
		 */
		if (adv_pr->adv_pr_AdvValidRealTime) {
			po->nd_opt_pi_valid_time =
			    htonl(adv_pr->adv_pr_AdvValidExpiration);
		} else {
			po->nd_opt_pi_valid_time =
			    htonl(adv_pr->adv_pr_AdvValidLifetime);
		}
		if (adv_pr->adv_pr_AdvPreferredRealTime) {
			po->nd_opt_pi_preferred_time =
			    htonl(adv_pr->adv_pr_AdvPreferredExpiration);
		} else {
			po->nd_opt_pi_preferred_time =
			    htonl(adv_pr->adv_pr_AdvPreferredLifetime);
		}
		po->nd_opt_pi_reserved2 = htonl(0);
		po->nd_opt_pi_prefix = adv_pr->adv_pr_prefix;

		po++;
		packetlen += sizeof (*po);
	}
	if (debug & D_PKTOUT) {
		print_route_adv("Sending advert to ", pi,
		    ra, packetlen, sin6);
	}
	sendpacket(sin6, pi->pi_sock, packetlen, 0);
	/* Feed packet back in for router operation */
	loopback_ra_enqueue(pi, ra, packetlen);
}

/* Poll support */
static int		pollfd_num = 0;	/* Allocated and initialized */
static struct pollfd	*pollfds = NULL;

/*
 * Add fd to the set being polled. Returns 0 if ok; -1 if failed.
 */
int
poll_add(int fd)
{
	int i;
	int new_num;
	struct pollfd *newfds;
retry:
	/* Check if already present */
	for (i = 0; i < pollfd_num; i++) {
		if (pollfds[i].fd == fd)
			return (0);
	}
	/* Check for empty spot already present */
	for (i = 0; i < pollfd_num; i++) {
		if (pollfds[i].fd == -1) {
			pollfds[i].fd = fd;
			return (0);
		}
	}

	/* Allocate space for 32 more fds and initialize to -1 */
	new_num = pollfd_num + 32;
	newfds = realloc(pollfds, new_num * sizeof (struct pollfd));
	if (newfds == NULL) {
		logperror("poll_add: realloc");
		return (-1);
	}
	for (i = pollfd_num; i < new_num; i++) {
		newfds[i].fd = -1;
		newfds[i].events = POLLIN;
	}
	pollfd_num = new_num;
	pollfds = newfds;
	goto retry;
}

/*
 * Remove fd from the set being polled. Returns 0 if ok; -1 if failed.
 */
int
poll_remove(int fd)
{
	int i;

	/* Check if already present */
	for (i = 0; i < pollfd_num; i++) {
		if (pollfds[i].fd == fd) {
			pollfds[i].fd = -1;
			return (0);
		}
	}
	return (-1);
}

/*
 * Extract information about the ifname (either a physical interface and
 * the ":0" logical interface or just a logical interface).
 * If the interface (still) exists in kernel set pr_in_use
 * for caller to be able to detect interfaces that are removed.
 * Starts sending advertisements/solicitations when new physical interfaces
 * are detected.
 */
static void
if_process(int s, char *ifname, boolean_t first)
{
	struct lifreq lifr;
	struct phyint *pi;
	struct prefix *pr;
	char *cp;
	char phyintname[LIFNAMSIZ + 1];

	if (debug & D_IFSCAN)
		logmsg(LOG_DEBUG, "if_process(%s)\n", ifname);

	(void) strncpy(lifr.lifr_name, ifname, sizeof (lifr.lifr_name));
	lifr.lifr_name[sizeof (lifr.lifr_name) - 1] = '\0';
	if (ioctl(s, SIOCGLIFFLAGS, (char *)&lifr) < 0) {
		if (errno == ENXIO) {
			/*
			 * Interface has disappeared
			 */
			return;
		}
		logperror("if_process: ioctl (get interface flags)");
		return;
	}

	/*
	 * Ignore loopback and point-to-multipoint interfaces.
	 * Point-to-point interfaces always have IFF_MULTICAST set.
	 */
	if (!(lifr.lifr_flags & IFF_MULTICAST) ||
	    (lifr.lifr_flags & IFF_LOOPBACK)) {
		return;
	}

	if (!(lifr.lifr_flags & IFF_IPV6))
		return;

	(void) strncpy(phyintname, ifname, sizeof (phyintname));
	phyintname[sizeof (phyintname) - 1] = '\0';
	if ((cp = strchr(phyintname, IF_SEPARATOR)) != NULL) {
		*cp = '\0';
	}

	pi = phyint_lookup(phyintname);
	if (pi == NULL) {
		/*
		 * Do not add anything for new interfaces until they are UP.
		 * For existing interfaces we track the up flag.
		 */
		if (!(lifr.lifr_flags & IFF_UP))
			return;

		pi = phyint_create(phyintname);
		if (pi == NULL) {
			logmsg(LOG_ERR, "if_process: out of memory\n");
			return;
		}
	}
	(void) phyint_init_from_k(pi);
	if (pi->pi_sock == -1 && !(pi->pi_kernel_state & PI_PRESENT)) {
		/* Interface is not yet present */
		if (debug & D_PHYINT) {
			logmsg(LOG_DEBUG, "if_process: interface not yet "
			    "present %s\n", pi->pi_name);
		}
		return;
	}

	if (pi->pi_sock != -1) {
		if (poll_add(pi->pi_sock) == -1) {
			/*
			 * reset state.
			 */
			phyint_cleanup(pi);
		}
	}

	/*
	 * Check if IFF_ROUTER has been turned off in kernel in which
	 * case we have to turn off AdvSendAdvertisements.
	 * The kernel will automatically turn off IFF_ROUTER if
	 * ip6_forwarding is turned off.
	 * Note that we do not switch back should IFF_ROUTER be turned on.
	 */
	if (!first &&
	    pi->pi_AdvSendAdvertisements && !(pi->pi_flags & IFF_ROUTER)) {
		logmsg(LOG_INFO, "No longer a router on %s\n", pi->pi_name);
		check_to_advertise(pi, START_FINAL_ADV);

		pi->pi_AdvSendAdvertisements = 0;
		pi->pi_sol_state = NO_SOLICIT;
	}

	/*
	 * Send advertisments and solicitation only if the interface is
	 * present in the kernel.
	 */
	if (pi->pi_kernel_state & PI_PRESENT) {

		if (pi->pi_AdvSendAdvertisements) {
			if (pi->pi_adv_state == NO_ADV)
				check_to_advertise(pi, START_INIT_ADV);
		} else {
			if (pi->pi_sol_state == NO_SOLICIT)
				check_to_solicit(pi, START_INIT_SOLICIT);
		}
	}

	/*
	 * Track static kernel prefixes to prevent in.ndpd from clobbering
	 * them by creating a struct prefix for each prefix detected in the
	 * kernel.
	 */
	pr = prefix_lookup_name(pi, ifname);
	if (pr == NULL) {
		pr = prefix_create_name(pi, ifname);
		if (pr == NULL) {
			logmsg(LOG_ERR, "if_process: out of memory\n");
			return;
		}
		if (prefix_init_from_k(pr) == -1) {
			prefix_delete(pr);
			return;
		}
	}
	/* Detect prefixes which are removed */
	if (pr->pr_kernel_state != 0)
		pr->pr_in_use = _B_TRUE;
}

static int ifsock = -1;

/*
 * Scan all interfaces to detect changes as well as new and deleted intefaces
 * 'first' is set for the initial call only. Do not effect anything.
 */
static void
initifs(boolean_t first)
{
	char *buf;
	int bufsize;
	int numifs;
	int n;
	struct lifnum lifn;
	struct lifconf lifc;
	struct lifreq *lifr;
	struct phyint *pi;
	struct phyint *next_pi;
	struct prefix *pr;

	if (debug & D_IFSCAN)
		logmsg(LOG_DEBUG, "Reading interface configuration\n");
	if (ifsock < 0) {
		ifsock = socket(AF_INET6, SOCK_DGRAM, 0);
		if (ifsock < 0) {
			logperror("initifs: socket");
			return;
		}
	}
	lifn.lifn_family = AF_INET6;
	lifn.lifn_flags = LIFC_NOXMIT | LIFC_TEMPORARY;
	if (ioctl(ifsock, SIOCGLIFNUM, (char *)&lifn) < 0) {
		logperror("initifs: ioctl (get interface numbers)");
		return;
	}
	numifs = lifn.lifn_count;
	bufsize = numifs * sizeof (struct lifreq);

	buf = (char *)malloc(bufsize);
	if (buf == NULL) {
		logmsg(LOG_ERR, "initifs: out of memory\n");
		return;
	}

	/*
	 * Mark the interfaces so that we can find phyints and prefixes
	 * which have disappeared from the kernel.
	 * if_process will set pr_in_use when it finds the interface
	 * in the kernel.
	 */
	for (pi = phyints; pi != NULL; pi = pi->pi_next) {
		/*
		 * Before re-examining the state of the interfaces,
		 * PI_PRESENT should be cleared from pi_kernel_state.
		 */
		pi->pi_kernel_state &= ~PI_PRESENT;
		for (pr = pi->pi_prefix_list; pr != NULL; pr = pr->pr_next) {
			pr->pr_in_use = _B_FALSE;
		}
	}

	lifc.lifc_family = AF_INET6;
	lifc.lifc_flags = LIFC_NOXMIT | LIFC_TEMPORARY;
	lifc.lifc_len = bufsize;
	lifc.lifc_buf = buf;

	if (ioctl(ifsock, SIOCGLIFCONF, (char *)&lifc) < 0) {
		logperror("initifs: ioctl (get interface configuration)");
		free(buf);
		return;
	}

	lifr = (struct lifreq *)lifc.lifc_req;
	for (n = lifc.lifc_len / sizeof (struct lifreq); n > 0; n--, lifr++)
		if_process(ifsock, lifr->lifr_name, first);
	free(buf);

	/*
	 * Detect phyints that have been removed from the kernel.
	 * Since we can't recreate it here (would require ifconfig plumb
	 * logic) we just terminate use of that phyint.
	 */
	for (pi = phyints; pi != NULL; pi = next_pi) {
		next_pi = pi->pi_next;
		/*
		 * If interface (still) exists in kernel, set
		 * pi_state to indicate that.
		 */
		if (pi->pi_kernel_state & PI_PRESENT) {
			pi->pi_state |= PI_PRESENT;
		}

		check_if_removed(pi);
	}
	if (show_ifs)
		phyint_print_all();
}


/*
 * Router advertisement state machine. Used for everything but timer
 * events which use advertise_event directly.
 */
void
check_to_advertise(struct phyint *pi, enum adv_events event)
{
	uint_t delay;
	enum adv_states old_state = pi->pi_adv_state;

	if (debug & D_STATE) {
		logmsg(LOG_DEBUG, "check_to_advertise(%s, %d) state %d\n",
		    pi->pi_name, (int)event, (int)old_state);
	}
	delay = advertise_event(pi, event, 0);
	if (delay != TIMER_INFINITY) {
		/* Make sure the global next event is updated */
		timer_schedule(delay);
	}

	if (debug & D_STATE) {
		logmsg(LOG_DEBUG, "check_to_advertise(%s, %d) state %d -> %d\n",
		    pi->pi_name, (int)event, (int)old_state,
		    (int)pi->pi_adv_state);
	}
}

/*
 * Router advertisement state machine.
 * Return the number of milliseconds until next timeout (TIMER_INFINITY
 * if never).
 * For the ADV_TIMER event the caller passes in the number of milliseconds
 * since the last timer event in the 'elapsed' parameter.
 */
uint_t
advertise_event(struct phyint *pi, enum adv_events event, uint_t elapsed)
{
	uint_t delay;

	if (debug & D_STATE) {
		logmsg(LOG_DEBUG, "advertise_event(%s, %d, %d) state %d\n",
		    pi->pi_name, (int)event, elapsed, (int)pi->pi_adv_state);
	}
	check_daemonize();
	if (!pi->pi_AdvSendAdvertisements)
		return (TIMER_INFINITY);
	if (pi->pi_flags & IFF_NORTEXCH) {
		if (debug & D_PKTOUT) {
			logmsg(LOG_DEBUG, "Suppress sending RA packet on %s "
			    "(no route exchange on interface)\n",
			    pi->pi_name);
		}
		return (TIMER_INFINITY);
	}

	switch (event) {
	case ADV_OFF:
		pi->pi_adv_state = NO_ADV;
		return (TIMER_INFINITY);

	case START_INIT_ADV:
		if (pi->pi_adv_state == INIT_ADV)
			return (pi->pi_adv_time_left);
		pi->pi_adv_count = ND_MAX_INITIAL_RTR_ADVERTISEMENTS;
		pi->pi_adv_time_left = 0;
		pi->pi_adv_state = INIT_ADV;
		break;	/* send advertisement */

	case START_FINAL_ADV:
		if (pi->pi_adv_state == NO_ADV)
			return (TIMER_INFINITY);
		if (pi->pi_adv_state == FINAL_ADV)
			return (pi->pi_adv_time_left);
		pi->pi_adv_count = ND_MAX_FINAL_RTR_ADVERTISEMENTS;
		pi->pi_adv_time_left = 0;
		pi->pi_adv_state = FINAL_ADV;
		break;	/* send advertisement */

	case RECEIVED_SOLICIT:
		if (pi->pi_adv_state == NO_ADV)
			return (TIMER_INFINITY);
		if (pi->pi_adv_state == SOLICIT_ADV) {
			if (pi->pi_adv_time_left != 0)
				return (pi->pi_adv_time_left);
			break;
		}
		delay = GET_RANDOM(0, ND_MAX_RA_DELAY_TIME);
		if (delay < pi->pi_adv_time_left)
			pi->pi_adv_time_left = delay;
		if (pi->pi_adv_time_since_sent < ND_MIN_DELAY_BETWEEN_RAS) {
			/*
			 * Send an advertisement (ND_MIN_DELAY_BETWEEN_RAS
			 * plus random delay) after the previous
			 * advertisement was sent.
			 */
			pi->pi_adv_time_left = delay +
			    ND_MIN_DELAY_BETWEEN_RAS -
			    pi->pi_adv_time_since_sent;
		}
		pi->pi_adv_state = SOLICIT_ADV;
		break;

	case ADV_TIMER:
		if (pi->pi_adv_state == NO_ADV)
			return (TIMER_INFINITY);
		/* Decrease time left */
		if (pi->pi_adv_time_left >= elapsed)
			pi->pi_adv_time_left -= elapsed;
		else
			pi->pi_adv_time_left = 0;

		/* Increase time since last advertisement was sent */
		pi->pi_adv_time_since_sent += elapsed;
		break;
	default:
		logmsg(LOG_ERR, "advertise_event: Unknown event %d\n",
		    (int)event);
		return (TIMER_INFINITY);
	}

	if (pi->pi_adv_time_left != 0)
		return (pi->pi_adv_time_left);

	/* Send advertisement and calculate next time to send */
	if (pi->pi_adv_state == FINAL_ADV) {
		/* Omit the prefixes */
		advertise(&v6allnodes, pi, _B_TRUE);
	} else {
		advertise(&v6allnodes, pi, _B_FALSE);
	}
	pi->pi_adv_time_since_sent = 0;

	switch (pi->pi_adv_state) {
	case SOLICIT_ADV:
		/*
		 * The solicited advertisement has been sent.
		 * Revert to periodic advertisements.
		 */
		pi->pi_adv_state = REG_ADV;
		/* FALLTHRU */
	case REG_ADV:
		pi->pi_adv_time_left =
		    GET_RANDOM(1000 * pi->pi_MinRtrAdvInterval,
		    1000 * pi->pi_MaxRtrAdvInterval);
		break;

	case INIT_ADV:
		if (--pi->pi_adv_count > 0) {
			delay = GET_RANDOM(1000 * pi->pi_MinRtrAdvInterval,
			    1000 * pi->pi_MaxRtrAdvInterval);
			if (delay > ND_MAX_INITIAL_RTR_ADVERT_INTERVAL)
				delay = ND_MAX_INITIAL_RTR_ADVERT_INTERVAL;
			pi->pi_adv_time_left = delay;
		} else {
			pi->pi_adv_time_left =
			    GET_RANDOM(1000 * pi->pi_MinRtrAdvInterval,
			    1000 * pi->pi_MaxRtrAdvInterval);
			pi->pi_adv_state = REG_ADV;
		}
		break;

	case FINAL_ADV:
		if (--pi->pi_adv_count > 0) {
			pi->pi_adv_time_left =
			    ND_MAX_INITIAL_RTR_ADVERT_INTERVAL;
		} else {
			pi->pi_adv_state = NO_ADV;
		}
		break;
	}
	if (pi->pi_adv_state != NO_ADV)
		return (pi->pi_adv_time_left);
	else
		return (TIMER_INFINITY);
}

/*
 * Router solicitation state machine. Used for everything but timer
 * events which use solicit_event directly.
 */
void
check_to_solicit(struct phyint *pi, enum solicit_events event)
{
	uint_t delay;
	enum solicit_states old_state = pi->pi_sol_state;

	if (debug & D_STATE) {
		logmsg(LOG_DEBUG, "check_to_solicit(%s, %d) state %d\n",
		    pi->pi_name, (int)event, (int)old_state);
	}
	delay = solicit_event(pi, event, 0);
	if (delay != TIMER_INFINITY) {
		/* Make sure the global next event is updated */
		timer_schedule(delay);
	}

	if (debug & D_STATE) {
		logmsg(LOG_DEBUG, "check_to_solicit(%s, %d) state %d -> %d\n",
		    pi->pi_name, (int)event, (int)old_state,
		    (int)pi->pi_sol_state);
	}
}

static void
daemonize_ndpd(void)
{
	FILE *pidfp;
	mode_t pidmode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); /* 0644 */
	struct itimerval it;
	boolean_t timerval = _B_TRUE;

	/*
	 * Need to get current timer settings so they can be restored
	 * after the fork(), as the it_value and it_interval values for
	 * the ITIMER_REAL timer are reset to 0 in the child process.
	 */
	if (getitimer(ITIMER_REAL, &it) < 0) {
		if (debug & D_TIMER)
			logmsg(LOG_DEBUG,
			    "daemonize_ndpd: failed to get itimerval\n");
		timerval = _B_FALSE;
	}

	/* Daemonize. */
	switch (fork()) {
	case 0:
		/* Child */
		break;
	case -1:
		logperror("fork");
		exit(1);
	default:
		/* Parent */
		_exit(0);
	}

	/* Store our process id, blow away any existing file if it exists. */
	if ((pidfp = fopen(PATH_PID, "w")) == NULL) {
		(void) fprintf(stderr, "%s: unable to open " PATH_PID ": %s\n",
		    argv0[0], strerror(errno));
	} else {
		(void) fprintf(pidfp, "%ld\n", getpid());
		(void) fclose(pidfp);
		(void) chmod(PATH_PID, pidmode);
	}

	(void) close(0);
	(void) close(1);
	(void) close(2);

	(void) chdir("/");
	(void) open("/dev/null", O_RDWR);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
	(void) setsid();

	already_daemonized = _B_TRUE;

	/*
	 * Restore timer values, if we were able to save them; if not,
	 * check and set the right value by calling run_timeouts().
	 */
	if (timerval) {
		if (setitimer(ITIMER_REAL, &it, NULL) < 0) {
			logperror("daemonize_ndpd: setitimer");
			exit(2);
		}
	} else {
		run_timeouts();
	}
}

/*
 * Check to see if the time is right to daemonize.  The right time is when:
 *
 * 1.  We haven't already daemonized.
 * 2.  We are not in debug mode.
 * 3.  All interfaces are marked IFF_NOXMIT.
 * 4.  All non-router interfaces have their prefixes set up and we're
 *     done sending router solicitations on those interfaces without
 *     prefixes.
 */
static void
check_daemonize(void)
{
	struct phyint		*pi;

	if (already_daemonized || debug != 0)
		return;

	for (pi = phyints; pi != NULL; pi = pi->pi_next) {
		if (!(pi->pi_flags & IFF_NOXMIT))
			break;
	}

	/*
	 * If we can't transmit on any of the interfaces there is no reason
	 * to hold up progress.
	 */
	if (pi == NULL) {
		daemonize_ndpd();
		return;
	}

	/* Check all interfaces.  If any are still soliciting, just return. */
	for (pi = phyints; pi != NULL; pi = pi->pi_next) {
		if (pi->pi_AdvSendAdvertisements ||
		    !(pi->pi_kernel_state & PI_PRESENT))
			continue;

		if (pi->pi_sol_state == INIT_SOLICIT)
			return;
	}

	daemonize_ndpd();
}

/*
 * Router solicitation state machine.
 * Return the number of milliseconds until next timeout (TIMER_INFINITY
 * if never).
 * For the SOL_TIMER event the caller passes in the number of milliseconds
 * since the last timer event in the 'elapsed' parameter.
 */
uint_t
solicit_event(struct phyint *pi, enum solicit_events event, uint_t elapsed)
{
	if (debug & D_STATE) {
		logmsg(LOG_DEBUG, "solicit_event(%s, %d, %d) state %d\n",
		    pi->pi_name, (int)event, elapsed, (int)pi->pi_sol_state);
	}

	if (pi->pi_AdvSendAdvertisements)
		return (TIMER_INFINITY);
	if (pi->pi_flags & IFF_NORTEXCH) {
		if (debug & D_PKTOUT) {
			logmsg(LOG_DEBUG, "Suppress sending RS packet on %s "
			    "(no route exchange on interface)\n",
			    pi->pi_name);
		}
		return (TIMER_INFINITY);
	}

	switch (event) {
	case SOLICIT_OFF:
		pi->pi_sol_state = NO_SOLICIT;
		check_daemonize();
		return (TIMER_INFINITY);

	case SOLICIT_DONE:
		pi->pi_sol_state = DONE_SOLICIT;
		check_daemonize();
		return (TIMER_INFINITY);

	case START_INIT_SOLICIT:
		if (pi->pi_sol_state == INIT_SOLICIT)
			return (pi->pi_sol_time_left);
		pi->pi_sol_count = ND_MAX_RTR_SOLICITATIONS;
		pi->pi_sol_time_left =
		    GET_RANDOM(0, ND_MAX_RTR_SOLICITATION_DELAY);
		pi->pi_sol_state = INIT_SOLICIT;
		break;

	case SOL_TIMER:
		if (pi->pi_sol_state == NO_SOLICIT)
			return (TIMER_INFINITY);
		/* Decrease time left */
		if (pi->pi_sol_time_left >= elapsed)
			pi->pi_sol_time_left -= elapsed;
		else
			pi->pi_sol_time_left = 0;
		break;
	default:
		logmsg(LOG_ERR, "solicit_event: Unknown event %d\n",
		    (int)event);
		return (TIMER_INFINITY);
	}

	if (pi->pi_sol_time_left != 0)
		return (pi->pi_sol_time_left);

	/* Send solicitation and calculate next time */
	switch (pi->pi_sol_state) {
	case INIT_SOLICIT:
		solicit(&v6allrouters, pi);
		if (--pi->pi_sol_count == 0) {
			pi->pi_sol_state = DONE_SOLICIT;
			check_daemonize();
			return (TIMER_INFINITY);
		}
		pi->pi_sol_time_left = ND_RTR_SOLICITATION_INTERVAL;
		return (pi->pi_sol_time_left);
	case NO_SOLICIT:
	case DONE_SOLICIT:
		return (TIMER_INFINITY);
	default:
		return (pi->pi_sol_time_left);
	}
}

/*
 * Timer mechanism using relative time (in milliseconds) from the
 * previous timer event. Timers exceeding TIMER_INFINITY milliseconds
 * will fire after TIMER_INFINITY milliseconds.
 */
static uint_t timer_previous;	/* When last SIGALRM occurred */
static uint_t timer_next;	/* Currently scheduled timeout */

static void
timer_init(void)
{
	timer_previous = getcurrenttime();
	timer_next = TIMER_INFINITY;
	run_timeouts();
}

/*
 * Make sure the next SIGALRM occurs delay milliseconds from the current
 * time if not earlier.
 * Handles getcurrenttime (32 bit integer holding milliseconds) wraparound
 * by treating differences greater than 0x80000000 as negative.
 */
void
timer_schedule(uint_t delay)
{
	uint_t now;
	struct itimerval itimerval;

	now = getcurrenttime();
	if (debug & D_TIMER) {
		logmsg(LOG_DEBUG, "timer_schedule(%u): now %u next %u\n",
		    delay, now, timer_next);
	}
	/* Will this timer occur before the currently scheduled SIGALRM? */
	if (delay >= timer_next - now) {
		if (debug & D_TIMER) {
			logmsg(LOG_DEBUG, "timer_schedule(%u): no action - "
			    "next in %u ms\n",
			    delay, timer_next - now);
		}
		return;
	}
	if (delay == 0) {
		/* Minimum allowed delay */
		delay = 1;
	}
	timer_next = now + delay;

	itimerval.it_value.tv_sec = delay / 1000;
	itimerval.it_value.tv_usec = (delay % 1000) * 1000;
	itimerval.it_interval.tv_sec = 0;
	itimerval.it_interval.tv_usec = 0;
	if (debug & D_TIMER) {
		logmsg(LOG_DEBUG, "timer_schedule(%u): sec %lu usec %lu\n",
		    delay,
		    itimerval.it_value.tv_sec, itimerval.it_value.tv_usec);
	}
	if (setitimer(ITIMER_REAL, &itimerval, NULL) < 0) {
		logperror("timer_schedule: setitimer");
		exit(2);
	}
}

/*
 * Conditional running of timer. If more than 'minimal_time' millseconds
 * since the timer routines were last run we run them.
 * Used when packets arrive.
 */
static void
conditional_run_timeouts(uint_t minimal_time)
{
	uint_t now;
	uint_t elapsed;

	now = getcurrenttime();
	elapsed = now - timer_previous;
	if (elapsed > minimal_time) {
		if (debug & D_TIMER) {
			logmsg(LOG_DEBUG, "conditional_run_timeouts: "
			    "elapsed %d\n", elapsed);
		}
		run_timeouts();
	}
}

/*
 * Timer has fired.
 * Determine when the next timer event will occur by asking all
 * the timer routines.
 * Should not be called from a timer routine but in some cases this is
 * done because the code doesn't know that e.g. it was called from
 * ifconfig_timer(). In this case the nested run_timeouts will just return but
 * the running run_timeouts will ensure to call all the timer functions by
 * looping once more.
 */
static void
run_timeouts(void)
{
	uint_t now;
	uint_t elapsed;
	uint_t next;
	uint_t nexti;
	struct phyint *pi;
	struct phyint *next_pi;
	struct prefix *pr;
	struct prefix *next_pr;
	struct adv_prefix *adv_pr;
	struct adv_prefix *next_adv_pr;
	struct router *dr;
	struct router *next_dr;
	static boolean_t timeout_running;
	static boolean_t do_retry;

	if (timeout_running) {
		if (debug & D_TIMER)
			logmsg(LOG_DEBUG, "run_timeouts: nested call\n");
		do_retry = _B_TRUE;
		return;
	}
	timeout_running = _B_TRUE;
retry:
	/* How much time since the last time we were called? */
	now = getcurrenttime();
	elapsed = now - timer_previous;
	timer_previous = now;

	if (debug & D_TIMER)
		logmsg(LOG_DEBUG, "run_timeouts: elapsed %d\n", elapsed);

	next = TIMER_INFINITY;
	for (pi = phyints; pi != NULL; pi = next_pi) {
		next_pi = pi->pi_next;
		nexti = phyint_timer(pi, elapsed);
		if (nexti != TIMER_INFINITY && nexti < next)
			next = nexti;
		if (debug & D_TIMER) {
			logmsg(LOG_DEBUG, "run_timeouts (pi %s): %d -> %u ms\n",
			    pi->pi_name, nexti, next);
		}
		for (pr = pi->pi_prefix_list; pr != NULL; pr = next_pr) {
			next_pr = pr->pr_next;
			nexti = prefix_timer(pr, elapsed);
			if (nexti != TIMER_INFINITY && nexti < next)
				next = nexti;
			if (debug & D_TIMER) {
				logmsg(LOG_DEBUG, "run_timeouts (pr %s): "
				    "%d -> %u ms\n", pr->pr_name, nexti, next);
			}
		}
		for (adv_pr = pi->pi_adv_prefix_list; adv_pr != NULL;
		    adv_pr = next_adv_pr) {
			next_adv_pr = adv_pr->adv_pr_next;
			nexti = adv_prefix_timer(adv_pr, elapsed);
			if (nexti != TIMER_INFINITY && nexti < next)
				next = nexti;
			if (debug & D_TIMER) {
				logmsg(LOG_DEBUG, "run_timeouts "
				    "(adv pr on %s): %d -> %u ms\n",
				    adv_pr->adv_pr_physical->pi_name,
				    nexti, next);
			}
		}
		for (dr = pi->pi_router_list; dr != NULL; dr = next_dr) {
			next_dr = dr->dr_next;
			nexti = router_timer(dr, elapsed);
			if (nexti != TIMER_INFINITY && nexti < next)
				next = nexti;
			if (debug & D_TIMER) {
				logmsg(LOG_DEBUG, "run_timeouts (dr): "
				    "%d -> %u ms\n", nexti, next);
			}
		}
		if (pi->pi_TmpAddrsEnabled) {
			nexti = tmptoken_timer(pi, elapsed);
			if (nexti != TIMER_INFINITY && nexti < next)
				next = nexti;
			if (debug & D_TIMER) {
				logmsg(LOG_DEBUG, "run_timeouts (tmp on %s): "
				    "%d -> %u ms\n", pi->pi_name, nexti, next);
			}
		}
	}
	/*
	 * Make sure the timer functions are run at least once
	 * an hour.
	 */
	if (next == TIMER_INFINITY)
		next = 3600 * 1000;	/* 1 hour */

	if (debug & D_TIMER)
		logmsg(LOG_DEBUG, "run_timeouts: %u ms\n", next);
	timer_schedule(next);
	if (do_retry) {
		if (debug & D_TIMER)
			logmsg(LOG_DEBUG, "run_timeouts: retry\n");
		do_retry = _B_FALSE;
		goto retry;
	}
	timeout_running = _B_FALSE;
}

static int eventpipe_read = -1;	/* Used for synchronous signal delivery */
static int eventpipe_write = -1;

/*
 * Ensure that signals are processed synchronously with the rest of
 * the code by just writing a one character signal number on the pipe.
 * The poll loop will pick this up and process the signal event.
 */
static void
sig_handler(int signo)
{
	uchar_t buf = (uchar_t)signo;

	if (eventpipe_write == -1) {
		logmsg(LOG_ERR, "sig_handler: no pipe\n");
		return;
	}
	if (write(eventpipe_write, &buf, sizeof (buf)) < 0)
		logperror("sig_handler: write");
}

/*
 * Pick up a signal "byte" from the pipe and process it.
 */
static void
in_signal(int fd)
{
	uchar_t buf;
	struct phyint *pi;
	struct phyint *next_pi;

	switch (read(fd, &buf, sizeof (buf))) {
	case -1:
		logperror("in_signal: read");
		exit(1);
		/* NOTREACHED */
	case 1:
		break;
	case 0:
		logmsg(LOG_ERR, "in_signal: read eof\n");
		exit(1);
		/* NOTREACHED */
	default:
		logmsg(LOG_ERR, "in_signal: read > 1\n");
		exit(1);
	}

	if (debug & D_TIMER)
		logmsg(LOG_DEBUG, "in_signal() got %d\n", buf);

	switch (buf) {
	case SIGALRM:
		if (debug & D_TIMER) {
			uint_t now = getcurrenttime();

			logmsg(LOG_DEBUG, "in_signal(SIGALRM) delta %u\n",
			    now - timer_next);
		}
		timer_next = TIMER_INFINITY;
		run_timeouts();
		break;
	case SIGHUP:
		/* Re-read config file by exec'ing ourselves */
		for (pi = phyints; pi != NULL; pi = next_pi) {
			next_pi = pi->pi_next;
			if (pi->pi_AdvSendAdvertisements)
				check_to_advertise(pi, START_FINAL_ADV);

			phyint_delete(pi);
		}

		/*
		 * Prevent fd leaks.  Everything gets re-opened at start-up
		 * time.  0, 1, and 2 are closed and re-opened as
		 * /dev/null, so we'll leave those open.
		 */
		closefrom(3);

		logmsg(LOG_ERR, "SIGHUP: restart and reread config file\n");
		(void) execv(argv0[0], argv0);
		(void) unlink(PATH_PID);
		_exit(0177);
		/* NOTREACHED */
	case SIGUSR1:
		logmsg(LOG_DEBUG, "Printing configuration:\n");
		phyint_print_all();
		break;
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		for (pi = phyints; pi != NULL; pi = next_pi) {
			next_pi = pi->pi_next;
			if (pi->pi_AdvSendAdvertisements)
				check_to_advertise(pi, START_FINAL_ADV);

			phyint_delete(pi);
		}
		(void) unlink(PATH_PID);
		logmsg(LOG_ERR, "terminated\n");
		exit(0);
		/* NOTREACHED */
	case 255:
		/*
		 * Special "signal" from looback_ra_enqueue.
		 * Handle any queued loopback router advertisements.
		 */
		loopback_ra_dequeue();
		break;
	default:
		logmsg(LOG_ERR, "in_signal: unknown signal: %d\n", buf);
	}
}

/*
 * Create pipe for signal delivery and set up signal handlers.
 */
static void
setup_eventpipe(void)
{
	int fds[2];
	struct sigaction act;

	if ((pipe(fds)) < 0) {
		logperror("setup_eventpipe: pipe");
		exit(1);
	}
	eventpipe_read = fds[0];
	eventpipe_write = fds[1];
	if (poll_add(eventpipe_read) == -1) {
		exit(1);
	}
	act.sa_handler = sig_handler;
	act.sa_flags = SA_RESTART;
	(void) sigaction(SIGALRM, &act, NULL);

	(void) sigset(SIGHUP, sig_handler);
	(void) sigset(SIGUSR1, sig_handler);
	(void) sigset(SIGTERM, sig_handler);
	(void) sigset(SIGINT, sig_handler);
	(void) sigset(SIGQUIT, sig_handler);
}

/*
 * Create a routing socket for receiving RTM_IFINFO messages and initialize
 * the routing socket message header and as much of the sockaddrs as possible.
 */
static int
setup_rtsock(void)
{
	int s;
	int ret;
	char *cp;
	struct sockaddr_in6 *sin6;

	s = socket(PF_ROUTE, SOCK_RAW, AF_INET6);
	if (s == -1) {
		logperror("socket(PF_ROUTE)");
		exit(1);
	}
	ret = fcntl(s, F_SETFL, O_NDELAY|O_NONBLOCK);
	if (ret < 0) {
		logperror("fcntl(O_NDELAY)");
		exit(1);
	}
	if (poll_add(s) == -1) {
		exit(1);
	}

	/*
	 * Allocate storage for the routing socket message.
	 */
	rt_msg = (struct rt_msghdr *)malloc(NDP_RTM_MSGLEN);
	if (rt_msg == NULL) {
		logperror("malloc");
		exit(1);
	}

	/*
	 * Initialize the routing socket message by zero-filling it and then
	 * setting the fields where are constant through the lifetime of the
	 * process.
	 */
	bzero(rt_msg, NDP_RTM_MSGLEN);
	rt_msg->rtm_msglen = NDP_RTM_MSGLEN;
	rt_msg->rtm_version = RTM_VERSION;
	rt_msg->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK | RTA_IFP;
	rt_msg->rtm_pid = getpid();
	if (rt_msg->rtm_pid < 0) {
		logperror("getpid");
		exit(1);
	}

	/*
	 * The RTA_DST sockaddr does not change during the lifetime of the
	 * process so it can be completely initialized at this time.
	 */
	cp = (char *)rt_msg + sizeof (struct rt_msghdr);
	sin6 = (struct sockaddr_in6 *)cp;
	sin6->sin6_family = AF_INET6;
	sin6->sin6_addr = in6addr_any;

	/*
	 * Initialize the constant portion of the RTA_GATEWAY sockaddr.
	 */
	cp += sizeof (struct sockaddr_in6);
	rta_gateway = (struct sockaddr_in6 *)cp;
	rta_gateway->sin6_family = AF_INET6;

	/*
	 * The RTA_NETMASK sockaddr does not change during the lifetime of the
	 * process so it can be completely initialized at this time.
	 */
	cp += sizeof (struct sockaddr_in6);
	sin6 = (struct sockaddr_in6 *)cp;
	sin6->sin6_family = AF_INET6;
	sin6->sin6_addr = in6addr_any;

	/*
	 * Initialize the constant portion of the RTA_IFP sockaddr.
	 */
	cp += sizeof (struct sockaddr_in6);
	rta_ifp = (struct sockaddr_dl *)cp;
	rta_ifp->sdl_family = AF_LINK;

	return (s);
}

/*
 * Retrieve one routing socket message. If RTM_IFINFO indicates
 * new phyint do a full scan of the interfaces. If RTM_IFINFO
 * indicates an existing phyint only scan that phyint and asociated
 * prefixes.
 */
static void
process_rtsock(int rtsock)
{
	int n;
#define	MSG_SIZE	2048/8
	int64_t msg[MSG_SIZE];
	struct rt_msghdr *rtm;
	struct if_msghdr *ifm;
	struct phyint *pi;
	struct prefix *pr;
	boolean_t need_initifs = _B_FALSE;
	boolean_t need_ifscan = _B_FALSE;
	int64_t	ifscan_msg[10][MSG_SIZE];
	int ifscan_index = 0;
	int i;

	/* Empty the rtsock and coealesce all the work that we have */
	while (ifscan_index < 10) {
		n = read(rtsock, msg, sizeof (msg));
		if (n <= 0) {
			/* No more messages */
			break;
		}
		rtm = (struct rt_msghdr *)msg;
		if (rtm->rtm_version != RTM_VERSION) {
			logmsg(LOG_ERR,
			    "process_rtsock: version %d not understood\n",
			    rtm->rtm_version);
			return;
		}
		switch (rtm->rtm_type) {
		case RTM_NEWADDR:
		case RTM_DELADDR:
			/*
			 * Some logical interface has changed - have to scan
			 * everything to determine what actually changed.
			 */
			if (debug & D_IFSCAN) {
				logmsg(LOG_DEBUG, "process_rtsock: "
				    "message %d\n", rtm->rtm_type);
			}
			need_initifs = _B_TRUE;
			break;
		case RTM_IFINFO:
			need_ifscan = _B_TRUE;
			(void) memcpy(ifscan_msg[ifscan_index], rtm,
			    sizeof (msg));
			ifscan_index++;
			/* Handled below */
			break;
		default:
			/* Not interesting */
			break;
		}
	}
	/*
	 * If we do full scan i.e initifs, we don't need to
	 * scan a particular interface as we should have
	 * done that as part of initifs.
	 */
	if (need_initifs) {
		initifs(_B_FALSE);
		return;
	}

	if (!need_ifscan)
		return;

	for (i = 0; i < ifscan_index; i++) {
		ifm = (struct if_msghdr *)ifscan_msg[i];
		if (debug & D_IFSCAN)
			logmsg(LOG_DEBUG, "process_rtsock: index %d\n",
			    ifm->ifm_index);

		pi = phyint_lookup_on_index(ifm->ifm_index);
		if (pi == NULL) {
			/*
			 * A new physical interface. Do a full scan of the
			 * to catch any new logical interfaces.
			 */
			initifs(_B_FALSE);
			return;
		}

		if (ifm->ifm_flags != pi->pi_flags) {
			if (debug & D_IFSCAN) {
				logmsg(LOG_DEBUG, "process_rtsock: clr for "
				    "%s old flags 0x%x new flags 0x%x\n",
				    pi->pi_name, pi->pi_flags, ifm->ifm_flags);
			}
		}


		/*
		 * Mark the interfaces so that we can find phyints and prefixes
		 * which have disappeared from the kernel.
		 * if_process will set pr_in_use when it finds the
		 * interface in the kernel.
		 * Before re-examining the state of the interfaces,
		 * PI_PRESENT should be cleared from pi_kernel_state.
		 */
		pi->pi_kernel_state &= ~PI_PRESENT;
		for (pr = pi->pi_prefix_list; pr != NULL; pr = pr->pr_next) {
			pr->pr_in_use = _B_FALSE;
		}

		if (ifsock < 0) {
			ifsock = socket(AF_INET6, SOCK_DGRAM, 0);
			if (ifsock < 0) {
				logperror("process_rtsock: socket");
				return;
			}
		}
		if_process(ifsock, pi->pi_name, _B_FALSE);
		for (pr = pi->pi_prefix_list; pr != NULL; pr = pr->pr_next) {
			if_process(ifsock, pr->pr_name, _B_FALSE);
		}
		/*
		 * If interface (still) exists in kernel, set
		 * pi_state to indicate that.
		 */
		if (pi->pi_kernel_state & PI_PRESENT) {
			pi->pi_state |= PI_PRESENT;
		}
		check_if_removed(pi);
		if (show_ifs)
			phyint_print_all();
	}
}

/*
 * Check whether the address formed by pr->pr_prefix and pi_token
 * exists in the kernel. Cannot call SIOCTMYADDR/ONLINK as it
 * does not check for down addresses. This function should not
 * be called for onlink prefixes.
 */
static boolean_t
is_address_present(struct phyint *pi, struct prefix *pr, uint64_t flags)
{
	int s;
	in6_addr_t addr, *token;
	int i;
	int ret;
	struct sockaddr_in6 sin6;

	s = socket(AF_INET6, SOCK_DGRAM, 0);
	if (s < 0) {
		logperror("is_address_present: socket");
		/*
		 * By returning B_TRUE, we make the caller delete
		 * the prefix from the internal table. In the worst
		 * case the next RA will create the prefix.
		 */
		return (_B_TRUE);
	}
	if (flags & IFF_TEMPORARY)
		token = &pi->pi_tmp_token;
	else
		token = &pi->pi_token;
	for (i = 0; i < 16; i++) {
		/*
		 * prefix_create ensures that pr_prefix has all-zero
		 * bits after prefixlen.
		 */
		addr.s6_addr[i] = pr->pr_prefix.s6_addr[i] | token->s6_addr[i];
	}
	(void) memset(&sin6, 0, sizeof (struct sockaddr_in6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_addr = addr;
	ret = bind(s, (struct sockaddr *)&sin6, sizeof (struct sockaddr_in6));
	(void) close(s);
	if (ret < 0 && errno == EADDRNOTAVAIL)
		return (_B_FALSE);
	else
		return (_B_TRUE);
}

/*
 * Look if the phyint or one of its prefixes have been removed from
 * the kernel and take appropriate action.
 * Uses {pi,pr}_in_use.
 */
static void
check_if_removed(struct phyint *pi)
{
	struct prefix *pr;
	struct prefix *next_pr;

	/*
	 * Detect phyints that have been removed from the kernel.
	 * Since we can't recreate it here (would require ifconfig plumb
	 * logic) we just terminate use of that phyint.
	 */
	if (!(pi->pi_kernel_state & PI_PRESENT) &&
	    (pi->pi_state & PI_PRESENT)) {
		logmsg(LOG_ERR, "Interface %s has been removed from kernel. "
		    "in.ndpd will no longer use it\n", pi->pi_name);
		/*
		 * Clear state so that should the phyint reappear
		 * we will start with initial advertisements or
		 * solicitations.
		 */
		phyint_cleanup(pi);
	}
	/*
	 * Detect prefixes which are removed.
	 *
	 * We remove the prefix in all of the following cases :
	 *
	 * 1) Static prefixes are not the ones we create. So,
	 *    just remove it from our tables.
	 *
	 * 2) On-link prefixes potentially move to a different
	 *    phyint during failover. As it does not have
	 *    an address, we can't use the logic in is_address_present
	 *    to detect whether it is present in the kernel or not.
	 *    Thus when it is manually removed we don't recreate it.
	 *
	 * 3) If there is a token mis-match and this prefix is not
	 *    in the kernel, it means we don't need this prefix on
	 *    this interface anymore. It must have been moved to a
	 *    different interface by in.mpathd. This normally
	 *    happens after a failover followed by a failback (or
	 *    another failover) and we re-read the network
	 *    configuration. For the failover from A to B, we would
	 *    have created state on B about A's address, which will
	 *    not be in use after the subsequent failback. So, we
	 *    remove that prefix here.
	 *
	 * 4) If the physical interface is not present, then remove
	 *    the prefix. In the cases where we are advertising
	 *    prefixes, the state is kept in advertisement prefix and
	 *    hence we can delete the prefix.
	 *
	 * 5) Similar to case (3), when we failover from A to B, the
	 *    prefix in A will not be in use as it has been moved to B.
	 *    We will delete it from our tables and recreate it when
	 *    it fails back. is_address_present makes sure that the
	 *    address is still valid in kernel.
	 *
	 * If none of the above is true, we recreate the prefix as it
	 * has been manually removed. We do it only when the interface
	 * is not FAILED or INACTIVE or OFFLINE.
	 */
	for (pr = pi->pi_prefix_list; pr != NULL; pr = next_pr) {
		next_pr = pr->pr_next;
		if (!pr->pr_in_use) {
			/* Clear PR_AUTO and PR_ONLINK */
			pr->pr_kernel_state &= PR_STATIC;
			if ((pr->pr_state & PR_STATIC) ||
			    !(pr->pr_state & PR_AUTO) ||
			    !(prefix_token_match(pi, pr, pr->pr_flags)) ||
			    (!(pi->pi_kernel_state & PI_PRESENT)) ||
			    (is_address_present(pi, pr, pr->pr_flags))) {
				prefix_delete(pr);
			} else if (!(pi->pi_flags &
			    (IFF_FAILED|IFF_INACTIVE|IFF_OFFLINE)) &&
			    pr->pr_state != pr->pr_kernel_state) {
				pr->pr_name[0] = '\0';
				logmsg(LOG_INFO, "Prefix manually removed "
				    "on %s - recreating it!\n",
				    pi->pi_name);
				prefix_update_k(pr);
			}
		}
	}
}


/*
 * Queuing mechanism for router advertisements that are sent by in.ndpd
 * and that also need to be processed by in.ndpd.
 * Uses "signal number" 255 to indicate to the main poll loop
 * that there is something to dequeue and send to incomining_ra().
 */
struct raq {
	struct raq	*raq_next;
	struct phyint	*raq_pi;
	int		raq_packetlen;
	uchar_t		*raq_packet;
};
static struct raq *raq_head = NULL;

/*
 * Allocate a struct raq and memory for the packet.
 * Send signal 255 to have poll dequeue.
 */
static void
loopback_ra_enqueue(struct phyint *pi, struct nd_router_advert *ra, int len)
{
	struct raq *raq;
	struct raq **raqp;

	if (no_loopback)
		return;

	if (debug & D_PKTOUT)
		logmsg(LOG_DEBUG, "loopback_ra_enqueue for %s\n", pi->pi_name);

	raq = calloc(sizeof (struct raq), 1);
	if (raq == NULL) {
		logmsg(LOG_ERR, "loopback_ra_enqueue: out of memory\n");
		return;
	}
	raq->raq_packet = malloc(len);
	if (raq->raq_packet == NULL) {
		free(raq);
		logmsg(LOG_ERR, "loopback_ra_enqueue: out of memory\n");
		return;
	}
	bcopy(ra, raq->raq_packet, len);
	raq->raq_packetlen = len;
	raq->raq_pi = pi;

	/* Tail insert */
	raqp = &raq_head;
	while (*raqp != NULL)
		raqp = &((*raqp)->raq_next);
	*raqp = raq;

	/* Signal for poll loop */
	sig_handler(255);
}

/*
 * Dequeue and process all queued advertisements.
 */
static void
loopback_ra_dequeue(void)
{
	struct sockaddr_in6 from = IN6ADDR_LOOPBACK_INIT;
	struct raq *raq;

	if (debug & D_PKTIN)
		logmsg(LOG_DEBUG, "loopback_ra_dequeue()\n");

	while ((raq = raq_head) != NULL) {
		raq_head = raq->raq_next;
		raq->raq_next = NULL;

		if (debug & D_PKTIN) {
			logmsg(LOG_DEBUG, "loopback_ra_dequeue for %s\n",
			    raq->raq_pi->pi_name);
		}

		incoming_ra(raq->raq_pi,
		    (struct nd_router_advert *)raq->raq_packet,
		    raq->raq_packetlen, &from, _B_TRUE);
		free(raq->raq_packet);
		free(raq);
	}
}


static void
usage(char *cmd)
{
	(void) fprintf(stderr,
	    "usage: %s [ -adt ] [-f <config file>]\n", cmd);
}

int
main(int argc, char *argv[])
{
	int i;
	struct phyint *pi;
	int c;
	char *config_file = PATH_NDPD_CONF;
	boolean_t file_required = _B_FALSE;

	argv0 = argv;
	srandom(gethostid());
	(void) umask(0022);

	while ((c = getopt(argc, argv, "adD:ntIf:")) != EOF) {
		switch (c) {
		case 'a':
			/*
			 * The StatelessAddrConf variable in ndpd.conf, if
			 * present, will override this setting.
			 */
			ifdefaults[I_StatelessAddrConf].cf_value = 0;
			break;
		case 'd':
			debug = D_ALL;
			break;
		case 'D':
			i = strtol((char *)optarg, NULL, 0);
			if (i == 0) {
				(void) fprintf(stderr, "Bad debug flags: %s\n",
				    (char *)optarg);
				exit(1);
			}
			debug |= i;
			break;
		case 'n':
			no_loopback = 1;
			break;
		case 'I':
			show_ifs = 1;
			break;
		case 't':
			debug |= D_PKTIN | D_PKTOUT | D_PKTBAD;
			break;
		case 'f':
			config_file = (char *)optarg;
			file_required = _B_TRUE;
			break;
		case '?':
			usage(argv[0]);
			exit(1);
		}
	}

	if (parse_config(config_file, file_required) == -1)
		exit(2);

	if (show_ifs)
		phyint_print_all();

	if (debug == 0) {
		initlog();
	}

	setup_eventpipe();
	rtsock = setup_rtsock();
	timer_init();
	initifs(_B_TRUE);

	check_daemonize();

	for (;;) {
		if (poll(pollfds, pollfd_num, -1) < 0) {
			if (errno == EINTR)
				continue;
			logperror("main: poll");
			exit(1);
		}
		for (i = 0; i < pollfd_num; i++) {
			if (!(pollfds[i].revents & POLLIN))
				continue;
			if (pollfds[i].fd == eventpipe_read) {
				in_signal(eventpipe_read);
				break;
			}
			if (pollfds[i].fd == rtsock) {
				process_rtsock(rtsock);
				break;
			}
			/*
			 * Run timer routine to advance clock if more than
			 * half a second since the clock was advanced.
			 * This limits CPU usage under severe packet
			 * arrival rates but it creates a slight inaccuracy
			 * in the timer mechanism.
			 */
			conditional_run_timeouts(500U);
			for (pi = phyints; pi != NULL; pi = pi->pi_next) {
				if (pollfds[i].fd == pi->pi_sock) {
					in_data(pi);
					break;
				}
			}
		}
	}
	/* NOTREACHED */
	return (0);
}

/*
 * LOGGER
 */

static boolean_t logging = _B_FALSE;

static void
initlog(void)
{
	logging = _B_TRUE;
	openlog("in.ndpd", LOG_PID | LOG_CONS, LOG_DAEMON);
}

/* Print the date/time without a trailing carridge return */
static void
fprintdate(FILE *file)
{
	char buf[BUFSIZ];
	struct tm tms;
	time_t now;

	now = time(NULL);
	(void) localtime_r(&now, &tms);
	(void) strftime(buf, sizeof (buf), "%h %d %X", &tms);
	(void) fprintf(file, "%s ", buf);
}

/* PRINTFLIKE1 */
void
logmsg(int level, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if (logging) {
		vsyslog(level, fmt, ap);
	} else {
		fprintdate(stderr);
		(void) vfprintf(stderr, fmt, ap);
	}
	va_end(ap);
}

void
logperror(char *str)
{
	if (logging) {
		syslog(LOG_ERR, "%s: %m\n", str);
	} else {
		fprintdate(stderr);
		(void) fprintf(stderr, "%s: %s\n", str, strerror(errno));
	}
}

void
logperror_pi(struct phyint *pi, char *str)
{
	if (logging) {
		syslog(LOG_ERR, "%s (interface %s): %m\n",
		    str, pi->pi_name);
	} else {
		fprintdate(stderr);
		(void) fprintf(stderr, "%s (interface %s): %s\n",
		    str, pi->pi_name, strerror(errno));
	}
}

void
logperror_pr(struct prefix *pr, char *str)
{
	if (logging) {
		syslog(LOG_ERR, "%s (prefix %s if %s): %m\n",
		    str, pr->pr_name, pr->pr_physical->pi_name);
	} else {
		fprintdate(stderr);
		(void) fprintf(stderr, "%s (prefix %s if %s): %s\n",
		    str, pr->pr_name, pr->pr_physical->pi_name,
		    strerror(errno));
	}
}
