##################################################
# Makefile
##################################################

BOOT:=boot.asm
LDR:=loader.asm
BOOT_BIN:=$(subst .asm,.bin,$(BOOT))
LDR_BIN:=$(subst .asm,.bin,$(LDR))
.PHONY : everything

everything :
	nasm loader.asm -o loader.bin
	sudo mount -o loop newfd.img /mnt/floppy/
	sudo cp loader.bin /mnt/floppy/ -v
	sudo umount /mnt/floppy/




clean :
	rm -f $(BOOT_BIN) $(LDR_BIN)


