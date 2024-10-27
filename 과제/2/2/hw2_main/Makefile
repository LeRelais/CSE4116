CC = arm-none-linux-gnueabi-gcc

default:
	$(CC) -static -o hw2 hw2.c

push: default
	adb push hw2 /data/local/tmp

clean:
	rm -rf hw2
