#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

//Requirement 3.2   /* TIMER DEVICE DRIVER */
#define DEVICE_NAME "dev_driver"
#define DEVICE_PATH "/dev/dev_driver"
#define DEVICE_MAJOR_NUMBER 242

/*
  _IO(int type, int number): Type, Number만 전달하는 단순한 ioctl
  _IOR(int type, int number, data_type): Device Driver에서 Read()
  _IOW(int type, int number, data_type): Device Driver에서 Write()
  _IORW(int type, int number, data_type): Device Driver에서 Read/Write
 */

#define IOCTL_SEND_QUERY _IOW(DEVICE_MAJOR_NUMBER, 0, char*)
#define IOCTL_REQUEST _IO(DEVICE_MAJOR_NUMBER, 0)

//
static int __init driver_init(void);
static void __exit driver_exit(void);

static struct file_operations driver_fops = {
	.open = driver_open,
	.release = driver_release,
	.unlocked_ioctl = driver_ioctl,
};

static int driver_open(struct inode* inode, struct file* file) {
	return 0;
}

static int driver_release(struct inode* inode, struct file* file) {
	return 0;
}

static long driver_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
	switch (cmd) {
		case IOCTL_REQUEST:

			break;

		case IOCTL_SEND_QUERY:

			break;

		default: 

			break;
	}
}