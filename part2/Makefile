#tells the kbuild there is one object in the directory named my_timer being built
obj-m += my_timer.o

#specifies the directory of kernel build
KERNELDIR := /lib/modules/`uname -r`/build
#builds target external module
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
#removes all generated files in the module directory
clean:
	rm -f *.ko *.o Module* *mod*

