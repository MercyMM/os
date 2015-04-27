# Makefile for boot
ENTRYPOINT	= 0x30400
ENTRYOFFSET = 0x400


# Program
ASM 		= nasm
Dasm		= ndisasm
CC			= gcc
LD			= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -f elf
CFLAGS		= -I include/ -c -fno-builtin
LDFLAGS		= -s -Ttext 

