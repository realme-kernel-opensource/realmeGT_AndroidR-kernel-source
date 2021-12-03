/************************************************************************************
** File: - oppo_dhcp.c
** VENDOR_EDIT
** Copyright (C), 2008-2020, OPPO Mobile Comm Corp., Ltd
**
** Description:
**		1. Add for dhcp conflict
**
** Version: 1.0
** Date :	2020-05-09
** Author:	LianGenglin@CONNECTIVITY.WIFI.INTERNET
** TAG	:	OPLUS_FEATURE_WIFI_DHCP
**
** ---------------------Revision History: ---------------------
**	<author>					  <data>	 <version >   <desc>
** ---------------------------------------------------------------
**	LianGenglin@CONNECTIVITY.WIFI.INTERNET	2020/05/09	1.0			 build this module
**	LiYi@CONNECTIVITY.WIFI.INTERNET			2020/06/01	2.0			 modify this module
**
************************************************************************************/
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/sysctl.h>
#include <net/route.h>
#include <net/ip.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/version.h>
#include <net/tcp.h>
#include <linux/random.h>
#include <net/sock.h>
#include <net/dst.h>
#include <linux/file.h>
#include <net/tcp_states.h>
#include <linux/netlink.h>
#include <net/sch_generic.h>
#include <net/pkt_sched.h>
#include <net/netfilter/nf_queue.h>
#include <linux/netfilter/xt_state.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_owner.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>
#include <trace/hooks/vh_af_packet.h>

/* packet ops */
#define BOOTP_REQUEST	1
#define BOOTP_REPLY	2

/* DHCP message types */
#define DHCPDISCOVER	1
#define DHCPOFFER		2
#define DHCPREQUEST		3
#define DHCPDECLINE		4
#define DHCPACK			5
#define DHCPNAK			6
#define DHCPRELEASE		7
#define DHCPINFORM		8

#define NETLINK_OPLUS_DHCP 38

#define NONE cpu_to_be32(INADDR_NONE)
#define ANY cpu_to_be32(INADDR_ANY)

#define DUP_SERVER_COUNT 2
#define IFACE_COUNT 2
#define OPPO_DHCP_MAX_SERVERS 4

#define MAIN_WLAN_IFACE "wlan0"
#define SECOND_WLAN_IFACE "wlan1"

#define MAIN_WLAN_INDEX 0
#define SECOND_WLAN_INDEX 1

#define LOG_TAG "[oppo_dhcp] %s line:%d "
#define debug(fmt,args...) printk(LOG_TAG fmt,__FUNCTION__,__LINE__,##args)

#define IP_MASK(addr) ((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2]

#define MAC_MASK(addr) addr[0], \
	addr[1], \
	addr[4], \
	addr[5]


extern int (*handle_dhcp)(struct sock *sk, struct sk_buff *skb, struct net_device *dev, struct packet_type *pt);
static bool oppo_dhcp_get_notify_state(struct net_device *dev);

/*NLMSG_MIN_TYPE is 0x10,so we start at 0x11*/
enum{
	OPPO_DHCP_SET_ANDROID_PID	 = 0x11,
	OPPO_DHCP_L2CONNECTED_CMD	 =0x12,
	OPPO_DHCP_NOTIFY_DUP_OFFER_EVENT	= 0x13,
	OPPO_DHCP_INTERNET_ACCESS_CMD	= 0x14,
	OPPO_DHCP_L2DISCONNECTED_CMD = 0x15,
	OPPO_DHCP_MAX = 0x16,
};

/* dhcp packet */
struct dhcp_pkt {	   /* BOOTP packet format */
	struct iphdr iph;	/* IP header */
	struct udphdr udph; /* UDP header */
	u8 op;			/* 1=request, 2=reply */
	u8 htype;		/* HW address type */
	u8 hlen;		/* HW address length */
	u8 hops;		/* Used only by gateways */
	__be32 xid;		/* Transaction ID */
	__be16 secs;		/* Seconds since we started */
	__be16 flags;		/* Just what it says */
	__be32 client_ip;		/* Client's IP address if known */
	__be32 your_ip;		/* Assigned IP address */
	__be32 server_ip;		/* (Next, e.g. NFS) Server's IP address */
	__be32 relay_ip;		/* IP address of BOOTP relay */
	u8 hw_addr[16];		/* Client's HW address */
	u8 serv_name[64];	/* Server host name */
	u8 boot_file[128];	/* Name of boot file */
	u8 exten[312];		/* DHCP options / BOOTP vendor extensions */
};


struct dhcp_server {
	int wlan_index; // -1 for unset item
	u8 server_mac[6];
	unsigned int server_addr;
};

/*
 * black list and white list offer, we only need one for each interface
 * so we got one for each interface index
 */
struct dhcp_server oppo_dhcp_drop_list[IFACE_COUNT];
struct dhcp_server oppo_dhcp_expect_list[IFACE_COUNT];
static bool do_notify[IFACE_COUNT] = {false};

static DEFINE_MUTEX(oppo_dhcp_netlink_mutex);
static struct ctl_table_header *oppo_dhcp_hooks_table_hrd;

static rwlock_t dhcp_hooks_lock;

static u32 dhcp_hooks_debug = 0;

// user space pid
static u32 oppo_dhcp_pid = 0;

//kernel sock
static struct sock *oppo_dhcp_sock;

/* send to user space */
static int oppo_dhcp_send_to_user(int msg_type, char *payload, int payload_len)
{
	int ret = 0;
	struct sk_buff *skbuff;
	struct nlmsghdr *nlh;

	/*allocate new buffer cache */
	skbuff = alloc_skb(NLMSG_SPACE(payload_len), GFP_ATOMIC);
	if (skbuff == NULL) {
		printk("oppo_nf_hooks_netlink: skbuff alloc_skb failed\n");
		return -1;
	}

	/* fill in the data structure */
	nlh = nlmsg_put(skbuff, 0, 0, msg_type, NLMSG_ALIGN(payload_len), 0);
	if (nlh == NULL) {
		printk("oppo_nf_hooks_netlink:nlmsg_put failaure\n");
		nlmsg_free(skbuff);
		return -1;

	}

	//compute nlmsg length
	nlh->nlmsg_len = NLMSG_HDRLEN + NLMSG_ALIGN(payload_len);

	if(NULL != payload){
		memcpy((char *)NLMSG_DATA(nlh),payload,payload_len);
	}

	/* set control field,sender's pid */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0))
	NETLINK_CB(skbuff).pid = 0;
#else
	NETLINK_CB(skbuff).portid = 0;
#endif

	NETLINK_CB(skbuff).dst_group = 0;

	/* send data */
	if(oppo_dhcp_pid){
		ret = netlink_unicast(oppo_dhcp_sock, skbuff, oppo_dhcp_pid, MSG_DONTWAIT);
	} else {
		printk(KERN_ERR "oppo_dhcp_hooks_netlink: can not unicast skbuff, oppo_dhcp_pid=0\n");
		kfree_skb(skbuff);
	}
	if(ret < 0){
		printk(KERN_ERR "oppo_dhcp_hooks_netlink: can not unicast skbuff,ret = %d\n", ret);
		return 1;
	}

	return 0;
}

struct notify_offer {
	u32 index;
	u32 server_addr;
	char server_mac[ETH_ALEN];
};

static void send_dhcp_offer_packet_to_userspace(char *wlan_iface, __be32 server_addr, char *server_mac) {
	struct notify_offer payload;
	int index = MAIN_WLAN_INDEX;

	debug("%s, %u.%u.%u.***, %02x:%02x:***:***:%02x:%02x\n", wlan_iface, IP_MASK(server_addr), MAC_MASK(server_mac));
	if (wlan_iface != NULL) {
		if (0 == memcmp(wlan_iface, MAIN_WLAN_IFACE,strlen(MAIN_WLAN_IFACE))) {
			index = MAIN_WLAN_INDEX;
		} else if (0 == memcmp(wlan_iface, SECOND_WLAN_IFACE, strlen(SECOND_WLAN_IFACE))) {
			index = SECOND_WLAN_INDEX;
		}
	}
	payload.index = index;
	payload.server_addr = server_addr;
	memcpy(payload.server_mac, server_mac, ETH_ALEN);
	oppo_dhcp_send_to_user(OPPO_DHCP_NOTIFY_DUP_OFFER_EVENT,(char *)&payload,sizeof(payload));
}

/*
 * get wlan index, return -1 for not found
 */
static int get_iface_index(char *dev_name) {
	if (0 == memcmp(dev_name, MAIN_WLAN_IFACE,strlen(MAIN_WLAN_IFACE))) {
		return MAIN_WLAN_INDEX;
	}
	if (0 == memcmp(dev_name, SECOND_WLAN_IFACE, strlen(SECOND_WLAN_IFACE))) {
		return SECOND_WLAN_INDEX;
	}
	return -1;  // not found
}

/*
 * @return true, drop packet
 * @return false, allow packet
 */
static bool blackwhite_list_match(struct net_device *dev, __be32 server_id) {
	bool is_list_empty = true;
	int wlan_index;

	wlan_index = get_iface_index(dev->name);
	// ignore all pkts come from interface which is not in our list
	if (wlan_index < 0 || wlan_index >= IFACE_COUNT) {
		return false;
	}

	read_lock_bh(&dhcp_hooks_lock);
	// white list first
	if (oppo_dhcp_expect_list[wlan_index].wlan_index >= 0) {
		is_list_empty = false;
		if (oppo_dhcp_expect_list[wlan_index].server_addr == server_id) {
			debug("allow white address %u.%u.%u.*** on %s\n", IP_MASK(server_id), dev->name);
			read_unlock_bh(&dhcp_hooks_lock);
			return false;
		}
	}
	// white list not empty, drop all pkts not matched
	if (!is_list_empty) {
		read_unlock_bh(&dhcp_hooks_lock);
		return true;
	}
	// then blacklist, only drop ptks in it
	if (oppo_dhcp_drop_list[wlan_index].wlan_index >= 0) {
		if (oppo_dhcp_drop_list[wlan_index].server_addr == server_id) {
			debug("drop black address %u.%u.%u.*** on %s\n", IP_MASK(server_id), dev->name);
			read_unlock_bh(&dhcp_hooks_lock);
			return true;
		}
	}
	read_unlock_bh(&dhcp_hooks_lock);
	return false;
}

static void handle_dhcp_packet(void *data, struct sock *sk, struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, int *do_drop) {
	struct dhcp_pkt *b;
	struct iphdr *iph;
	int len, ext_len;

	/*
	 * To be more fast, We first ignore non-raw socket and
	 * non-ETH_P_IP packet types, such as ipv6
	 * Note: ETH_P_IP should be sync with raw socket in DhcpClient.java
	 */
	if (sk->sk_type != SOCK_RAW || pt->type != htons(ETH_P_IP))
		goto ignore;

	b = (struct dhcp_pkt *)skb_network_header(skb);
	iph = &b->iph;
	if (iph->ihl != 5 || iph->version != 4 || iph->protocol != IPPROTO_UDP)
		goto ignore;

	/* Fragments are not supported */
	if (ip_is_fragment(iph)) {
		net_err_ratelimited("DHCP/BOOTP: Ignoring fragmented reply\n");
		goto ignore;
	}

	if (skb->len < ntohs(iph->tot_len))
		goto ignore;

	if (ip_fast_csum((char *) iph, iph->ihl))
		goto ignore;

	if (b->udph.source != htons(67) || b->udph.dest != htons(68))
		goto ignore;

	if (ntohs(iph->tot_len) < ntohs(b->udph.len) + sizeof(struct iphdr))
		goto ignore;

	len = ntohs(b->udph.len) - sizeof(struct udphdr);
	ext_len = len - (sizeof(*b) -
		sizeof(struct iphdr) -
		sizeof(struct udphdr) -
		sizeof(b->exten));
	if (ext_len < 0)
		goto ignore;

	/* Ok the front looks good, make sure we can get at the rest.  */
	if (!pskb_may_pull(skb, skb->len))
		goto ignore;

	b = (struct dhcp_pkt *)skb_network_header(skb);
	iph = &b->iph;

	if (b->op != BOOTP_REPLY) {
		debug("handle_dhcp_packet: not A reply message");
		goto ignore;
	}

	/* Parse extensions */
	if (ext_len >= 4 ) {
		u8 *end = (u8 *) b + ntohs(b->iph.tot_len);
		u8 *ext;
		__be32 server_id = NONE;
		int mt = 0;

		ext = &b->exten[4];
		while (ext < end && *ext != 0xff) {
			u8 *opt = ext++;
			if (*opt == 0)	/* Padding */
				continue;
			ext += *ext + 1;
			if (ext >= end)
				break;
			switch (*opt) {
			case 53:	/* Message type */
				if (opt[1])
					mt = opt[2];
				break;
			case 54:	/* Server ID (IP address) */
				if (opt[1] >= 4)
					memcpy(&server_id, opt + 2, 4);
				break;
			}
		}

		switch (mt) {
		case DHCPOFFER:
		case DHCPNAK:
		case DHCPACK:
			if (memcmp(dev->dev_addr,b->hw_addr,dev->addr_len) != 0) {
				debug("this packet is not for us");
				goto ignore;
			}

			// notify offers
			if (oppo_dhcp_get_notify_state(dev) && (mt == DHCPOFFER || mt == DHCPACK)) {
				char server_mac[ETH_ALEN] = {0};

				memcpy(server_mac, eth_hdr(skb)->h_source, ETH_ALEN);
				send_dhcp_offer_packet_to_userspace(dev->name, server_id, server_mac);
			}
			if (blackwhite_list_match(dev, server_id)) {
				*do_drop = 1;
				return;
			}
			break;
		default:
			break;
		}
	}
ignore:
	*do_drop  = 0;
	return;

}

/*
 * record mode, ignore expected offer list and drop offer list
 * just notify offers to user space
 */
static int oppo_dhcp_start_notify_offers(u32 if_index)
{
	if (if_index >= IFACE_COUNT)
		return 0;

	write_lock_bh(&dhcp_hooks_lock);
	do_notify[if_index] = true;
	write_unlock_bh(&dhcp_hooks_lock);
	return 0;
}

static bool oppo_dhcp_get_notify_state(struct net_device *dev) {
	int wlan_index;
	bool state;

	wlan_index = get_iface_index(dev->name);
	// ignore all pkts come from interface which is not in our list
	if (wlan_index < 0 || wlan_index >= IFACE_COUNT) {
		return false;
	}
	read_lock_bh(&dhcp_hooks_lock);
	state = do_notify[wlan_index];
	read_unlock_bh(&dhcp_hooks_lock);
	return state;
}
static int oppo_dhcp_stop_notify_offers(u32 if_index)
{
	if (if_index >= IFACE_COUNT)
		return 0;

	write_lock_bh(&dhcp_hooks_lock);
	do_notify[if_index] = false;
	write_unlock_bh(&dhcp_hooks_lock);
	return 0;
}

/*
 * @param clear: set if we need clear the list on interface @if_index
 */
static int oppo_dhcp_set_drop_offer(u32 if_index, __be32 offer, bool clear)
{
	if (if_index >= IFACE_COUNT)
		return 0;

	write_lock_bh(&dhcp_hooks_lock);
	if (clear) {
		oppo_dhcp_drop_list[if_index].wlan_index = -1;
		oppo_dhcp_drop_list[if_index].server_addr = 0;
		write_unlock_bh(&dhcp_hooks_lock);
		return 0;
	}
	oppo_dhcp_drop_list[if_index].wlan_index = if_index;
	oppo_dhcp_drop_list[if_index].server_addr = offer;
	write_unlock_bh(&dhcp_hooks_lock);
	return 0;
}


static int oppo_dhcp_set_expect_offer(u32 if_index, __be32 offer, bool clear)
{
	if (if_index >= IFACE_COUNT)
		return 0;

	write_lock_bh(&dhcp_hooks_lock);
	if (clear) {
		oppo_dhcp_expect_list[if_index].wlan_index = -1;
		oppo_dhcp_expect_list[if_index].server_addr = 0;
		write_unlock_bh(&dhcp_hooks_lock);
		return 0;
	}
	oppo_dhcp_expect_list[if_index].wlan_index = if_index;
	oppo_dhcp_expect_list[if_index].server_addr = offer;
	write_unlock_bh(&dhcp_hooks_lock);
	return 0;
}


static int oppo_dhcp_set_android_pid(struct sk_buff *skb)
{
	oppo_dhcp_pid = NETLINK_CB(skb).portid;
	debug("oppo_dhcp_set_android_pid pid=%d\n", oppo_dhcp_pid);
	return 0;
}

static int oppo_dhcp_l2connected_cmd(struct nlmsghdr *nlh)
{
	u32 *data = (u32 *)NLMSG_DATA(nlh);
	u32 index = data[0];
	u32 dup_dhcp = data[1];
	__be32 server_addr = data[2];

	debug("iface_index = %u, dup_dhcp = %u, server_addr=%u.%u.%u.***\n", index, dup_dhcp, IP_MASK(server_addr));
	oppo_dhcp_start_notify_offers(index);
	if (!dup_dhcp) {
		oppo_dhcp_set_drop_offer(index, 0, true);
		oppo_dhcp_set_expect_offer(index, 0, true);
	} else {
		oppo_dhcp_set_drop_offer(index, 0, true);
		oppo_dhcp_set_expect_offer(index, server_addr, false);
	}
	return 0;
}

static int oppo_dhcp_internet_access_cmd(struct nlmsghdr *nlh)
{
	u32 *data = (u32 *)NLMSG_DATA(nlh);
	u32 index = data[0];
	u32 internet_access = data[1];
	__be32 server_addr = data[2];

	debug("iface_index = %u, internet_access = %u, server_addr=%u.%u.%u.***\n", index, internet_access, IP_MASK(server_addr));
	// no internet access, so drop address
	if (!internet_access) {
		oppo_dhcp_start_notify_offers(index);
		oppo_dhcp_set_expect_offer(index, 0, true);
		oppo_dhcp_set_drop_offer(index, server_addr, false);
	} else {
		// we have internet access, stops everything
		oppo_dhcp_stop_notify_offers(index);
		oppo_dhcp_set_drop_offer(index, 0, true);
		oppo_dhcp_set_expect_offer(index, 0, true);
	}
	return 0;
}

static int oppo_dhcp_l2disconnected_cmd(struct nlmsghdr *nlh)
{
	u32 *data = (u32 *)NLMSG_DATA(nlh);
	u32 index = data[0];

	oppo_dhcp_stop_notify_offers(index);
	oppo_dhcp_set_drop_offer(index, 0, true);
	oppo_dhcp_set_expect_offer(index, 0, true);

	return 0;
}

static int oppo_dhcp_netlink_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh, struct netlink_ext_ack *extack)
{
	int ret = 0;
	u32 portid = NETLINK_CB(skb).portid;

	if (nlh->nlmsg_type == OPPO_DHCP_SET_ANDROID_PID) {
		return oppo_dhcp_set_android_pid(skb);
	}
	// only recv msg from target pid
	if (portid != oppo_dhcp_pid) {
		return ret;
	}
	switch (nlh->nlmsg_type) {
	case OPPO_DHCP_L2CONNECTED_CMD:
		ret = oppo_dhcp_l2connected_cmd(nlh);
		break;
	case OPPO_DHCP_L2DISCONNECTED_CMD:
		ret = oppo_dhcp_l2disconnected_cmd(nlh);
		break;
	case OPPO_DHCP_INTERNET_ACCESS_CMD:
		ret = oppo_dhcp_internet_access_cmd(nlh);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}


static void oppo_dhcp_netlink_rcv(struct sk_buff *skb)
{
	mutex_lock(&oppo_dhcp_netlink_mutex);
	netlink_rcv_skb(skb, &oppo_dhcp_netlink_rcv_msg);
	mutex_unlock(&oppo_dhcp_netlink_mutex);
}

static int oppo_dhcp_netlink_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.input	= oppo_dhcp_netlink_rcv,
	};

	oppo_dhcp_sock = netlink_kernel_create(&init_net, NETLINK_OPLUS_DHCP, &cfg);
	return oppo_dhcp_sock == NULL ? -ENOMEM : 0;
}

static void oppo_dhcp_netlink_exit(void)
{
	netlink_kernel_release(oppo_dhcp_sock);
	oppo_dhcp_sock = NULL;
}


static struct ctl_table oppo_dhcp_hooks_sysctl_table[] = {
	{
		.procname	= "dhcp_hooks_debug",
		.data		= &dhcp_hooks_debug,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{ }
};

static int oppo_dhcp_hooks_sysctl_init(void)
{
	oppo_dhcp_hooks_table_hrd = register_net_sysctl(&init_net, "net/oppo_dhcp_hooks",
												  oppo_dhcp_hooks_sysctl_table);
	return oppo_dhcp_hooks_table_hrd == NULL ? -ENOMEM : 0;
}

static void oppo_dhcp_hooks_sysctl_fini(void)
{
	if(oppo_dhcp_hooks_table_hrd){
		unregister_net_sysctl_table(oppo_dhcp_hooks_table_hrd);
		oppo_dhcp_hooks_table_hrd = NULL;
	}
}

static int __init oppo_dhcp_init(void)
{
	int ret = 0;
	int i;
	rwlock_init(&dhcp_hooks_lock);

	memset(oppo_dhcp_drop_list, 0, sizeof(struct dhcp_server) * IFACE_COUNT);
	memset(oppo_dhcp_expect_list, 0, sizeof(struct dhcp_server) * IFACE_COUNT);
	/* set wlan_index = -1 to init black white list */
	for(i = 0; i < IFACE_COUNT; i++) {
		oppo_dhcp_set_drop_offer(i, 0, true);
		oppo_dhcp_set_expect_offer(i, 0, true);
		oppo_dhcp_stop_notify_offers(i);
	}

	ret = oppo_dhcp_netlink_init();
	if (ret < 0) {
		debug("oppo_dhcp_init module failed to init netlink.\n");
		goto out;
	}

	ret = oppo_dhcp_hooks_sysctl_init();
	if (ret < 0) {
		debug("oppo_dhcp_init module failed to init sysctl.\n");
		goto error_uninit_netlink;
	}

	ret = register_trace_android_vh_check_dhcp_pkt(handle_dhcp_packet, NULL);
	if (ret != 0) {
		pr_err("register_trace_android_vh_check_dhcp_pkt failed! ret=%d\n", ret);
		goto error_uninit_sysctl;
	}
	return ret;

error_uninit_sysctl:
	oppo_dhcp_hooks_sysctl_fini();
error_uninit_netlink:
	oppo_dhcp_netlink_exit();
out:
	return ret;
}

static void __exit oppo_dhcp_fini(void)
{
	unregister_trace_android_vh_check_dhcp_pkt(handle_dhcp_packet, NULL);

	oppo_dhcp_hooks_sysctl_fini();

	oppo_dhcp_netlink_exit();
}

module_init(oppo_dhcp_init);
module_exit(oppo_dhcp_fini);
MODULE_LICENSE("GPL v2");
