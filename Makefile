# compilers
NASM = nasm
CC   = gcc
LD   = ld
GENISO = genisoimage

# flags
NASM_FLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -c -fno-builtin
LD_FLAGS = -m elf_i386 -T link.ld

# files
ASM = loader.s
C_SRC = kernel.c vga.c klog.c bioskbd.c terminal.c commands.c string.c reboot.c fs.c diskinfo.c runtest.c library.c libdiv.c
OBJ = loader.o kernel.o vga.o klog.o bioskbd.o terminal.o commands.o string.o reboot.o fs.o diskinfo.o runtest.o library.o libdiv.o
ISO_DIR = iso
GRUB_DIR = $(ISO_DIR)/boot/grub
STAGE2 = ./stage2_eltorito
ISO = os.iso
MENU = ./menu.lst
KERNEL = ./kernel.elf

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

libdiv.o: libdiv.c
	$(CC) $(CFLAGS) libdiv.c -o libdiv.o

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
	qemu-system-i386 -kernel kernel.elf -hda disk.img -m 512M -serial stdio

#tryna make auto dd
dd32:
	dd if=/dev/zero of=disk.img bs=1M count=32 status=progress
dd128:
	dd if=/dev/zero of=disk.img bs=1M count=128 status=progress
dd4:
	dd if=/dev/zero of=disk.img bs=1M count=4 status=progress

#WHOLE MAKE CYCLE
a:
	make clean
	make dd128
	make 
	make run
	echo (((((MAKE HAS DONE EVERYTHING FOR YOU, DRIVE SIZE: 128MB)))))