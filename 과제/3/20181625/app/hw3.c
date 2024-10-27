#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEVICE_PATH "/dev/stopwatch"
#define DEVICE_MAJOR_NUMBER 242

#define IOCTL_START _IO(DEVICE_MAJOR_NUMBER, 0)

int main(int argc, char** argv) {
	int fd = open(DEVICE_PATH, O_WRONLY);
	if (fd < 0) {
		printf("failed to open /dev/stopwatch\n");
		return -1;
	}
	
	ioctl(fd, IOCTL_START);
	
	//to check program entered sleep state, and is terminated when VOL+ is pressed for about 3 secs 
	printf("program terminated after sleep\n");
	close(fd);

	return 0;
}