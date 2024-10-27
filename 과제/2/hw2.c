#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

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

int main(int argc, char** argv) {
	//If number of given inputs via terminal is not 4, terminate program and show error.
	if (argc < 4 || argc > 4) {
		printf("usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]\n");
		return -1;
	}

	int TIMER_INTERVAL = atoi(argv[1]);
	int TIMER_CNT = atoi(argv[2]);
	int TIMER_INIT = atoi(argv[3]);

	//check conditions. If conditions given to **argv is out of range, terminate program
	if (TIMER_INTERVAL <= 0 || TIMER_INTERVAL > 100 || TIMER_CNT <= 0 || TIMER_CNT > 100 || TIMER_INIT <= 0 || TIMER_INIT > 8000) {
		printf("usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]\n");
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

	ioctl(fd, IOCTL_SEND_QUERY, options);
	ioctl(fd, IOCTL_REQUEST);

	close(fd);

	return 0;
}