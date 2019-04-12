/*
 *  Lowest net device code:
 *    virtual net device driver based on tap device
 */
#include "netif.h"
#include "ether.h"
#include "ip.h"
#include "lib.h"
#include "list.h"
#include "netcfg.h"
//#include "tap.h"

#include "stats.h"
#include "log.h"
#include "memory.h"
#include "driver/device.h"

// struct tapdev *tap;
// struct netdev *veth;
struct netdev *ixy;

#define BATCH_SIZE  1
#define PKT_SIZE 60

static const uint8_t pkt_data[] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // dst MAC
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, // src MAC
	0x08, 0x00,                         // ether type: IPv4
	0x45, 0x00,                         // Version, IHL, TOS
	(PKT_SIZE - 14) >> 8,               // ip len excluding ethernet, high byte
	(PKT_SIZE - 14) & 0xFF,             // ip len exlucding ethernet, low byte
	0x00, 0x00, 0x00, 0x00,             // id, flags, fragmentation
	0x40, 0x11, 0x00, 0x00,             // TTL (64), protocol (UDP), checksum
	0x0A, 0x00, 0x00, 0x01,             // src ip (10.0.0.1)
	0x0A, 0x00, 0x00, 0x02,             // dst ip (10.0.0.2)
	0x00, 0x2A, 0x05, 0x39,             // src and dst ports (42 -> 1337)
	(PKT_SIZE - 20 - 14) >> 8,          // udp len excluding ip & ethernet, high byte
	(PKT_SIZE - 20 - 14) & 0xFF,        // udp len exlucding ip & ethernet, low byte
	0x00, 0x00,                         // udp checksum, optional
	'i', 'x', 'y',                       // payload
    ' ', 'h', 'e', 'l', 'l', 'o',
	// rest of the payload is zero-filled because mempools guarantee empty bufs
};

struct ixy_device* ixy_dev;
//tx
struct mempool* mempool;
struct pkt_buf* tx_bufs[BATCH_SIZE];
//rx
struct pkt_buf* rx_bufs[BATCH_SIZE];

// /* Altough dev is already created, this function is safe! */
// static int tap_dev_init(void)
// {
// 	tap = xmalloc(sizeof(*tap));
// 	tap->fd = alloc_tap("tap0");
// 	if (tap->fd < 0)
// 		goto free_tap;
// 	if (setpersist_tap(tap->fd) < 0)
// 		goto close_tap;
// 	/* set tap information */
// 	set_tap();
// 	getname_tap(tap->fd, tap->dev.net_name);
// 	getmtu_tap(tap->dev.net_name, &tap->dev.net_mtu);
// #ifndef CONFIG_TOP1
// 	gethwaddr_tap(tap->fd, tap->dev.net_hwaddr);
// 	setipaddr_tap(tap->dev.net_name, FAKE_TAP_ADDR);
// 	getipaddr_tap(tap->dev.net_name, &tap->dev.net_ipaddr);
// 	setnetmask_tap(tap->dev.net_name, FAKE_TAP_NETMASK);
// 	setup_tap(tap->dev.net_name);
// #endif
// 	unset_tap();
// 	/* Dont add tap device into local net device list */
// 	list_init(&tap->dev.net_list);
// 	return 0;

// close_tap:
// 	close(tap->fd);
// free_tap:
// 	free(tap);
// 	return -1;
// }

// static void veth_dev_exit(struct netdev *dev)
// {
// 	if (dev != veth)
// 		perrx("Net Device Error");
// 	delete_tap(tap->fd);
// }
static void ixy_dev_exit(struct netdev *dev)
{
}


// static int veth_dev_init(struct netdev *dev)
// {
// 	/* init tap: out network nic */
// 	if (tap_dev_init() < 0)
// 		perrx("Cannot init tap device");

// 	/* init veth: information for our netstack */
// 	dev->net_mtu = tap->dev.net_mtu;
// 	dev->net_ipaddr = FAKE_IPADDR;
// 	dev->net_mask = FAKE_NETMASK;
// 	hwacpy(dev->net_hwaddr, FAKE_HWADDR);
// 	dbg("%s ip address: " IPFMT, dev->net_name, ipfmt(dev->net_ipaddr));
// 	dbg("%s hw address: " MACFMT, dev->net_name, macfmt(dev->net_hwaddr));
// 	/* net stats have been zero */
// 	return 0;
// }
static int ixy_dev_init(struct netdev *dev)
{
	/* init veth: information for our netstack */
	dev->net_mtu = 1500;
	dev->net_ipaddr = FAKE_IXY_IPADDR;
	dev->net_mask = FAKE_NETMASK;
	hwacpy(dev->net_hwaddr, FAKE_IXY_HWADDR);

    dbg("ixy init start");
    const int NUM_BUFS = 2048;
	mempool = memory_allocate_mempool(NUM_BUFS, 0);
    char ixy_addr[] = "0000:02:00.0";
	ixy_dev = ixy_init(ixy_addr, 1, 1);
	dbg("ixy init finish");

	dbg("%s ip address: " IPFMT, dev->net_name, ipfmt(dev->net_ipaddr));
	dbg("%s hw address: " MACFMT, dev->net_name, macfmt(dev->net_hwaddr));
	return 0;
}

// static int veth_xmit(struct netdev *dev, struct pkbuf *pkb)
// {
// 	int l;
// 	l = write(tap->fd, pkb->pk_data, pkb->pk_len);
// 	if (l != pkb->pk_len) {
// 		devdbg("write net dev");
// 		dev->net_stats.tx_errors++;
// 	} else {
// 		dev->net_stats.tx_packets++;
// 		dev->net_stats.tx_bytes += l;
// 		devdbg("write net dev size: %d\n", l);
// 	}
// 	return l;
// }
static int ixy_xmit(struct netdev *dev, struct pkbuf *pkb)
{
	struct pkt_buf* pkt = pkt_buf_alloc(mempool);
	pkt->size = pkb->pk_len;
	memcpy(pkt->data, pkb->pk_data, pkb->pk_len);

	tx_bufs[0] = pkt;
	ixy_tx_batch_busy_wait(ixy_dev, 0, tx_bufs, BATCH_SIZE);

	return pkb->pk_len;
}

void ixy_xmit_test()
{
	dbg("ixy_xmit_test");
	struct pkbuf* pkb = alloc_pkb(PKT_SIZE);
	memcpy(pkb->pk_data, pkt_data, sizeof(pkt_data));
	ixy_xmit(ixy, pkb);
	free_pkb(pkb);
}

extern void netdev_tx(struct netdev *dev, struct pkbuf *pkb, int len,
		unsigned short proto, unsigned char *dst);

void netdev_tx_test()
{
	dbg("netdev_tx_test");
	struct pkbuf* pkb = alloc_pkb(PKT_SIZE);
	memcpy(pkb->pk_data, pkt_data, sizeof(pkt_data));
	memset(pkb->pk_data, 0, ETH_HRD_SZ);
	netdev_tx(ixy, pkb, PKT_SIZE -  ETH_HRD_SZ, 0x08, "\x00\x34\x45\x67\x89\xef");

}


// static struct netdev_ops veth_ops = {
// 	.init = veth_dev_init,
// 	.xmit = veth_xmit,
// 	.exit = veth_dev_exit,
// };
static struct netdev_ops ixy_ops = {
	.init = ixy_dev_init,
	.xmit = ixy_xmit,
	.exit = ixy_dev_exit,
};



// static int veth_recv(struct pkbuf *pkb)
// {
// 	int l;
// 	l = read(tap->fd, pkb->pk_data, pkb->pk_len);
// 	if (l <= 0) {
// 		devdbg("read net dev");
// 		veth->net_stats.rx_errors++;
// 	} else {
// 		devdbg("read net dev size: %d\n", l);
// 		veth->net_stats.rx_packets++;
// 		veth->net_stats.rx_bytes += l;
// 		pkb->pk_len = l;
// 	}
// 	return l;
// }
static int ixy_recv(struct pkbuf *pkb)
{
}


// static void veth_rx(void)
// {
// 	// the life start of pkb recved.
// 	struct pkbuf *pkb = alloc_netdev_pkb(veth);
// 	if (veth_recv(pkb) > 0)
// 		net_in(veth, pkb);	/* pass to upper */
// 	else
// 		free_pkb(pkb);
// }
static void ixy_rx(void)
{
	// the life start of pkb recved.
	struct pkbuf *pkb = alloc_netdev_pkb(ixy);
	if (ixy_recv(pkb) > 0)
	{
		net_in(ixy, pkb);	/* pass to upper */
	}
	else
		free_pkb(pkb);	
}


// void veth_poll(void)
// {
// 	struct pollfd pfd = {};
// 	int ret;
// 	while (1) {
// 		pfd.fd = tap->fd;
// 		pfd.events = POLLIN;
// 		pfd.revents = 0;
// 		/* one event, infinite time */
// 		ret = poll(&pfd, 1, -1);
// 		if (ret <= 0)
// 			perrx("poll /dev/net/tun");
// 		/* get a packet and handle it */
// 		veth_rx();
// 	}
// }
void ixy_poll(void)
{
	while (true)
	{
		ixy_rx();
	}
}



// void veth_init(void)
// {
// 	veth = netdev_alloc("veth", &veth_ops);
// }
void ixy_driver_init(void)
{
	ixy = netdev_alloc("ixy", &ixy_ops);
}


// void veth_exit(void)
// {
// 	netdev_free(veth);
// }
void ixy_driver_exit(void)
{
	netdev_free(ixy);
}
