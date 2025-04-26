# Makefile
obj-m += apds_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

dtbo:
	dtc -@ -I dts -O dtb -o apds-overlay.dtbo apds-overlay.dts
