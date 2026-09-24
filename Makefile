.PHONY: all common bootloader firmware clean

all: common bootloader firmware

common:
	$(MAKE) -C common

bootloader: common
	$(MAKE) -C bootloader

firmware: bootloader
	$(MAKE) -C firmware

clean:
	$(MAKE) -C common clean
	$(MAKE) -C bootloader clean
	$(MAKE) -C firmware clean
	