KERNELDIR =/home/dfyuan/projects/rk3399_sunnysdk/kernel
PWD := $(shell pwd)
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf modules.* *.o *~ core .depend .*.cmd *.ko
obj-m := ath204a.o
ath204a-objs:= ath204.o sha256.o

