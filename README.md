# Crusader OS

## A hobby operating system made based on: 
- The little book about OS development by Erik Helin, Adam Renberg
- - https://littleosbook.github.io/
- And many other sources like my old Crusader-OS project
- **Special thanks to my friend L.M. <3 for some special ideas**
- And youtube tutorials by https://www.youtube.com/@nanobyte-dev Nanobyte

## Basic facts
- 32 bit atp
- runs well in qemu-i386
- hasn't been tested on real HW yet
- use klog for screen printing basic text and kklog for printing without /n after every ("")
- runs in ring0 atp

## Dependencies:
- Linux debian:
- **sudo apt install make gcc-multilib binutils nasm grub-pc-bin xorriso qemu-system-i386 genisoimage** 
- use **qemu-system-i386 -cdrom os.iso -m 128M -no-reboot -no-shutdown** for running

## Functions:
- [x] Jumping cow uwu
- [x] C support
- [x] terminal
- [x] custom filesystem
- [ ] usb support
- [x] keyboard support
- [ ] sound support
- [ ] txt editors
- [ ] apps
- [ ] internet support
- [ ] linked communication between Crusader-OS devices
- [ ] custom programming language
- [ ] some games 
- [ ] GUI
- [ ] Ethernet smart docking station conection
- [ ] CD-DVD music/movies support
- [ ] ring3 support

## Commands inside OS
- **lib** to get more commands and info
- help → help yourself lol :3
- rt → kinda interesting test thing for functionality testing of new functions etc...
- reboot → makes triple fault to CPU
- ld → info about drive
- and much more...

## Filesystem

| Offset    | Component          | Description                                                                           |
| --------- | ------------------ | ------------------------------------------------------------------------------------- |
| `LBA0`  | Superblock         | Filesystem metadata: 0x5A4C534A, total block count, starting location of other zones |
| `LBA1`  | Block Bitmap       | Each bit represents a block.                                             |
| `LBA2`  | Inode Bitmap       | Tracks which inodes are used or free                                                  |
| `LBA3`  | Inode Table        | Every file has inode_t structure here                         |
| `LBA4`  | Data Blocks        | Stores actual file data                                                             |

- It uses ADA /(IDE)
- Maximum file size is currently 6kB (12*512B)
- FS has no more fixed size so you can use whatever sized disk.img you want (I guess up to 128Gb by hardware)
- You can now also save and see more than 16 files (commit v14)
- i use **dd if=/dev/zero of=disk.img bs=1M count=32 status=progress** for recleaning drive
- and **hexdump -C disk.img | less** for seeing inside content

## MY MAKEFILE SUPPORT
- use **make a** to do every step like make clean, format drive and make again
- you can normally use **make clean**, **make**, and **make run**
- you can also use **make dd4**, **make dd32** or **make dd128** for formating drive with **4**, **32** or **128** MB's 