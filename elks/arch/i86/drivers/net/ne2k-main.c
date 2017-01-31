//-----------------------------------------------------------------------------
// NE2K driver - high part - test program
//-----------------------------------------------------------------------------

typedef unsigned char  byte_t;
typedef unsigned short word_t;


#define NE2K_LINK_UP    0x0001
#define NE2K_LINK_FULL  0x0002
#define NE2K_LINK_HI    0x0004
#define NE2K_LINK_LOOP  0x0008

#define NE2K_STAT_RX    0x0001
#define NE2K_STAT_TX    0x0002
#define NE2K_STAT_ERR   0x8000


//-----------------------------------------------------------------------------
// Low-level routines
//-----------------------------------------------------------------------------

extern word_t ne2k_phy_get (word_t reg, word_t * val);
extern word_t ne2k_phy_set (word_t reg, word_t val);

extern word_t ne2k_link_stat ();
extern word_t ne2k_link_mode (word_t mode);

extern word_t ne2k_init ();
extern void   ne2k_term ();

extern word_t ne2k_start ();
extern void   ne2k_stop ();

extern void   ne2k_addr_set (byte_t *addr);

extern word_t ne2k_pack_get (byte_t * pack, word_t * len);
extern word_t ne2k_pack_put (byte_t * pack, word_t len);

extern word_t ne2k_stat ();


//-----------------------------------------------------------------------------
// Local data
//-----------------------------------------------------------------------------

static word_t phy_regs [32];

// MAC address (first byte, bit 0 : broadcast, bit 1 : local)
// so use any address with first bits = 10b

static byte_t mac_address [6] = {2, 0, 0, 0, 0, 1};

static byte_t rx_packet [6 * 256];
static byte_t tx_packet [6 * 256];


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

static word_t test_rx_single ()
	{
	word_t err;

	while (1)
		{
		word_t len;

		// TODO: insert INT0 handler

		err = ne2k_init ();
		if (err) break;

		ne2k_addr_set (mac_address);

		err = ne2k_start ();
		if (err) break;

		while (1)
			{
			err = ne2k_stat ();
			if (err & NE2K_STAT_RX) break;
			idle ();
			}

		len = ne2k_pack_get (rx_packet);
		if (!len) break;

		ne2k_stop ();
		ne2k_term ();

		// TODO: remove INT0 handler

		break;
		}

	return err;
	}


//-----------------------------------------------------------------------------

word_t main ()
	{
	word_t err;

	while (1)
		{
		err = test_phy_read ();
		if (err) break;

		err = test_rx_single ();
		if (err) break;

		break;
		}

	return err;
	}


//-----------------------------------------------------------------------------
