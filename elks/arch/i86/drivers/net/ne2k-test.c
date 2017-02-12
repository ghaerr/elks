//-----------------------------------------------------------------------------
// NE2K driver - high part - test program
//-----------------------------------------------------------------------------

typedef unsigned char  byte_t;
typedef unsigned short word_t;

#include "ne2k.h"


#define NE2K_LINK_UP    0x0001  // link up
#define NE2K_LINK_FD    0x0002  // full duplex
#define NE2K_LINK_HI    0x0004  // high speed (100 Mbps)
#define NE2K_LINK_LB    0x0008  // loop back

#define NE2K_STAT_RX    0x0001  // packet received
#define NE2K_STAT_TX    0x0002  // packet sent
#define NE2K_STAT_OF    0x0010  // RX ring overflow

#define MAX_PACK        256 * 6

//-----------------------------------------------------------------------------
// Low-level routines
//-----------------------------------------------------------------------------

// From low level test

extern void idle ();
extern void int_setup ();

extern void strncpy (byte_t * dst, byte_t * src, word_t len);
extern word_t strncmp (byte_t * s1, byte_t * s2, word_t len);



//-----------------------------------------------------------------------------
// Local data
//-----------------------------------------------------------------------------

//static word_t phy_regs [32];

//static word_t link_stat;

// MAC address (first byte, bit 0 : broadcast, bit 1 : local)
// so use any address with first bits = 10b

//static byte_t mac_broad [6] = {255, 255, 255, 255, 255, 255};
static byte_t mac_addr  [6] = {2, 0, 0, 0, 0, 1};

static byte_t ip_addr [4] = {192, 168, 0, 200};

static byte_t rx_packet [MAX_PACK];
//static byte_t tx_packet [MAX_PACK];

//static word_t test1;
//static word_t test2;

struct arp_s
	{
	byte_t eth_to   [6];
	byte_t eth_from [6];

	byte_t head     [10];

	byte_t mac_from [6];
	byte_t ip_from  [4];

	byte_t mac_to   [6];
	byte_t ip_to    [4];
	};

typedef struct arp_s arp_t;

static byte_t arp_req [10] = {8, 6, 0, 1, 8, 0, 6, 4, 0, 1};


//-----------------------------------------------------------------------------
// Read all PHY registers
//-----------------------------------------------------------------------------
/*
static word_t test_phy_read ()
	{
	word_t err;
	word_t r;

	for (r = 0; r < 32; r++)
		{
		err = ne2k_phy_get (r, phy_regs + r);
		if (err) break;
		}

	return err;
	}
*/

//-----------------------------------------------------------------------------
// Send a single packet
//-----------------------------------------------------------------------------
/*
static word_t test_tx_single ()
	{
	word_t err;

	while (1)
		{
		int_setup ();

		err = ne2k_init ();
		if (err) break;

		ne2k_addr_set (mac_addr);

		err = ne2k_start ();
		if (err) break;

		tx_packet [0 + 0] = 0xFF;
		tx_packet [0 + 1] = 0xFF;
		tx_packet [0 + 2] = 0xFF;
		tx_packet [0 + 3] = 0xFF;
		tx_packet [0 + 4] = 0xFF;
		tx_packet [0 + 5] = 0xFF;

		tx_packet [6 + 0] = mac_addr [0];
		tx_packet [6 + 1] = mac_addr [1];
		tx_packet [6 + 2] = mac_addr [2];
		tx_packet [6 + 3] = mac_addr [3];
		tx_packet [6 + 4] = mac_addr [4];
		tx_packet [6 + 5] = mac_addr [5];

		tx_packet [12 + 0] = 0x00;
		tx_packet [12 + 1] = 0x55;
		tx_packet [12 + 2] = 0xAA;
		tx_packet [12 + 3] = 0xFF;

		err = ne2k_pack_put (tx_packet, 16);
		if (err) break;

		while (1)
			{
			err = ne2k_status ();
			if (err & NE2K_STAT_TX) break;
			idle ();
			}

		ne2k_stop ();
		ne2k_term ();

		break;
		}

	return err;
	}
*/

//-----------------------------------------------------------------------------
// Receive a single packet
//-----------------------------------------------------------------------------
/*
static word_t test_rx_single ()
	{
	word_t err;

	while (1)
		{
		int_setup ();

		err = ne2k_init ();
		if (err) break;

		ne2k_addr_set (mac_addr);

		err = ne2k_start ();
		if (err) break;

		while (1)
			{
			err = ne2k_status ();
			if (err & NE2K_STAT_RX) break;
			idle ();
			}

		err = ne2k_pack_get (rx_packet);
		if (err) break;

		ne2k_stop ();
		ne2k_term ();

		break;
		}

	return err;
	}
*/

//-----------------------------------------------------------------------------
// ARP responder
//-----------------------------------------------------------------------------

static word_t test_arp ()
	{
	word_t err;

	while (1)
		{
		word_t count = 0;
		arp_t * arp;

		int_setup ();

		err = ne2k_init ();
		if (err) break;

		ne2k_addr_set (mac_addr);

		err = ne2k_start ();
		if (err) break;

		while (count < 100)
			{
			while (1)
				{
				err = ne2k_int_stat ();
				if (err & NE2K_STAT_RX) break;
				idle ();
				}

			err = ne2k_pack_get (rx_packet);
			if (err) break;

			// Filter ARP request

			arp = (arp_t *) (rx_packet + 4);
			err = strncmp (arp->head, arp_req, 10);
			if (err) continue;

			err = strncmp  (arp->ip_to, ip_addr, 4);
			if (err) continue;

			// Fill ARP reply

			arp->head [9] = 2;

			strncpy (arp->eth_to, arp->eth_from, 6);
			strncpy (arp->eth_from, mac_addr, 6);

			strncpy (arp->ip_to, arp->ip_from, 4);
			strncpy (arp->ip_from, ip_addr, 4);

			strncpy (arp->mac_to, arp->mac_from, 6);
			strncpy (arp->mac_from, mac_addr, 6);

			err = ne2k_pack_put (rx_packet + 4, sizeof (arp_t));
			if (err) break;

			count++;
			}

		ne2k_stop ();
		ne2k_term ();

		break;
		}

	return err;
	}


//-----------------------------------------------------------------------------
// Program main procedure
//-----------------------------------------------------------------------------

word_t main ()
	{
	word_t err;

	while (1)
		{
		ne2k_reset ();

		//err = test_phy_read ();
		//if (err) break;

		//link_stat = ne2k_link_stat ();

		//err = test_tx_single ();
		//if (err) break;

		//err = test_rx_single ();
		//if (err) break;

		//strncpy (rx_packet, mac_addr, 6);
		//test1 = strncmp (rx_packet, mac_broad, 6);
		//test2 = strncmp (rx_packet, mac_addr, 6);

		err = test_arp ();
		if (err) break;

		break;
		}

	return err;
	}


//-----------------------------------------------------------------------------
