#include <linuxmt/cgatext.h>
#include <linuxmt/errno.h>
#include <linuxmt/cgatext.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <arch/cgatext.h>

static size_t
free_len(struct file * f, size_t len)
{
  if(f->f_pos >= cgatext_mem_SIZE)
    return 0;

  if(f->f_pos + len > cgatext_mem_SIZE)
    return cgatext_mem_SIZE - f->f_pos;

  return len;
}

int
cgatext_lseek(struct inode *inode, struct file *f, loff_t offset,
  unsigned int origin)
{
  switch (origin)
  {
    case 0: /* SEEK_SET */
      break;

    case 1: /* SEEK_CUR */
      offset += f->f_pos;
      break;

    case 2: /* SEEK_END */
      offset = cgatext_mem_SIZE - offset;
      break;

    default:
      return -EINVAL;
  }

  if(offset < 0)
    offset = 0;
  else if(offset > cgatext_mem_SIZE)
    offset = cgatext_mem_SIZE;

  f->f_pos = offset;

  return 0;
}

size_t
cgatext_read(struct inode *inode, struct file *f, char *data, size_t len)
{
  len = free_len(f, len);

  if(len)
  {
    fmemcpyb
      ((byte_t *)data, current->t_regs.ds,
      (byte_t *)f->f_pos, (seg_t)cgatext_mem_SEG,
      (word_t)len);

    f->f_pos += len;
  }

  return len;
}

size_t
cgatext_write(struct inode *inode, register struct file *f, char *data, size_t len)
{
  len = free_len(f, len);

  if(len)
  {
    fmemcpyb(
      (byte_t *)f->f_pos, (seg_t)cgatext_mem_SEG,
      (byte_t *)data, current->t_regs.ds,
      (word_t)len);

    f->f_pos += len;
  }

  return len;
}

static struct file_operations cgatext_fops =
{
  cgatext_lseek,	/* lseek */
  cgatext_read,		/* read */
  cgatext_write,	/* write */
  NULL,	/* readdir */
  NULL,	/* select */
  NULL,	/* ioctl */
  NULL,	/* open */
  NULL,	/* release */
#ifdef BLOAT_FS
  NULL,	/* fsync */
  NULL,	/* check_media_change */
  NULL,	/* revalidate */
#endif
};

void
cgatext_init(void)
{
  int i;

  i = register_chrdev(CGATEXT_MAJOR, CGATEXT_DEVICE_NAME, &cgatext_fops);
  if(i)
	  printk("cgatext: unable to register: %d\n", i);
}
