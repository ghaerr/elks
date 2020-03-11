//-----------------------------------------------------------------------------
// Ethernet device test
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <linuxmt/ioctl.h>
#include <linuxmt/limits.h>


// These settings are for testing under QEMU

typedef unsigned char byte_t;

static byte_t mac_zero  [6] = {0, 0, 0, 0, 0, 0};
static byte_t mac_addr  [6] = {0x52, 0x55, 10, 0, 2, 15};
static byte_t mac_broad [6] = {255, 255, 255, 255, 255, 255};

static byte_t mac_test  [6];

static byte_t ip_guest [4] = {10, 0, 2, 15};
static byte_t ip_host  [4] = {10, 0, 2, 2};

static byte_t arp_head [10] = {8, 6, 0, 1, 8, 0, 6, 4, 0, 1};

struct arp_s
	{
	byte_t eth_to   [6];
	byte_t eth_from [6];

	byte_t head     [10];

	byte_t mac_from [6];
	byte_t ip_from  [4];

	byte_t mac_to   [6];
	byte_t ip_to    [4];

	byte_t padding  [18];  // pad to 60 bytes
	};

typedef struct arp_s arp_t;

static byte_t arp_buf [MAX_PACKET_ETH];


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main ()
	{
	int res;

	int fd = -1;

	while (1)
		{
		int count = 0;

		// Open Ethernet device

		puts ("Open Ethernet device...");

		fd = open ("/dev/eth", O_RDWR);
		if (fd < 0)
			{
			perror ("open /dev/eth");
			res = 1;
			break;
			}

		// Test set & get MAC address

		res = ioctl (fd, IOCTL_ETH_ADDR_SET, mac_addr);
		if (res)
			{
			perror ("ioctl /dev/eth addr_set");
			break;
			}

		res = ioctl (fd, IOCTL_ETH_ADDR_GET, mac_test);
		if (res)
			{
			perror ("ioctl /dev/eth addr_get");
			break;
			}

		if (!memcmp (mac_addr, mac_test, 6))
			{
			puts ("address mismatch");
			res = 1;
			}

		// Cycle on ARP request & reply

		while (count < 1000)
			{
			arp_t * arp;
			int m;

			fd_set rx_fds;
			fd_set tx_fds;

			FD_ZERO (&rx_fds);
			FD_ZERO (&tx_fds);

			FD_SET  (fd, &rx_fds);
			FD_SET  (fd, &tx_fds);

			res = select (fd + 1, &rx_fds, &tx_fds, NULL, NULL);
			if (res < 0)
				{
				perror ("select /dev/eth");
				res = 1;
				break;
				}

			if (FD_ISSET (fd, &rx_fds))
				{
				// Get ARP reply

				puts ("Receive ARP reply...");

				res = read (fd, arp_buf, MAX_PACKET_ETH);
				if (res < 0)
					{
					perror ("read /dev/eth");
					res = 1;
					break;
					}

				// Filter ARP reply

				arp_head [9] = 2;

				arp = (arp_t *) arp_buf;

				res = memcmp (arp->head, arp_head, 10);
				if (res)
					{
					printf ("Not an ARP reply\n");
					continue;
					}

				res = memcmp  (arp->ip_to, ip_guest, 4);
				if (res)
					{
					printf ("Other destination IP\n");
					continue;
					}

				res = memcmp  (arp->ip_from, ip_host, 4);
				if (res)
					{
					printf ("Other source IP\n");
					continue;
					}

				printf ("MAC of host is: ");

				for (m = 0; m < 6; m++)
					{
					printf ("%x", arp->mac_from [m]);
					if (m < 5) putchar (':');
					}

				putchar ('\n');
				}

			if (FD_ISSET (fd, &tx_fds))
				{
				// Send ARP request

				arp_head [9] = 1;

				arp = (arp_t *) arp_buf;

				memcpy (arp->eth_to, mac_broad, 6);
				memcpy (arp->eth_from, mac_addr, 6);

				memcpy (arp->head, arp_head, 10);

				memcpy (arp->ip_to, ip_host, 4);
				memcpy (arp->ip_from, ip_guest, 4);

				memcpy (arp->mac_to, mac_zero, 6);
				memcpy (arp->mac_from, mac_addr, 6);

				puts ("Send ARP request...");

				res = write (fd, arp_buf, sizeof (arp_t));
				if (res < 0)
					{
					perror ("write /dev/eth");
					res = 1;
					break;
					}
				}

			count++;
			}

		res = 0;
		break;
		}

	if (fd >= 0)
		{
		close (fd);
		}

	return res;
	}


//-----------------------------------------------------------------------------
