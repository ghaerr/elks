/* Define items that may vary from one configuration to another.
 * These are as follows:
 *
 *	DISK_SIZE	The size of the flash disk we are writing to
 *			in Kilobytes.
 *
 *	DRIVE		The drive letter the source file is on.
 *
 *	FILENAME	The filename to assume for the source file.
 *
 * These last two are put together in the code to create a string similar
 * to the following...
 *
 *	loc::m:\\minix.dsk
 *
 * ...which would be produced for the following definitions...
 *
 *	DRIVE		'm'
 *	FILENAME	"minix.dsk"
 */

#define DISK_SIZE	128

#define DRIVE		'm'

#define FILENAME	"minix.dsk"
