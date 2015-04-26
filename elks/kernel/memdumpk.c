#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>

void memdumpk(__u16 seg,__u16 off,size_t len)
{
    char buffer[17], ch;
    size_t i;

    buffer[16] = '\0';
    while (len > 0) {
	for (i=0; i<16; i++)
	    buffer[i] = (char) peekb(seg,off++);
	printk("%x:%x ", seg, off);
	for (i=0; i<16; i++) {
	    printk(" ");
	    switch ((ch = buffer[i] / 16)) {
	    case 7:
		if (ch == 127)
		    buffer[i] = '.';
	    case 2:
	    case 3:
	    case 4:
	    case 5:
	    case 6:
		printk("%X",ch);
		break;
	    case 0:
		printk("0");
	    default:
		printk("%X",ch);
		buffer[i] = '.';
		break;
	    }
	}
	printk("  '%s'\n",buffer);
	len -= 8;
    }
}
