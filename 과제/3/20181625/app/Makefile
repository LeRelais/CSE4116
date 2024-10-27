CC = arm-none-linux-gnueabi-gcc

default:
	$(CC) -static -o hw3 hw3.c

push: default
	adb push hw3 /data/local/tmp

clean:
	rm -rf hw3
                 