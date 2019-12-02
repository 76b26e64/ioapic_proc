obj-m := ioapic_proc.o
 
ifndef KERNEL_DIR
KERNEL_DIR=/usr/src/linux-headers-`uname -r`/
endif
 
all:
	${MAKE} -C ${KERNEL_DIR} M=`pwd`
 
clean:
	${MAKE} -C ${KERNEL_DIR} M=`pwd` clean
