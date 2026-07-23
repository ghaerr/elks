/*
 * AMD PCnet/LANCE Ethernet driver for ELKS
 *
 * Supports ISA LANCE boards (Am7990/79C960, NE1500/NE2100 class) and
 * the register-compatible PCnet-PCI family (Am79C970/970A/973).
 *
 * Configuration is static: the default is I/O 0x300 IRQ 3 (LANCE_PORT/
 * LANCE_IRQ in arch/ports.h) - the fixed resources of the VirtualBox
 * Am79C960 and the NE2100 factory default.  Boards at other addresses
 * are set with le0=irq,port in /bootopts.  There is no PCI config-space
 * probe: a PCnet-PCI card needs le0= set to the BIOS-assigned BAR values
 * AND firmware that enables bus mastering for the card.  QEMU's SeaBIOS
 * does not (verified: '-device pcnet' with le0=11,0xc000 attaches and
 * reads the MAC, but DMA is dead - no packet ever hits the wire), so on
 * QEMU prefer ne2k; this driver targets the VirtualBox/ISA case.
 * QEMU workaround (verified): boot with '-boot order=na' - the bundled
 * iPXE option ROM opens the NIC first, which sets the bus-master bit
 * and leaves it set; after PXE times out and the floppy boots, le0
 * works normally (adds a few seconds per boot).
 *
 * The chip is driven in 16-bit LANCE mode (SWSTYLE 0): all register
 * access through two 16-bit I/O ports (RAP/RDP), descriptor rings and
 * buffers in main memory addressed with 24-bit physical addresses -
 * fully 8086-compatible, no 32-bit instructions anywhere.
 *
 * July 2026 - written for ELKS using the wd/el3 driver framework.
 */

#include <linuxmt/memory.h>
#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/limits.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>
#include <linuxmt/netstat.h>
#include <linuxmt/init.h>

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>
#include <arch/segment.h>
#include <arch/seg286.h>        /* desc_base() to recover physical address */

#include "eth-msgs.h"

/* runtime configuration: ports.h defaults, overridden by le0= in /bootopts */
#define net_irq     (netif_parms[ETH_LANCE].irq)
#define net_port    (netif_parms[ETH_LANCE].port)
#define net_flags   (netif_parms[ETH_LANCE].flags)

/* I/O port offsets (16-bit WIO mode) */
#define PCNET_APROM     0       /* MAC address PROM, bytes 0-5 */
#define PCNET_RDP       0x10    /* CSR data */
#define PCNET_RAP       0x12    /* CSR/BCR address */
#define PCNET_RESET     0x14    /* reading causes s/w reset */
#define PCNET_BDP       0x16    /* BCR data */

/* CSR0 bits */
#define CSR0_INIT       0x0001
#define CSR0_STRT       0x0002
#define CSR0_STOP       0x0004
#define CSR0_TDMD       0x0008
#define CSR0_INEA       0x0040
#define CSR0_INTR       0x0080
#define CSR0_IDON       0x0100
#define CSR0_TINT       0x0200
#define CSR0_RINT       0x0400
#define CSR0_MERR       0x0800
#define CSR0_MISS       0x1000
#define CSR0_CERR       0x2000
#define CSR0_BABL       0x4000
#define CSR0_ANYINTR    (CSR0_BABL|CSR0_CERR|CSR0_MISS|CSR0_MERR|\
                         CSR0_RINT|CSR0_TINT|CSR0_IDON)

/* descriptor word 1 bits (RMD1/TMD1) */
#define DESC_OWN        0x8000
#define DESC_ERR        0x4000
#define DESC_STP        0x0200
#define DESC_ENP        0x0100
#define DESC_HADRMASK   0x00FF

/* ring geometry: 4 RX buffers, 2 TX buffers, 1536 bytes each.
 * RLEN/TLEN are log2 of the ring size, encoded in the init block. */
#define RX_RING_SIZE    4
#define RX_RING_LENBITS 2       /* log2(RX_RING_SIZE) */
#define TX_RING_SIZE    4
#define TX_RING_LENBITS 2       /* log2(TX_RING_SIZE) */
#define BUF_SIZE        1536

/* layout inside the DMA segment (paragraph aligned => 8-byte aligned).
 * TX buffers are NOT here: each is lazily seg_alloc'd on first use of
 * its descriptor and referenced by physical address only. */
#define OFF_INITBLK     0x00    /* 24 bytes */
#define OFF_RXD         0x20    /* 4 descriptors x 8 bytes */
#define OFF_TXD         0x40    /* 4 descriptors x 8 bytes */
#define OFF_RXBUF       0x60
#define DMA_SEG_BYTES   (OFF_RXBUF + RX_RING_SIZE * BUF_SIZE)

/* far pointer to a word inside the DMA segment */
#define DMAW(off)   ((volatile word_t __far *)_MK_FP(dmaseg->base, (off)))

static const char *dev_name = "le0";

static struct wait_queue rxwait;
static struct wait_queue txwait;
static struct netif_stat netif_stat;
static byte_t usecount;
static byte_t found;
static segment_s *dmaseg;       /* rings + buffers, main memory */
static addr_t dmalin;           /* its linear (physical) address */
static unsigned char cur_rx;    /* next RX descriptor to check */
static unsigned char cur_tx;    /* next TX descriptor to fill */

/* TX buffers: one per descriptor, seg_alloc'd on first use of the slot */
static struct {
    segment_s *seg;
    addr_t lin;                 /* physical address for the descriptor */
} txbuf[TX_RING_SIZE];

extern struct eth eths[];

static void pcnet_int(int irq, struct pt_regs *regs);

/* ------------------------------------------------------------------ */
/* register access                                                     */

static word_t NICPROC rcsr(word_t idx)
{
    outw(idx, net_port + PCNET_RAP);
    return inw(net_port + PCNET_RDP);
}

static void NICPROC wcsr(word_t idx, word_t val)
{
    outw(idx, net_port + PCNET_RAP);
    outw(val, net_port + PCNET_RDP);
}

static void NICPROC wbcr(word_t idx, word_t val)
{
    outw(idx, net_port + PCNET_RAP);
    outw(val, net_port + PCNET_BDP);
}

/* ------------------------------------------------------------------ */
/* ring management                                                     */

static void NICPROC pcnet_rxdesc_give(unsigned int i)
{
    addr_t buf = dmalin + OFF_RXBUF + (addr_t)i * BUF_SIZE;
    word_t d = OFF_RXD + (i << 3);

    *DMAW(d + 0) = (word_t)buf;
    *DMAW(d + 4) = 0xF000 | ((-BUF_SIZE) & 0x0FFF);
    *DMAW(d + 6) = 0;
    *DMAW(d + 2) = DESC_OWN | (word_t)((buf >> 16) & DESC_HADRMASK);
}

static void NICPROC pcnet_init_rings(void)
{
    unsigned int i;
    addr_t rdra = dmalin + OFF_RXD;
    addr_t tdra = dmalin + OFF_TXD;
    byte_t *mac = netif_stat.mac_addr;

    for (i = 0; i < RX_RING_SIZE; i++)
        pcnet_rxdesc_give(i);
    cur_rx = 0;

    /* TX descriptors: not owned by chip until a send is queued;
     * buffer address is (re)written per send in pack_put */
    for (i = 0; i < TX_RING_SIZE; i++) {
        word_t d = OFF_TXD + (i << 3);
        *DMAW(d + 0) = 0;
        *DMAW(d + 2) = 0;
        *DMAW(d + 4) = 0xF000;
        *DMAW(d + 6) = 0;
    }
    cur_tx = 0;

    /* init block, 16-bit LANCE layout */
    *DMAW(OFF_INITBLK + 0)  = 0;                            /* MODE: normal */
    *DMAW(OFF_INITBLK + 2)  = mac[0] | ((word_t)mac[1] << 8);
    *DMAW(OFF_INITBLK + 4)  = mac[2] | ((word_t)mac[3] << 8);
    *DMAW(OFF_INITBLK + 6)  = mac[4] | ((word_t)mac[5] << 8);
    *DMAW(OFF_INITBLK + 8)  = 0;                            /* LADRF: no multicast */
    *DMAW(OFF_INITBLK + 10) = 0;
    *DMAW(OFF_INITBLK + 12) = 0;
    *DMAW(OFF_INITBLK + 14) = 0;
    *DMAW(OFF_INITBLK + 16) = (word_t)rdra;
    *DMAW(OFF_INITBLK + 18) = (RX_RING_LENBITS << 13) | (word_t)((rdra >> 16) & 0xFF);
    *DMAW(OFF_INITBLK + 20) = (word_t)tdra;
    *DMAW(OFF_INITBLK + 22) = (TX_RING_LENBITS << 13) | (word_t)((tdra >> 16) & 0xFF);
}

/* ------------------------------------------------------------------ */
/* start / stop                                                        */

static int NICPROC pcnet_start(void)
{
    unsigned int t;

    inw(net_port + PCNET_RESET);        /* s/w reset (WIO read) */
    outw(0, net_port + PCNET_RESET);    /* DWIO-mode escape, harmless in WIO */
    wcsr(0, CSR0_STOP);

    wbcr(20, 0);                        /* SWSTYLE 0 = 16-bit LANCE mode */

    pcnet_init_rings();

    wcsr(1, (word_t)(dmalin + OFF_INITBLK));
    wcsr(2, (word_t)((dmalin + OFF_INITBLK) >> 16) & 0xFF);
    wcsr(0, CSR0_INIT);
    for (t = 0; t < 30000; t++) {
        if (rcsr(0) & CSR0_IDON)
            break;
    }
    if (!(rcsr(0) & CSR0_IDON)) {
        wcsr(0, CSR0_STOP);
        return -EIO;
    }
    wcsr(0, CSR0_IDON | CSR0_STRT | CSR0_INEA);
    return 0;
}

static void NICPROC pcnet_stop(void)
{
    wcsr(0, CSR0_STOP);
}

/* ------------------------------------------------------------------ */
/* packet receive / transmit                                           */

static word_t NICPROC pcnet_rx_stat(void)
{
    return (*DMAW(OFF_RXD + (cur_rx << 3) + 2) & DESC_OWN) ? 0 : 1;
}

static word_t NICPROC pcnet_tx_stat(void)
{
    return (*DMAW(OFF_TXD + (cur_tx << 3) + 2) & DESC_OWN) ? 0 : 1;
}

static size_t NICPROC pcnet_pack_get(char *data, size_t len)
{
    word_t d, stat, mcnt;

    for (;;) {
        d = OFF_RXD + (cur_rx << 3);
        stat = *DMAW(d + 2);
        if (stat & DESC_OWN)
            return -EAGAIN;             /* nothing (left) to read */
        if ((stat & (DESC_ERR | DESC_STP | DESC_ENP)) ==
                    (DESC_STP | DESC_ENP))
            break;                      /* good single-buffer frame */
        /* errored or fragmented frame: recycle and keep looking */
        netif_stat.rx_errors++;
        pcnet_rxdesc_give(cur_rx);
        cur_rx = (cur_rx + 1) & (RX_RING_SIZE - 1);
    }

    mcnt = *DMAW(d + 6) & 0x0FFF;       /* message length incl FCS */
    if (mcnt > 4)
        mcnt -= 4;
    if (len > mcnt)
        len = mcnt;
    fmemcpyb(data, current->t_regs.ds,
             (byte_t *)(OFF_RXBUF + (word_t)cur_rx * BUF_SIZE),
             dmaseg->base, len);
    pcnet_rxdesc_give(cur_rx);
    cur_rx = (cur_rx + 1) & (RX_RING_SIZE - 1);
    return len;
}

static size_t NICPROC pcnet_pack_put(char *data, size_t len)
{
    if (len > MAX_PACKET_ETH)
        len = MAX_PACKET_ETH;
    if (len < 64U)
        len = 64U;                      /* min frame, see issue #133 */

    {
        word_t d = OFF_TXD + (cur_tx << 3);
        addr_t buf;

        if (!txbuf[cur_tx].seg) {       /* first use of this slot: back it */
            segment_s *s = seg_alloc((BUF_SIZE + 15) >> 4, SEG_FLAG_EXTBUF);
            if (!s)
                return -ENOMEM;         /* degrade: caller retries later */
            txbuf[cur_tx].seg = s;
#ifdef CONFIG_286_PMODE
            txbuf[cur_tx].lin = desc_base(s->base);
#else
            txbuf[cur_tx].lin = (addr_t)s->base << 4;
#endif
        }
        buf = txbuf[cur_tx].lin;
        fmemcpyb((byte_t *)0, txbuf[cur_tx].seg->base,
                 data, current->t_regs.ds, len);
        *DMAW(d + 0) = (word_t)buf;     /* buffer assigned at request time */
        *DMAW(d + 4) = 0xF000 | ((-(int)len) & 0x0FFF);
        *DMAW(d + 6) = 0;
        *DMAW(d + 2) = DESC_OWN | DESC_STP | DESC_ENP |
            (word_t)((buf >> 16) & DESC_HADRMASK);
        cur_tx = (cur_tx + 1) & (TX_RING_SIZE - 1);
    }
    wcsr(0, CSR0_INEA | CSR0_TDMD);
    return len;
}

/* ------------------------------------------------------------------ */
/* file operations (framework identical to wd.c)                       */

static size_t pcnet_read(struct inode *inode, struct file *filp,
    char *data, size_t len)
{
    size_t res;

    do {
        prepare_to_wait_interruptible(&rxwait);
        if (!pcnet_rx_stat()) {
            if (filp->f_flags & O_NONBLOCK) {
                res = -EAGAIN;
                break;
            }
            do_wait();
            if (current->signal) {
                res = -EINTR;
                break;
            }
        }
        res = pcnet_pack_get(data, len);
    } while (0);

    finish_wait(&rxwait);
    return res;
}

static size_t pcnet_write(struct inode *inode, struct file *file,
    char *data, size_t len)
{
    size_t res;

    do {
        prepare_to_wait_interruptible(&txwait);
        if (!pcnet_tx_stat()) {
            if (file->f_flags & O_NONBLOCK) {
                res = -EAGAIN;      /* TX ring full; ktcp retries the write */
                break;
            }
            do_wait();
            if (current->signal) {
                res = -EINTR;
                break;
            }
        }
        res = pcnet_pack_put(data, len);
    } while (0);

    finish_wait(&txwait);
    return res;
}

static int pcnet_select(struct inode *inode, struct file *filp, int sel_type)
{
    int res = 0;

    switch (sel_type) {
    case SEL_OUT:
        if (!pcnet_tx_stat()) {
            select_wait(&txwait);
            break;
        }
        res = 1;
        break;
    case SEL_IN:
        if (!pcnet_rx_stat()) {
            select_wait(&rxwait);
            break;
        }
        res = 1;
        break;
    default:
        res = -EINVAL;
    }
    return res;
}

static int pcnet_ioctl(struct inode *inode, struct file *file,
    unsigned int cmd, unsigned int arg)
{
    int err = 0;

    switch (cmd) {
    case IOCTL_ETH_ADDR_GET:
        err = verified_memcpy_tofs((char *)arg, netif_stat.mac_addr, 6U);
        break;
    case IOCTL_ETH_GETSTAT:
        err = verified_memcpy_tofs((char *)arg, &netif_stat,
                                   sizeof(netif_stat));
        break;
    default:
        err = -EINVAL;
    }
    return err;
}

static int pcnet_open(struct inode *inode, struct file *file)
{
    int err = 0;

    do {
        if (!found) {
            err = -ENODEV;
            break;
        }
        if (usecount++)
            break;

        dmaseg = seg_alloc((DMA_SEG_BYTES + 15) >> 4, SEG_FLAG_EXTBUF);
        if (!dmaseg) {
            err = -ENOMEM;
            usecount = 0;
            break;
        }
#ifdef CONFIG_286_PMODE
        dmalin = desc_base(dmaseg->base);
#else
        dmalin = (addr_t)dmaseg->base << 4;
#endif
        err = request_irq(net_irq, pcnet_int, INT_GENERIC);
        if (err) {
            printk(EMSG_IRQERR, dev_name, net_irq, err);
            seg_put(dmaseg);
            dmaseg = NULL;
            usecount = 0;
            break;
        }
        err = pcnet_start();
        if (err) {
            printk(EMSG_ERROR, dev_name, err);
            free_irq(net_irq);
            seg_put(dmaseg);
            dmaseg = NULL;
            usecount = 0;
        }
    } while (0);
    return err;
}

static void pcnet_release(struct inode *inode, struct file *file)
{
    if (--usecount == 0) {
        unsigned int i;

        pcnet_stop();
        free_irq(net_irq);
        seg_put(dmaseg);
        dmaseg = NULL;
        for (i = 0; i < TX_RING_SIZE; i++) {
            if (txbuf[i].seg) {
                seg_put(txbuf[i].seg);
                txbuf[i].seg = NULL;
            }
        }
    }
}

struct file_operations pcnet_fops =
{
    NULL,           /* lseek */
    pcnet_read,
    pcnet_write,
    NULL,           /* readdir */
    pcnet_select,
    pcnet_ioctl,
    pcnet_open,
    pcnet_release
};

/* ------------------------------------------------------------------ */
/* interrupt                                                           */

static void pcnet_int(int irq, struct pt_regs *regs)
{
    word_t csr0;

    while ((csr0 = rcsr(0)) & CSR0_INTR) {
        /* ack everything seen (write-1-to-clear), keep INEA */
        wcsr(0, (csr0 & CSR0_ANYINTR) | CSR0_INEA);
        if (csr0 & CSR0_RINT)
            wake_up(&rxwait);
        if (csr0 & CSR0_TINT) {
            wake_up(&txwait);
        }
        if (csr0 & CSR0_MISS)
            netif_stat.oflow_errors++;
        if (csr0 & (CSR0_BABL | CSR0_CERR))
            netif_stat.tx_errors++;
        if (csr0 & CSR0_MERR) {
            printk(EMSG_TXERR, dev_name, csr0);
            netif_stat.tx_errors++;
        }
    }
}

/* ------------------------------------------------------------------ */
/* probe & init                                                        */

static int INITPROC pcnet_probe(void)
{
    unsigned int u;
    byte_t *mac = netif_stat.mac_addr;

    inw(net_port + PCNET_RESET);        /* WIO s/w reset */
    wcsr(0, CSR0_STOP);
    if (rcsr(0) != CSR0_STOP)           /* fresh-stopped chip reads exactly STOP */
        return -1;
    for (u = 0; u < 6; u++)
        mac[u] = inb(net_port + PCNET_APROM + u);
    /* reject an obviously empty PROM */
    if ((mac[0] | mac[1] | mac[2]) == 0 ||
        (mac[0] & mac[1] & mac[2] & mac[3] & mac[4] & mac[5]) == 0xFF)
        return -1;
    return 0;
}

void INITPROC pcnet_drv_init(void)
{
    unsigned int u;
    byte_t *mac = netif_stat.mac_addr;

    printk("eth: ");
    if (!net_port) {
        printk("%s no I/O port set, use le0= in /bootopts\n", dev_name);
        return;
    }
    printk("%s at 0x%x, irq %d", dev_name, net_port, net_irq);
    if (pcnet_probe()) {
        printk(" not found\n");
        return;
    }
    found = 1;
    printk(", (pcnet) MAC %02X", mac[0]);
    for (u = 1; u < 6; u++)
        printk(":%02X", mac[u]);
    printk(", flags 0x%x\n", net_flags);
    eths[ETH_LANCE].stats = &netif_stat;
}
