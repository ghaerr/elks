/*
 * Hex display function
 *
 * void hexdump(unsigned char *addr, int count, int summary, char *prefix)
 */
#include <stdio.h>
#include <string.h>
#include "config.h"

#if DEBUG_TCPDATA

#define printf DPRINTF
#define isprint(c) ((c) > ' ' && (c) <= '~')

static int lastnum[16] = {-1};
static int lastaddr = -1;

static void
printline(int address, int *num, char *chr, int count, int summary, char *prefix)
{
	int   j;

	if (lastaddr >= 0)
	{
		for (j = 0; j < count; j++)
			if (num[j] != lastnum[j])
				break;
		if (j == 16 && summary)
		{
			if (lastaddr + 16 == address)
			{
				printf("*\n");
			}
			return;
		}
	}

	lastaddr = address;
	printf("%s%04x:", prefix, address);
	for (j = 0; j < count; j++)
	{
		if (j == 8)
			printf(" ");
		if (num[j] >= 0)
			printf(" %02x", num[j]);
		else
			printf("   ");
		lastnum[j] = num[j];
		num[j] = -1;
	}

	for (j=count; j < 16; j++)
	{
		if (j == 8)
			printf(" ");
		printf("   ");
	}

	printf("  ");
	for (j = 0; j < count; j++)
		printf("%c", chr[j]);
	printf("\n");
}


void hexdump(unsigned char *addr, int count, int summary, char *prefix)
{
	int offset;
	char buf[20];
	int num[16];

	if (!prefix) prefix = "";
	for (offset = 0; count > 0; count -= 16, offset += 16)
	{
		int j, ch;

		memset(buf, 0, 16);
		for (j = 0; j < 16; j++)
			num[j] = -1;
		for (j = 0; j < 16; j++)
		{
			ch = *addr++;

			num[j] = ch;
			if (isprint(ch) && ch < 0x80)
				buf[j] = ch;
			else
				buf[j] = '.';
		}
		printline(offset, num, buf, count > 16? 16: count, summary, prefix);
	}
}
#endif /* DEBUG_TCPDATA*/
