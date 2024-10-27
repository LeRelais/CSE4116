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
#include <linux/sched.h>
#include <mach/gpio.h>

#define DEVICE_NAME "stopwatch"
#define DEVICE_PATH "/dev/stopwatch"
#define DEVICE_MAJOR_NUMBER 242

//IOCTL request.
#define IOCTL_START _IO(DEVICE_MAJOR_NUMBER, 0)

//Physical address for FPGA modules 
#define FPGA_FND_ADDR 0x08000004
#define FPGA_DOT_ADDR 0x08000210

//FPGA_DOT, char array responsible for printing 0 ~ 9 on FPGA DOT 
unsigned char fpga_number[10][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};

unsigned char fpga_set_full[10] = {
	// memset(array,0x7e,sizeof(array));
	0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f
};

//char array responsible for blank FPGA DOT 
unsigned char fpga_set_blank[10] = {
	// memset(array,0x00,sizeof(array));
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*
	driver_init() : initialize driver for aissgnment #3 and register it
*/
static int __init driver_init(void);

/*
	driver_exit() : unregister driver and free memory previously occupied by driver
*/
static void __exit driver_exit(void);

/*
	driver_ioctl() : handle ioctl request based on cmd
*/
static long driver_ioctl(struct file*, unsigned int, unsigned long);

/*
	driver_open() : open driver
*/
static int driver_open(struct inode*, struct file*);

/*
	driver_release() : close driver
*/
static int driver_release(struct inode*, struct file*);

/*
	fpga_fnd_init() : set fnd according to content of str
*/
void fpga_fnd_init(char *str);

/*
	button_handler() : when irq sparks, handle irq request (top half).
*/
static irqreturn_t button_handler(int irq, void* dev_id);

/*
	stopwatch_blink(): function that will be called when timer expires
*/
static void stopwatch_blink(unsigned long data);

/*
	exit_blink() : function that will be called when exit timer expires
*/
static void exit_blink(unsigned long data);

/*
	status_reset() : reset data used in driver
*/
static void status_reset(void);

/*
	fpga_dot_init() : set FPGA DOT acording to content of value
*/
void fpga_dot_init(int value);

/*
	fpga_dot_clear() : wipe out FPGA DOT
*/
void fpga_dot_clear();

//wait queue declaration. Enable process to change state to SLEEP
wait_queue_head_t wait_queue;
DECLARE_WAIT_QUEUE_HEAD(wait_queue);

static int irqs[4];
static char fnd_str[5];

char *fpga_fnd;
char *fpga_dot;

bool already_opened;

static struct file_operations driver_fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_release,
	.unlocked_ioctl = driver_ioctl,
};


/*
	DATA data structure : data structure that will be used by implemented stopwatch.
*/
typedef struct _data{
	int minute;
	int second;
	int millisec;

	struct timer_list timer; 
	struct timer_list exit_timer;

	long unsigned int prev_irq_time;
	int prev_irq;

	bool started;
	bool paused;
	bool quit;
	bool reset;
	bool timer_exit_executed;
}DATA;

DATA stopwatch;

/*
	TASKLET_DATA data structure : implement bottom half logic by using tasklet
*/
typedef struct _tasklet_data{
	int irq;
} TASKLET_DATA;

TASKLET_DATA task_data;

/*
	fpga_fnd_init() : with content passed from parameter *str, adjust content to right format
					  and write to fnd module's physical memory using outw()
*/
void fpga_fnd_init(char *str){
	//printk("setFND called\n");
	unsigned short int value_short = 0;
	unsigned char value[5];
	char tmp;
	int i = 0, prefix = 12;

	for(i = 0; i < 4; i++){
		tmp = str[i] >= '0' ? str[i] - '0' : str[i];
		value_short |= tmp << prefix;
		prefix -= 4;
	}

	// printk("fnd init : %s\n", str);
	// value_short = value[0] << 12 | value[1] << 8 || value[2] << 4 | value[3];

	outw(value_short, fpga_fnd);
}

/*
	fpga_dot_init() : with content passed from parameter *str, adjust content to right format
					  and write to dot module's physical memory using outw()
*/
void fpga_dot_init(int value){
	int i = 0;
	unsigned short int _s_value;
	
	value = value >= '0' ? value - '0' : value;

	for(i = 0; i < 10; i++){
		_s_value = fpga_number[value][i] & 0x7F;
		outw(_s_value, (unsigned int)fpga_dot + i * 2);
	}
}

/*
	fpga_dot_clear() : using fpga_set_blank, wipe out content of dot module
					   with outw()
*/
void fpga_dot_clear(){
	int i = 0;
	unsigned short int _s_value;

	for(i = 0; i < 10; i++){
		_s_value = fpga_set_blank[i] & 0x7F;
		outw(_s_value, (unsigned int)fpga_dot + i * 2);
	}
}

/*
	status_rest() : reset the data used by implemented stopwatch to initial state
*/
static void status_reset(void){
	stopwatch.minute = 0;
	stopwatch.second = 0;
	stopwatch.millisec = 0;

	stopwatch.quit = false;
	stopwatch.started = false;
	stopwatch.paused = false; 
	stopwatch.timer_exit_executed = false; 

	char fnd[5] = "0000";
	fpga_fnd_init(fnd);
	fpga_dot_init(0);
}

/*
	driver_init() : register device and map memories of FPGA modules
*/
static int __init driver_init(void) {
	printk("driver_init()\n");
	int ret = register_chrdev(DEVICE_MAJOR_NUMBER, DEVICE_NAME, &driver_fops);
	if(ret != 0){
		printk("register error\n");
		return ret;
	}

	/* Physical mapping for FND */
	fpga_fnd = ioremap(FPGA_FND_ADDR, 0x02);
	if (fpga_fnd == NULL) {
		printk("failed to ioremap fpga_fnd\n");
		return -1;
	}
	fpga_dot = ioremap(FPGA_DOT_ADDR, 0x10);
	if(fpga_dot == NULL){
		printk(KERN_ERR "physical mapping unsuccessful.\n");
		return -1;
	}
	//status_reset();
	//fpga_dot_clear();

	return 0;
}

/*
	driver_exit() : unmap physical memories dedicated to FPGA modules and unregister device
*/
static void __exit driver_exit(void) {
	printk("driver_exit()\n");
	iounmap(fpga_fnd);
	iounmap(fpga_dot);
	unregister_chrdev(DEVICE_MAJOR_NUMBER, DEVICE_NAME);
}

/*
	stopwatch_proceed() : logic of stopwatch progression per millisecond
*/
static void stopwatch_proceed(void){
	//printk("stopwatch proceed()\n");
	stopwatch.millisec += 1;

	if(stopwatch.millisec == 10){
		stopwatch.millisec -= 10;
		stopwatch.second += 1;

		if(stopwatch.second == 60){
			stopwatch.second -= 60;
			stopwatch.minute += 1;
		}
	}

	char fnd[5];
	sprintf(fnd, "%02d%02d", stopwatch.minute, stopwatch.second);
	fpga_fnd_init(fnd);
	fpga_dot_init(stopwatch.millisec);
}

/*
	stopwatch_blink() : function that will be executed when stopwatch timer expires.
						update stopwatch information by using stopwatch_proceed().
*/
static void stopwatch_blink(unsigned long data){
	stopwatch_proceed();

	stopwatch.timer.expires = get_jiffies_64() + HZ / 10; //0.1 sec interval
	stopwatch.timer.function = stopwatch_blink;
	add_timer(&stopwatch.timer);
}

/*
	exit_blink() : function that will be executed when exit timer expires
				   wake up currently sleeping process. 
*/
static void exit_blink(unsigned long data){
	printk("exit_blink()");
	del_timer(&stopwatch.timer);
	status_reset();
	fpga_dot_clear();
	__wake_up(&wait_queue, 1, 1, NULL);
	stopwatch.quit = false;
	stopwatch.timer_exit_executed = true; 
}

/*
	stopwatch_set() : initialize stopwatch information
*/
static void stopwatch_set(){
	//printk("stopwatch_set()\n");
	init_timer(&stopwatch.timer);
	//printk("stopwatch_set() 2\n");
	stopwatch.timer.expires = get_jiffies_64() + HZ / 10; //0.1 sec interval
	stopwatch.timer.function = stopwatch_blink;
	//printk("stopwatch_set() 3\n");
	stopwatch.started = true;
	add_timer(&stopwatch.timer);
}

/*
	button_handler_bottom() : implement bottom-half logic of interrupt handling.
							  in bottom half, schedule works which are not time-sensitive by using tasklet.
							  tasklets are not executed currently on same CPU. 
*/
static void button_handler_bottom(unsigned long data){
	TASKLET_DATA *tmp = (TASKLET_DATA *)data;
	
	if(tmp->irq == irqs[0]){ //HOME

	}
	else if(tmp->irq == irqs[1]){ //VOLUP, reset FPGA module contents, set stopwatch to initial state 
		stopwatch.reset = false;
		status_reset();
		stopwatch.reset == true ? printk("1\n") : printk("0\n");
		del_timer(&stopwatch.timer);
	}
	else if(tmp->irq == irqs[2]){ //VOLDOWN, add exit timer. if it is keep pressed, update timer. otherwise, delete exit timer. 
		if(stopwatch.quit){
			init_timer(&stopwatch.exit_timer);
			stopwatch.exit_timer.expires = get_jiffies_64() + HZ/10 * 30;
			stopwatch.exit_timer.function = exit_blink;
			add_timer(&stopwatch.exit_timer);
		}
		else{ //delete exit timer
			printk("delete exit timer\n");
			del_timer(&stopwatch.exit_timer);
		}
	}
	else{ //BACK,  if stopwatch is executed, pause/unpause stopwatch when BACK input is given 
		if(stopwatch.started){  
			stopwatch.paused == true ? del_timer(&stopwatch.timer) : stopwatch_set(); //if true, 
		}
	}
} 

//MACRO for TASKLET execution, execute button_handler_bottom with using tasklet_proc as data
static DECLARE_TASKLET(tasklet_proc, button_handler_bottom, (unsigned long)&task_data);

/*
	button_handler() : handler to handle interrupt
*/
static irqreturn_t button_handler(int irq, void* dev_id) {
	unsigned long cur_press_time = get_jiffies_64();
	
	/*
		logic to avoid same irq to be requested in short period of time.
		if requested irq is other than VOLDOWN irq, then if interval between previous request and current request
		is less than 0.3 sec, do no handle.
	*/
	if(irq == stopwatch.prev_irq && irq != irqs[3]){
		if(cur_press_time - stopwatch.prev_irq_time < (HZ / 10) * 3) return;
	}

	stopwatch.prev_irq = irq;
	stopwatch.prev_irq_time = get_jiffies_64();
	task_data.irq = irq; 
	
	if(irq == irqs[0]){  //HOME
		printk("HOME KEY pressed\n");
		if(!stopwatch.started)
			stopwatch_set();
		//setFND("1111");
	}
	else if(irq == irqs[1]){  //VOL+
		printk("VOLUP pressed\n");
		stopwatch.reset = true;
	}
	else if(irq == irqs[2]){  //VOL-
		printk("VOLDOWN pressed\n");
		stopwatch.quit = !stopwatch.quit;

	}
	else{				//BACK
		if(stopwatch.started){
			printk("BACK pressed\n");
			stopwatch.paused = !stopwatch.paused;
		}
	}

	tasklet_schedule(&tasklet_proc);

	return IRQ_HANDLED;
}

/*
	driver_open() : install interrupt (IRQ)
*/
static int driver_open(struct inode* inode, struct file* file) {
	int ret;
	int irq;

	printk("driver_open()\n");

	if(already_opened) return -1;
	already_opened = true;

	/* Install GPIO handler */

	/* HOME button */
	gpio_direction_input(IMX_GPIO_NR(1, 11));
	irqs[0] = gpio_to_irq(IMX_GPIO_NR(1, 11));
	ret = request_irq(irqs[0], button_handler, IRQF_TRIGGER_RISING, "HOME", 0);
	
	/* VOL+ button */
	gpio_direction_input(IMX_GPIO_NR(2, 15));
	irqs[1] = gpio_to_irq(IMX_GPIO_NR(2, 15));
	ret = request_irq(irqs[1], button_handler, IRQF_TRIGGER_RISING, "VOLUP", 0);

	/* VOL- button */
	gpio_direction_input(IMX_GPIO_NR(5, 14));
	irqs[2] = gpio_to_irq(IMX_GPIO_NR(5, 14));
	ret = request_irq(irqs[2], button_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "VOLDOWN", 0);

	/* BACK button */
	gpio_direction_input(IMX_GPIO_NR(1, 12));
	irqs[3] = gpio_to_irq(IMX_GPIO_NR(1, 12));
	ret = request_irq(irqs[3], button_handler, IRQF_TRIGGER_RISING, "BACK", 0);

	/* Install end */

	status_reset();

	return 0;
}

/*
	driver_release() : free installed IRQ, drop driver
*/
static int driver_release(struct inode* inode, struct file* file) {
	printk("driver_release()\n");
	already_opened = false;
	stopwatch.timer_exit_executed = false;
	free_irq(irqs[0], NULL);
	free_irq(irqs[1], NULL);
	free_irq(irqs[2], NULL);
	free_irq(irqs[3], NULL);
	return 0;
}

/*
	driver_ioctl() : handle ioctl request corresponding to cmd
*/
static long driver_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
	printk("driver_ioctl()\n");

	switch(cmd){
		case IOCTL_START:
			wait_event_interruptible(wait_queue, stopwatch.timer_exit_executed == true);
			printk("sleep done\n");
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

/*
	some functions can only be used with specific module license permission.
*/
MODULE_LICENSE("GPL");

module_init(driver_init);
module_exit(driver_exit);