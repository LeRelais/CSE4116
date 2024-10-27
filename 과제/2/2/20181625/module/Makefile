obj-m := dev_driver.o
dev_driver-objs := hw2_driver.o

KDIR = /work/achroimx_kernel
PWD = $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=arm

push: default
	adb push *.ko /data/local/tmp

clean:
	rm -rf .tmp_versions .driver.* .dev_driver.* *.cmd *.mod.c *.mod.o *.o *.ko *.synmvers *.order

