obj-m += apds_driver.o

KDIR := $(KERNELDIR)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

dtbo:
	dtc -@ -I dts -O dtb -o apds-overlay.dtbo apds-overlay.dts

