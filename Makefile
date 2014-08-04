obj-m += nbt.o
obj-m += dht.o
obj-m += client.o
export-objs += nbt.o
export-objs += dht.o
export-objs += client.o

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
