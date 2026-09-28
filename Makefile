.PHONY: all common bootloader default_firmware clean

all: common default_firmware bootloader 

common:
	$(MAKE) -C common

bootloader: common
	$(MAKE) -C bootloader

default_firmware: bootloader
	$(MAKE) -C default_firmware

clean:
	$(MAKE) -C common clean
	$(MAKE) -C bootloader clean
	$(MAKE) -C default_firmware clean
	