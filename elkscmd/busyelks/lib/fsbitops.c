#ifndef __linux__
#define volatile
#endif

unsigned char set_bit(unsigned int nr, void * add)
{
	unsigned int * addr = add;
	int	mask, retval;

	addr += nr >> 4;
	mask = 1 << (nr & 0x0f);
	retval = (mask & *addr) != 0;
	*addr |= mask;
	return retval;
}

unsigned char clear_bit(unsigned int nr, void * add)
{
	unsigned int * addr = add;
	int	mask, retval;

	addr += nr >> 4;
	mask = 1 << (nr & 0x0f);
	retval = (mask & *addr) != 0;
	*addr &= ~mask;
	return retval;
}

unsigned char test_bit(unsigned int nr, void * add)
{
	unsigned int * addr = add;
	unsigned int	mask;
	int retval;

	addr += nr >> 4;
	mask = 1 << (nr & 0x0f);
	retval = ((mask & *addr) != 0L);
	return retval;
}

