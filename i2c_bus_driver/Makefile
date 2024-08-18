KVERSION = $(shell uname -r)
PWD := $(shell pwd)
obj-m := custom_i2c_bus_driver.o

default:
	make ARCH=arm64 -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
