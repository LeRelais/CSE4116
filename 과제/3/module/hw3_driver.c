#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <mach/gpio.h>

#define DEVICE_PATH "/dev/stopwatch"
#define DEVICE_MAJOR_NUMBER 242

#define IOCTL_START _IO(DEVICE_MAJOR_NUMBER, 0)

static int __init driver_init(void);
static void __exit driver_exit(void);

static long driver_ioctl(struct file*, unsigned int, unsigned long);
static int driver_open(struct inode*, struct file*);
static int driver_release(struct inode*, struct file*);

enum _INTER_TYPE {

};

static struct file_operations driver_fops = {
	.open = driver_open,
	.release = driver_release,
	.unlocked_ioctl = driver_ioctl,
}

static irqreturn_t button_hanlder(int irq, void* dev_id) {

}

static int driver_open(struct inode* inode, struct file* file) {
	int ret;
	int irq;

	printk("driver_open()\n");

	/* Install GPIO handler */

	/* HOME button */
	gpio_direction_input(IMX_GPIO_NR(1, 11));
	irq = gpio_to_irq(IMX_GPIO_NR(1, 11));
	ret = request_irq(irq, button_handler, IRQF_TRIGGER_RISING, "HOME", 0);
	
	/* VOL+ button */
	gpio_direction_input(IMX_GPIO_NR(2, 15));
	irq = gpio_to_irq(IMX_GPIO_NR(2, 15));
	ret = request_irq(irq, button_handler, IRQF_TRIGGER_RISING, "VOLUP", 0);

	/* VOL- button */
	gpio_direction_input(IMX_GPIO_NR(5, 14));
	irq = gpio_to_irq(IMX_GPIO_NR(5, 14));
	ret = request_irq(irq, button_handler, IRQF_TRIGGER_RISING, "VOLDOWN", 0);

	/* BACK button */
	gpio_direction_input(IMX_GPIO_NR(1, 12));
	irq = gpio_to_irq(IMX_GPIO_NR(1, 12));
	ret = request_irq(irq, button_handler, IRQF_TRIGGER_RISING, "BACK", 0);

	/* Install end */

	return 0;
}

MODULE_LICENSE("GPL");

module_init(driver_init);
module_exit(driver_exit);