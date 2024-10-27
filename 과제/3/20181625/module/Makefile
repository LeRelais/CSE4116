obj-m := stopwatch.o
stopwatch-objs := hw3_driver.o

KDIR = /work/achroimx_kernel
PWD = $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=arm

push: default
	adb push *.ko /data/local/tmp

clean:
	rm -rf .tmp_versions .driver.* .stopwatch.* *.cmd *.mod.c *.mod.o *.o *.ko *.symvers *.order
