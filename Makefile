# compilers
NASM = nasm
CC   = gcc
LD   = ld
GENISO = genisoimage

# flags
NASM_FLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -c
LD_FLAGS = -m elf_i386 -T link.ld

# files
ASM = loader.s
C_SRC = kernel.c vga.c klog.c bioskbd.c terminal.c commands.c string.c
OBJ = loader.o kernel.o vga.o klog.o bioskbd.o terminal.o commands.o string.o
KERNEL = kernel.elf
ISO_DIR = iso
GRUB_DIR = $(ISO_DIR)/boot/grub
STAGE2 = ./stage2_eltorito
ISO = os.iso
MENU = ./menu.lst


# main target
all: $(ISO)

# compile assembly
loader.o: loader.s
	$(NASM) $(NASM_FLAGS) loader.s -o loader.o

# compile C sources
kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

vga.o: vga.c
	$(CC) $(CFLAGS) vga.c -o vga.o

klog.o: klog.c
	$(CC) $(CFLAGS) klog.c -o klog.o

bioskbd.o: bioskbd.c bioskbd.h vga.h
	$(CC) $(CFLAGS) bioskbd.c -o bioskbd.o

# link kernel
kernel.elf: $(OBJ)
	$(LD) $(LD_FLAGS) $(OBJ) -o $(KERNEL)

# build ISO
$(ISO): $(KERNEL)
	mkdir -p $(GRUB_DIR)
	cp $(STAGE2) $(GRUB_DIR)/
	cp $(MENU) $(GRUB_DIR)/
	cp $(KERNEL) $(ISO_DIR)/boot/
# mkdir -p iso/modules

	$(GENISO) -R \
	 -b boot/grub/$(notdir $(STAGE2)) \
	 -no-emul-boot \
	 -boot-load-size 4 \
	 -A os \
	 -input-charset utf8 \
	 -quiet \
	 -boot-info-table \
	 -o $(ISO) $(ISO_DIR)

# clean
clean:
	rm -f *.o $(KERNEL) $(ISO)
	rm -rf $(ISO_DIR)

# run
run:
	qemu-system-i386 -cdrom  os.iso
