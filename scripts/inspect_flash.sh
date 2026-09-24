#!/bin/bash
st-flash read /tmp/flash.bin 0x08000000 0x100000
xxd -o 0x08000000 /tmp/flash.bin > /tmp/flash.txt
nvim /tmp/flash.txt
