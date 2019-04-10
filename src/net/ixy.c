/*
 *  Lowest net device code:
 *    ixy device
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

#define BATCH_SIZE 1

struct netdev* ixy;

const char ixy_addr[] = "0000:02:00.0";
struct ixy_device* ixy_dev;
struct mempool* mempool;
struct pkb_buf* tx_bufs[BATCH_SIZE];
struct pkb_buf* rx_bufs[BATCH_SIZE];


static void ixy_dev_exit(struct netdev* dev)
{
}

static int ixy_dev_init(struct netdev* dev)
{
    dev->net_mtu = 1500;
    dev->net_ipaddr = FAKE_IPADDR;
    dev->net_mask = FAKE_NETMASK;
    hwacpy(dev->net_hwaddr, FAKE_HWADDR);

    const int NUM_BUFS = 2048;
    mempool = memory_allocate_mempool(NUM_BUFS, 0);
    ixy_dev = ixy_init(ixy_addr, 1, 1);
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

static int ixy_xmit(struct netdev* dev, struct pkbuf* pkb)
{
    struct pkt_buf* buf = pkt_buf_alloc(mempool);
    buf->size = pkb->pk_len;
    memcpy(buf->data, pkb->pk_data, pkb->pk_len);
    tx_bufs[0] = buf;
    ixy_tx_batch_busy_wait(ixy_dev, 0, tx_bufs, BATCH_SIZE);
    
    debug("tx_len: %d", pkb->pk_len);
    return pkb->pk_len;
}

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
static int ixy_recv(struct pkbuf* pkb)
{
    uint32_t num_rx = ixy_rx_batch(ixy_dev, 0, rx_bufs, BATCH_SIZE);
    struct pkt_buf* buf = rx_bufs[0];
    pkb->pk_len = buf->size;
    memcpy(pkb->pk_data, buf->data, buf->size);
    pkt_buf_free(buf);

    debug("rx_len: %d", pkb->pk_len);
    return pkb->pk_len;
}


static void ixy_rx()
{
    struct pkbuf* pkb = alloc_netdev_pkb(ixy);

    if  (ixy_recv(pkb) > 0)
        net_in(ixy, pkb);
    else
        free_pkb(pkb);
}

void ixy_poll()
{
    while (true)
    {
        ixy_rx();
    }
}


void ixy_driver_init()
{
    ixy = netdev_alloc("ixy", &ixy_ops);
}

void ixy_driver_exit()
{
    netdev_free(ixy);
}
