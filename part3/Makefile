#tells the kbuild there is two objects in the directory named sys_call.o and elevator.o being built
obj-y := sys_call.o
obj-m := elevator.o
#specifies directory of where kernel source is located and informs kbuild 
PWD := $(shell pwd)
KDIR := /lib/modules/`uname -r`/build
#builds target external module
default:
	$(MAKE) -C $(KDIR) SUBDIRS= $(PWD) modules
#removes all generated files in the module directory
clean:
	rm -f *.o *.ko *.mod.* Module.* modules.*
