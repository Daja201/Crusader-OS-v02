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
- use klog for screen printing basic text

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

## Commands inside OS
- cow → jumping cow
- cat → just cat
- help → help yourself lol :3
- runtest → kinda interesting test thing for functionality testing of new functions etc...

## Filesystem
| Offset    | Component          | Description                                                                           |
| --------- | ------------------ | ------------------------------------------------------------------------------------- |
| `0x0000`  | Superblock         | Filesystem metadata: magic number, version, block size, inode count, data block count |
| `0x0200`  | Block Bitmap       | Tracks which data blocks are used or free                                             |
| `0x0400`  | Inode Bitmap       | Tracks which inodes are used or free                                                  |
| `0x0600`  | Inode Table        | Stores file metadata: type, size, and pointers to data blocks                         |
| `0x1600`  | Data Blocks        | Stores actual file content                                                            |
| End of FS | Reserved / padding | Reserved space for future expansion                                                   |


- at this moment, FS has fixed size of 32MB
- i use **dd if=/dev/zero of=disk.img bs=1M count=32 status=progress** for recleaning drive
- and **hexdump -C disk.img | less** for seeing inside content

