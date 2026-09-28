# STM32F401RE Bootloader

## Prerequisites 

- Install ST-Link drivers to flash https://www.st.com/en/development-tools/stsw-link009.html
- Install stlink-tools https://github.com/stlink-org/stlink
- Install GCC tool chain with arm-none-eabi (https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
- Make (https://www.gnu.org/software/make/)
- Python3

I am personally using WSL on windows.

```
git submodule init
git submodule update

cd firmware
make
```

### Flasing

```
st-flash --reset write firmware.bin 0x08000000
```

### Notes on using WSL

You must expose the USB to WSL. As admin:

```
# Install
winget install --interactive --exact dorssel.usbipd-win

# Use
# Note the BUS ID of ST-Link
usbipd list
usbipd bind -b {INSERT BUS_ID}
usbipd attach --wsl -ab {INSERT BUS_ID}
```