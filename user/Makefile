PROGS = init shell

#CFLAGS = -std=c99 -m32 -nostdlib -nostdinc -g -O1 -Wall -Werror
CFLAGS = -std=c99 -m32 -nostdlib -g -O1 -Wall -Werror
RUSTBIN = ~gheith/public/rust32/bin

all : $(PROGS)
	(cd ../mkfs; make)

LIBC_OFILES = printf.o sys.o crt0.o libc.o heap.o machine.o

# keep all files
.SECONDARY :

%.o :  Makefile %.c
	gcc -c -MD $(CFLAGS) $*.c

%.o :  Makefile %.S
	gcc -MD -m32 -c $*.S

%.o :  Makefile %.s
	gcc -MD -m32 -c $*.s

lib%.a : Makefile %.rs
	PATH=$(RUSTBIN):$$PATH rustc --crate-type staticlib $*.rs

$(PROGS) : % : Makefile %.o $(LIBC_OFILES) lib%.a
	ld -N -m elf_i386 -e start -Ttext=0x80000000 -o $@  $*.o $(LIBC_OFILES) lib$*.a
	strip $*
	cp $@ ../mkfs/root/sbin

clean ::
	rm -f *.a
	rm -f *.o
	rm -f *.d
	rm -f $(PROGS)

-include *.d
