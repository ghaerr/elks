//-----------------------------------------------------------------------------
// NE2K driver - high part - test program
//-----------------------------------------------------------------------------

typedef unsigned char  byte_t;
typedef unsigned short word_t;


// Low-level routines

extern void   ne2k_mac_start ();
extern void   ne2k_mac_stop ();

extern word_t ne2k_init ();
extern void   ne2k_term ();

extern void   ne2k_addr_set (byte_t *addr);
extern word_t ne2k_pack_get (byte_t *pack);

extern word_t ne2k_rx_stat ();


// MAC address (first byte, bit 0 : broadcast, bit 1 : local)
// so use any address with first bits = 10b

static  byte_t mac_address [6] = { 2, 0, 0, 0, 0, 1 };

static byte_t rx_packet [6 * 256];


void main ()
	{
	while (1)
		{
		word_t err;
		word_t len;

		// TODO: insert INT0 handler

		err = ne2k_init ();
		if (err) break;

		ne2k_addr_set (mac_address);

		ne2k_mac_start ();

		while (1)
			{
			err = ne2k_rx_stat ();
			if (err) break;
			idle ();
			}

		len = ne2k_pack_get (rx_packet);
		if (!len) break;

		ne2k_term ();

		// TODO: remove INT0 handler

		break;
		}
	}


//-----------------------------------------------------------------------------
