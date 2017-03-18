#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
/*
 * Framebuffer VT switch operations
 */

int	vterm;			/* the VT we were started on */
int 	cvt, ocvt;
static int ttyfd = -1;

/* entry points*/
void 	fb_InitVt(void);
int  	fb_CurrentVt(void);
int  	fb_CheckVtChange(void);
void 	fb_RedrawVt(int t);

void
fb_InitVt(void)
{
	ttyfd = open("/dev/tty0", O_RDONLY);
	cvt = ocvt = vterm = fb_CurrentVt();
	/*
	 * Note: this hack is required to get Linux
	 * to orient virtual 0,0 with physical 0,0
	 * I have no idea why this kluge is required...
	 */
	fb_RedrawVt(vterm);
}

/*
 * This function is used to find out what the current active VT is.
 */
int
fb_CurrentVt(void)
{
	struct vt_stat stat;

	ioctl(ttyfd, VT_GETSTATE, &stat);
	return stat.v_active;
}

/*
 * Check if our VT has changed.  Return 1 if so.
 */
int
fb_CheckVtChange(void)
{
	cvt = fb_CurrentVt();
	if(cvt != ocvt && cvt == vterm) {
		ocvt = cvt;
		return 1;
	}
	ocvt = cvt;
	return 0;
}

/*
 * This function is used to cause a redraw of the text console.
 * FIXME: Switching to another console and
 * back works, but that's a really dirty hack
 */
void
fb_RedrawVt(int t)
{
	if(fb_CurrentVt() == vterm) {
		ioctl(ttyfd, VT_ACTIVATE, t == 1 ? 2 : 1); /* Awful hack!!*/
		ioctl(ttyfd, VT_ACTIVATE, t);
	}
}

void
fb_ExitVt(void)
{
	if(ttyfd != -1)
		close(ttyfd);
}
