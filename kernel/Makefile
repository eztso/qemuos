all : kernel.img

-include ../common.mak

kernel : Makefile $(OFILES) $(CFILES) $(SFILES) $(CCFILES)
	ld -T script.ld -N -m elf_i386 -e start -o kernel $(OFILES)

clean ::
	rm -f kernel

