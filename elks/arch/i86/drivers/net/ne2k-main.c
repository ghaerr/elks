//-----------------------------------------------------------------------------
// NE2K driver - high part - test program
//-----------------------------------------------------------------------------

typedef unsigned char  byte_t;
typedef unsigned short word_t;


#define NE2K_LINK_UP    0x0001  // link up
#define NE2K_LINK_FD    0x0002  // full duplex
#define NE2K_LINK_HI    0x0004  // high speed (100 Mbps)
#define NE2K_LINK_LB    0x0008  // loop back

#define NE2K_STAT_RX    0x0001
#define NE2K_STAT_TX    0x0002
#define NE2K_STAT_ERR   0x8000

#define MAX_PACK        256 * 6


//-----------------------------------------------------------------------------
// Low-level routines
//-----------------------------------------------------------------------------

extern word_t ne2k_phy_get (word_t reg, word_t * val);
extern word_t ne2k_phy_set (word_t reg, word_t val);

extern word_t ne2k_link_stat ();
//extern word_t ne2k_link_mode (word_t mode);

extern void   ne2k_int_setup ();

extern word_t ne2k_init ();
extern void   ne2k_term ();
extern void   ne2k_reset ();

extern word_t ne2k_start ();
extern void   ne2k_stop ();

extern void   ne2k_addr_set (byte_t *addr);

extern word_t ne2k_mem_test (byte_t * pack, word_t len);

extern word_t ne2k_pack_get (byte_t * pack, word_t * len);
extern word_t ne2k_pack_put (byte_t * pack, word_t len);

extern word_t ne2k_status ();


//-----------------------------------------------------------------------------
// Local data
//-----------------------------------------------------------------------------

static word_t phy_regs [32];

static word_t link_stat;
static word_t mem_stat;

// MAC address (first byte, bit 0 : broadcast, bit 1 : local)
// so use any address with first bits = 10b

static byte_t mac_addr [6] = {2, 0, 0, 0, 0, 1};

static byte_t rx_packet [MAX_PACK];
static word_t rx_len;

static byte_t tx_packet [MAX_PACK];


//-----------------------------------------------------------------------------
// Read all PHY registers
//-----------------------------------------------------------------------------

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


//-----------------------------------------------------------------------------
// Test TX internal memory
//-----------------------------------------------------------------------------

static word_t test_tx_mem ()
	{
	word_t err;

	err = ne2k_mem_test (tx_packet, MAX_PACK);
	mem_stat = err;

	return err;
	}


//-----------------------------------------------------------------------------
// Send a single packet
//-----------------------------------------------------------------------------

static word_t test_tx_single ()
	{
	word_t err;

	while (1)
		{
		word_t len;

		ne2k_int_setup ();

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


//-----------------------------------------------------------------------------
// Receive a single packet
//-----------------------------------------------------------------------------

static word_t test_rx_single ()
	{
	word_t err;

	while (1)
		{
		word_t len;

		ne2k_int_setup ();

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

		err = ne2k_pack_get (rx_packet, &rx_len);
		if (err) break;

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

		link_stat = ne2k_link_stat ();

		//err = test_tx_mem ();
		//if (err) break;

		err = test_tx_single ();
		if (err) break;

		//err = test_rx_single ();
		//if (err) break;

		break;
		}

	return err;
	}


//-----------------------------------------------------------------------------
