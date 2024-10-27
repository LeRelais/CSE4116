#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define READ_KEY "/dev/input/event0"
#define DEVICE_NAME "dev_driver"
#define DEVICE_PATH "/dev/dev_driver"
#define DEVICE_MAJOR_NUMBER 242
#define PUSH_SWITCH "/dev/fpga_push_switch"

#define KEY_RELEASE 0
#define KEY_PRESS 1


/*
  _IO(int type, int number): Type, Number�� �����ϴ� �ܼ��� ioctl
  _IOR(int type, int number, data_type): Device Driver���� Read()
  _IOW(int type, int number, data_type): Device Driver���� Write()
  _IORW(int type, int number, data_type): Device Driver���� Read/Write
 */

#define IOCTL_SET _IOW(DEVICE_MAJOR_NUMBER, 0, char*)
#define IOCTL_START _IO(DEVICE_MAJOR_NUMBER, 0)

int main(int argc, char** argv) {
	//If number of given inputs via terminal is not 4, terminate program and show error.
	if (argc < 4 || argc > 4) {
		printf("usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-200] TIMER_INIT[0001-8000]\n");
		return -1;
	}

	int TIMER_INTERVAL = atoi(argv[1]);
	int TIMER_CNT = atoi(argv[2]);
	int TIMER_INIT = atoi(argv[3]);

	//check conditions. If conditions given to **argv is out of range, terminate program
	if (TIMER_INTERVAL <= 0 || TIMER_INTERVAL > 100 || TIMER_CNT <= 0 || TIMER_CNT > 200) {
		printf("usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-200] TIMER_INIT[0001-8000, three zeros and one non-zero]\n");
		return -1;
	}

	int i, cnt = 0;
	for(i = 0; i < 4; i++){
		if(argv[3][i] == '0') cnt += 1;
		if(argv[3][i] < '0' || argv[3][i] > '8'){
			printf("usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-200] TIMER_INIT[0001-8000, three zeros and one non-zero]\n");
			return -1;
		}
	}

	if(cnt != 3){
		printf("usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-200] TIMER_INIT[0001-8000, three zeros and one non-zero]\n");
		return -1;	
	}
	
	//open device driver and send ioctl request.
	int fd = open(DEVICE_PATH, O_WRONLY);
	if (fd == -1) {
		printf("failed to open /dev/dev_driver\n");
		return -1;
	}

	char options[15];
	sprintf(options, "%03d %03d %04d", TIMER_INTERVAL, TIMER_CNT, TIMER_INIT);

	long res = ioctl(fd, IOCTL_SET, options);
	printf("res : %ld\n", res);
	ioctl(fd, IOCTL_START);

	close(fd);

	return 0;
}