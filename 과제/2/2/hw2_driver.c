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

int major_number;
bool already_opened;

enum _FPGA_MODULES{
	FPGA_DOT = 0,
	FPGA_FND,
	FPGA_LCD,
	FPGA_LED
}FPGA_MODULES;

char *fpga_dot;
char *fpga_fnd;
char *fpga_lcd;
char *fpga_led;
char *fpga_push_switch;

#define FPGA_DOT_ADDR 0x08000210
#define FPGA_FND_ADDR 0x08000004
#define FPGA_LCD_ADDR 0x08000090
#define FPGA_LED_ADDR 0x08000016
#define FPGA_PUSH_SWITCH 0x08000050

#define RESET_RECEIVED 1

//FPGA_DOT 
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

unsigned char fpga_set_blank[10] = {
	// memset(array,0x00,sizeof(array));
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};


typedef struct _timer{
	struct timer_list timer;
	int interval;
	int cnt;
	int cnt_tmp;
	char init[5];
	int origin_num;

	char st_id[9];
	char name_initial[4];
	char lcd_timer_cnt[4];
	char first_initial;
	char last_initial;
	int start_idx;
	int dir;
	int lcd_timer_cnt_len;
}TIMER;

TIMER timer, timer2;

/*
  _IO(int type, int number): Type, Number�� �����ϴ� �ܼ��� ioctl
  _IOR(int type, int number, data_type): Device Driver���� Read()
  _IOW(int type, int number, data_type): Device Driver���� Write()
  _IORW(int type, int number, data_type): Device Driver���� Read/Write
 */

#define IOCTL_SET _IOW(DEVICE_MAJOR_NUMBER, 0, char*)
#define IOCTL_START _IO(DEVICE_MAJOR_NUMBER, 0)

//
static int __init driver_init(void);
static void __exit driver_exit(void);

static long driver_ioctl(struct file*, unsigned int, unsigned long);
static int driver_open(struct inode*, struct file*);
static int driver_release(struct inode*, struct file*);

static struct file_operations driver_fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_release,
	.unlocked_ioctl = driver_ioctl,
};

void fpga_dot_init(int value);
void fpga_dot_clear();
void fpga_fnd_init(char *fnd_str);
void fpga_lcd_init(char *lcd_str);
void fpga_led_init();
void fpga_push_switch_init();

bool push_switch_read();

void driver_set();
static void timer_blink(unsigned long);
void fpga_fnd_blink();
void fpga_lcd_blink();
void fpga_lcd_blink_after_timer();
void fpga_led_blink();
void fpga_dot_blink();
/*
	dirver_init(void) : Initialize driver and register driver to kernel
						Map FPGA modules with using ioremap()
*/
static int __init driver_init(void){
	printk("device_driver_init\n");

	major_number = register_chrdev(DEVICE_MAJOR_NUMBER, DEVICE_NAME, &driver_fops);

	if(major_number < 0){
		printk(KERN_ERR "error. failed to register device %d\n", major_number);
		return major_number;
	}

	/* physical mapping of fpga modules */
	fpga_dot = ioremap(FPGA_DOT_ADDR, 0x0a);
	if(fpga_dot == NULL){
		printk(KERN_ERR "physical mapping unsuccessful.\n");
		return -1;
	}

	fpga_fnd = ioremap(FPGA_FND_ADDR, 0x02);
	if(fpga_fnd == NULL){
		printk(KERN_ERR "physical mapping unsuccessful.\n");
		return -1;
	}

	fpga_lcd = ioremap(FPGA_LCD_ADDR, 0x01);
	if(fpga_lcd == NULL){
		printk(KERN_ERR "physical mapping unsuccessful.\n");
		return -1;
	}

	fpga_led = ioremap(FPGA_LED_ADDR, 0x32);
	if(fpga_led == NULL){
		printk(KERN_ERR "physical mapping unsuccessful.\n");
		return -1;
	}
	
	fpga_push_switch = ioremap(FPGA_PUSH_SWITCH, 0x18);
	if(fpga_push_switch == NULL){
		printk(KERN_ERR "physical mapping unsuccessful.\n");
		return -1;
	}
	return 0;
}

static void __exit driver_exit(void){
	printk("driver exit()\n");
	iounmap(fpga_dot);
	iounmap(fpga_fnd);
	iounmap(fpga_lcd);
	iounmap(fpga_led);
	iounmap(fpga_push_switch);
	unregister_chrdev(DEVICE_MAJOR_NUMBER, DEVICE_NAME);
}

static int driver_open(struct inode* inode, struct file* file) {
	printk("driver opened\n");

	if(already_opened)
		return -1;
	already_opened = true;
	return 0;
}

static int driver_release(struct inode* inode, struct file* file) {
	printk("driver released\n");
	already_opened = false;
	return 0;
}

static long driver_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
	int i, j, timer_val;
	char str[15];
	switch (cmd) {
		case IOCTL_START:
			driver_set();
			break;

		case IOCTL_SET:
			if(strncpy_from_user(str, arg, strlen_user(arg)) < 0){
				printk(KERN_ERR "failed to copy string from user\n");
				return -1;
			}
			printk("%s\n", str);
			i = 0, j = 2;
			char fnd_str[5] = {0, };
			char lcd_str[33] = {0, };
			char timer_cnt[3];

			for(i = 6; i >= 4; i--){
				lcd_str[j] = str[i];
				timer_cnt[j] = str[i];
				j--;
			}

			j = 0;
			char timer_interval_str[4];
			char timer_init[5];
			for(i = 2; i >= 0; i--){
				timer_interval_str[i] = str[i];
			}

			int tmp = 1;
			int timer_count = 0, timer_interval = 0;
			for(i = 2; i >= 0; i--){
				timer_interval += tmp * (timer_interval_str[i] - '0');
				timer_count += tmp * (timer_cnt[i] - '0');
				tmp *= 10;
			}

			printk("timer count : %d\n", timer_count);
			int timer_cnt_len;
			if(timer_count < 10){
				timer_cnt_len = 0;
			}
			else if(timer_count < 100){
				timer_cnt_len = 1;
			}
			else
				timer_cnt_len = 2;

			char *stu_id = "20181625        ";
			
			for(i = 31; i >= 0; i--)
				lcd_str[i] = ' ';

			for(i = strlen(stu_id) - 1; i >= 0; i--)
				lcd_str[i] = stu_id[i];
			
			j = 2;
			printk("timer cnt len : %d\n", timer_cnt_len);
			for(i = 0; i <= timer_cnt_len; i++){
				lcd_str[15-i] = timer_cnt[j];
				j--;
			}

			strcpy(lcd_str+16, "NHJ             ");
			
			j = 0, timer_val = 0;
			for(i = 8; i < 12; i++){
				timer_init[j] = str[i];
				if(timer_init[j] != '0') timer_val = timer_init[j] - '0';
				printk("%c", timer_init[j]);
				j++;
			}
			timer_init[4] = '\0';
			printk("\ntimer_init : %s\n", timer_init);
			// printk("%s\n", timer_init);
			fpga_dot_init(timer_val);
			fpga_fnd_init(timer_init);
			fpga_lcd_init(lcd_str);
			// fpga_led_init(0); 

			// // int timer_interval = atoi(timer_interval_str);
			// // int timer_count = atoi(timer_cnt);


			// /* Initialize timer values */
			printk("Setting initial values of timer\n");
			timer.interval = timer_interval;
			timer.cnt = timer_count;
			strcpy(timer.init, timer_init);
			timer.origin_num = timer_val;
			strcpy(timer.st_id, "20181625");
			strcpy(timer.name_initial, "NHJ");
			strcpy(timer.lcd_timer_cnt, timer_cnt);
			timer.dir = 2;
			timer.first_initial = timer.name_initial[0];
			timer.last_initial = timer.name_initial[2];
			timer.start_idx = 16;
			timer.lcd_timer_cnt_len = timer_cnt_len;
			printk("Setting initial values of timer done\n");
			/*  Timer initialization done */

			/* Waiting for Reset */
			while(1){
				if(push_switch_read()){
					printk("Reset pushed\n");
					break;
				}
			}
			return RESET_RECEIVED;
			break;

		default: 
			printk(KERN_ERR "UNDEFINED IOCTL GIVEN\n");
			return -1;
			break;
	}

	return 0;
}

void driver_set(){
	printk("driver_set()\n");
	/*
		TIMER INIT 
	*/
	init_timer(&timer.timer);
	timer.timer.data = (unsigned long)&timer;
	timer.timer.expires = get_jiffies_64() + timer.interval * (HZ / 10);
	timer.timer.function = timer_blink;
	add_timer(&timer.timer);
}

/* 
	timer_blink() 
	
*/

static void timer_after_timer_cnt(unsigned long data){
	printk("timer_after_timer_cnt() called\n");
	TIMER* timer_data = (TIMER*)data;
	timer_data->cnt--;
	printk("kernel_timer_blink %d\n", timer_data->cnt);
	fpga_lcd_blink_after_timer();

	if(timer_data->cnt >= 0){
		timer_data->timer.data = (unsigned long)&timer2;
		timer_data->timer.expires = get_jiffies_64() + 10 * (HZ / 10);
		timer.timer.function = timer_after_timer_cnt;

		add_timer(&timer_data->timer);
	}
	else{
		char fnd_str[5] = {0, };
		char lcd_str[33] = {0, };
		// fpga_dot_init(0);
		fpga_fnd_init(fnd_str);
		fpga_lcd_init("                                ");
		// fpga_led_init(0);
		printk("Timer expired\n");
	}
}

static void timer_blink(unsigned long data){
	printk("timer_blink() called\n");

	TIMER* timer_data = (TIMER*)data;
	timer_data->cnt--;
	printk("kernel_timer_blink %d\n", timer_data->cnt);
	int tmp = timer_data->cnt; 
	int i = 0;

	fpga_fnd_blink();
	fpga_lcd_blink();
	fpga_dot_blink();
	fpga_led_blink();

	if(timer_data->cnt >= 1){
		timer_data->timer.data = (unsigned long)&timer;
		timer_data->timer.expires = get_jiffies_64() + timer_data->interval * (HZ / 10);
		timer.timer.function = timer_blink;

		add_timer(&timer_data->timer);
	}
	else{
		fpga_lcd_blink();
		char fnd_str[5] = {0,};
		// char lcd_str[33] = {' ',};
		// strcpy(lcd_str, "Time's up!     ");
		// lcd_str[15] = '0';
		timer2.cnt = 3;
		// sprintf(lcd_str+16, "Shutdown in %d...", timer2.cnt);
		// fpga_dot_init(0);
		fpga_fnd_init(fnd_str);
		// fpga_lcd_init(lcd_str);
		fpga_dot_clear();
		fpga_led_init(0);
		init_timer(&timer2.timer);
		timer2.timer.data = (unsigned long)&timer2;
		timer2.timer.expires = get_jiffies_64() + 10 * (HZ / 10);
		timer2.timer.function = timer_after_timer_cnt;

		// timer.timer.data = (unsigned long)&timer;
		// timer.timer.expires = get_jiffies_64() + timer.interval * (HZ / 10);
		// timer.timer.function = timer_blink;
		add_timer(&timer2.timer);

		//add_timer(&timer_data->timer);
		// fpga_led_init(0);
		printk("timer_blink() expired\n");
	}
}

bool push_switch_read(){
	unsigned char push_sw_value[9];
	unsigned short int _s_value;
	int i;

	for(i = 0; i < 9; i++){
		_s_value = inw((unsigned int)fpga_push_switch+i*2);
		push_sw_value[i] = _s_value & 0xFF;
	}

	if(push_sw_value[0] == 80) return true;
	return false;
}

void fpga_dot_init(int value){
	int i = 0;
	unsigned short int _s_value;
	
	value = value >= '0' ? value - '0' : value;

	for(i = 0; i < 10; i++){
		_s_value = fpga_number[value][i] & 0x7F;
		outw(_s_value, (unsigned int)fpga_dot + i * 2);
	}
}

void fpga_dot_clear(){
	int i = 0;
	unsigned short int _s_value;

	for(i = 0; i < 10; i++){
		_s_value = fpga_set_blank[i] & 0x7F;
		outw(_s_value, (unsigned int)fpga_dot + i * 2);
	}
}

void fpga_fnd_init(char *str){
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

void fpga_lcd_init(char *str){
	unsigned short int _s_value = 0;
	int i = 0;

	for(i = 0; i < 32; i++){
		_s_value = (str[i] & 0xFF) << 8 | str[i+1] & 0xFF;
		outw(_s_value, fpga_lcd + i);
		i++;
	}
}

void fpga_led_init(int value){
	outw(value, fpga_led);
}

void fpga_fnd_blink(){
	int i, idx;

	for(i = 0; i < 4; i++){
		if(timer.init[i] != '0'){
			idx = i;
			break;
		}
	}
	timer.init[idx] = timer.init[idx] + 1 >= '9' ? '1' : timer.init[idx] + 1;
	if(timer.init[idx] - '0' == timer.origin_num){
		char tmp = timer.init[idx];
		timer.init[idx] = '0';
		idx = idx + 1 >= 4 ? 0 : idx+1;
		timer.init[idx] = tmp;
	}

	fpga_fnd_init(timer.init);
	// fpga_led_init(timer.init[idx] - '0');
	// fpga_dot_init(timer.init[idx] - '0');
}

void fpga_led_blink(){
	int i, idx;

	for(i = 0; i < 4; i++){
		if(timer.init[i] != '0'){
			idx = i;
			break;
		}
	}

	int led_value = 1;

	led_value <<= 8 - (timer.init[idx] - '0');

	fpga_led_init(led_value);
}

void fpga_dot_blink(){
	int i, idx;

	for(i = 0; i < 4; i++){
		if(timer.init[i] != '0'){
			idx = i;
			break;
		}
	}

	fpga_dot_init(timer.init[i] - '0');
}

void fpga_lcd_blink_after_timer(){
	char lcd_str[33];
	int i =0;

	for(i = 31; i >= 0; i--)
		lcd_str[i] = ' ';

	timer2.cnt_tmp = timer2.cnt;
	// printk("count_tmp is : %d\n", count_tmp);
	
	sprintf(lcd_str, "Time's up!     %dShutdown in %d...", timer.cnt, timer2.cnt_tmp+1);

	fpga_lcd_init(lcd_str);
}

void fpga_lcd_blink(){
	int i;

	char lcd_str[33];
	for(i = 31; i >= 0; i--)
		lcd_str[i] = ' ';

	for(i = 0; i < 8; i++)
		lcd_str[i] = timer.st_id[i];

	timer.cnt_tmp = timer.cnt;
	// printk("count_tmp is : %d\n", count_tmp);
	i = 15;
	if(timer.cnt_tmp != 0){
		while(timer.cnt_tmp > 0){
			lcd_str[i] = (timer.cnt_tmp % 10) + '0';
			printk("count_tmp is : %d\n", timer.cnt_tmp);
			timer.cnt_tmp /= 10;
			i--;
		}
	}
	else
	{
		lcd_str[15] = '0';
	}
	int j = 0;

	if(timer.dir == 2){
		timer.start_idx++;
		
		for(i = timer.start_idx; i <= timer.start_idx + 2; i++){
			lcd_str[i] = timer.name_initial[j];
			j++;
		}

		if(timer.start_idx + 2 == 31)
		{
			timer.start_idx = 31;
			timer.dir = 0;
		}
	}
	else{
		timer.start_idx--;

		for(i = timer.start_idx; i >= timer.start_idx - 2; i--){
			lcd_str[i] = timer.name_initial[2 - j];
			j++;
		}

		if(timer.start_idx - 2 == 16)
		{
			timer.start_idx = 16;
			timer.dir = 2;
		}
	}

	fpga_lcd_init(lcd_str);
}

module_init(driver_init);
module_exit(driver_exit);