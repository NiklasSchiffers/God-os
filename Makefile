CC      = gcc
AS      = nasm
LD      = ld

CFLAGS  = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra -Iinclude
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = boot.o \
       kernel/main.o \
       kernel/vga.o \
       kernel/keyboard.o \
       kernel/utils.o \
       kernel/iso9660.o \
       shell/shell.o

all: kernel.bin

boot.o: boot.s
	$(AS) -f elf32 boot.s -o boot.o

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

shell/%.o: shell/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o kernel.bin $(OBJS)

clean:
	rm -f *.o kernel/*.o shell/*.o kernel.bin

.PHONY: all clean
