#!/bin/bash
insmod dev_driver.ko
mknod /dev/dev_driver c 242 0

