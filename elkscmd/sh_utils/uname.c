#include <unistd.h>
#include <string.h>
#include <sys/utsname.h>

static void print_element ();

/* Values that are bitwise or'd into toprint'. */
/* Operating system name. */
#define PRINT_SYSNAME 1

/* Node name on a communications network. */
#define PRINT_NODENAME 2

/* Operating system release. */
#define PRINT_RELEASE 4

/* Operating system version. */
#define PRINT_VERSION 8

/* Machine hardware name. */
#define PRINT_MACHINE 16

/* Mask indicating which elements of the name to print. */
static unsigned char toprint;

void
main (argc, argv)
	int argc;
	char **argv;
{
	int	i;
	struct utsname name;

	toprint = 0;

	for (i=1;i<argc;i++)
	{
		switch(argv[i][1])
		{
	        case 's':
        		toprint |= PRINT_SYSNAME;
	        	break;

		case 'n':
          		toprint |= PRINT_NODENAME;
          		break;

	        case 'r':
        		toprint |= PRINT_RELEASE;
          		break;

        	case 'v':
          		toprint |= PRINT_VERSION;
          		break;

        	case 'm':
          		toprint |= PRINT_MACHINE;
          		break;

        	case 'a':
          		toprint = PRINT_SYSNAME | PRINT_NODENAME |
            		PRINT_RELEASE | PRINT_VERSION | PRINT_MACHINE;
          		break;

		default:
			break;
		}
	}
	if (toprint == 0)
		toprint = PRINT_SYSNAME;
	if (uname (&name) != -1)
	{
		print_element (PRINT_SYSNAME, name.sysname);
		print_element (PRINT_NODENAME, name.nodename);
		print_element (PRINT_RELEASE, name.release);
		print_element (PRINT_VERSION, name.version);
		print_element (PRINT_MACHINE, name.machine);
	}

	exit(0);
}

static void
print_element (mask, element)
	unsigned char mask;
	char *element;
{
	if (toprint & mask)
	{
		toprint &= ~mask;
		write(STDOUT_FILENO,element,strlen(element));
		write(STDOUT_FILENO,toprint ? " " : "\n",1);
	}
}

