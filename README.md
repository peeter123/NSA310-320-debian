```
#include <standard_disclaimer.h>
/*
 * Your warranty is now void.
 *
 * I am not responsible for bricked devices, dead SD cards,
 * thermonuclear war, or you getting fired because the system failed. Please
 * do some research if you have any concerns about features included in this Kernel/OS
 * before flashing it! YOU are choosing to use these modifications upon your own choice, and if
 * you point the finger at me for messing up your device, I'll just sit and laugh at you.
 */
```

GITHUB: [NSA310-320-debian](https://github.com/peeter123/NSA310-320-debian)

I used AA666 NSA320 sources for build on NSA310, parts of this tutorial are taken from his [topic](http://forum.nas-central.org/viewtopic.php?f=249&t=3939) (credits to him). We couldn't use his sources because there was no compatible ethernet driver included in his kernel image (Rtl8169 for NSA310).

* Uimage: [NSA310 uImage-2.6.38.6](http://www.scintilla.utwente.nl/~petero/nsa310/uImage-2.6)
* Uimage: [NSA310 uImage-3.2](http://www.scintilla.utwente.nl/~petero/nsa310/uImage-3.2)
* Debian installation ramdisk: [Debian 6](http://www.scintilla.utwente.nl/~petero/nsa310/initrd.gz)
* AA666 diff: [Kernel 2.6.38.6 patch](http://www.scintilla.utwente.nl/~petero/nsa310/patch-2.6.38.6-nsa320)
* Peeter123 diff: [Kernel 3.2 patch](http://www.scintilla.utwente.nl/~petero/nsa310/patch-3.2-nsa320)
* My kernel config: [ARM Kernel config 2.6](http://www.scintilla.utwente.nl/~petero/nsa310/config-arm-2.6)
* My kernel config: [ARM Kernel config 3.2](http://www.scintilla.utwente.nl/~petero/nsa310/config-arm-3.2)

Now you can choose between 2.6 and 3.2 kernel :)

# Compilation from source(CrossCompile on Ubutu)

* Grab the linux kernel source: [linux-2.6.38.6.tar.bz2](http://www.kernel.org/pub/linux/kernel/v2.6/linux-2.6.38.6.tar.bz2)
* Make sure you have _build-essential_ and _gcc-arm-linux-gnueabi_ installed
* Extract the kernel to a dir in your homedir (for example `~/build`)
* Cd to `build/`
* Put the patchfile next to the `linux-2.6.38.6` dir
* Apply patch: `patch -p0 < patch-2.6.38.6-nsa320`
* Copy `config-arm` to `.config` in `linux-2.6.38.6` directory
* `make menuconfig ARCH=arm` (Optional if you want to make changes)
* `ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make uImage modules`
* You'll now have _uImage_ in `linux-2.6.38.6/arch/arm/boot/uImage`

My kernel includes:
* Usb audio drivers + ALSA (for MPD)
* Usb printer support
* Ext2, Ext3, Ext4, ntfs, fat filesystems
* RTL8169 ethernet drivers
* Led driver (from AA666)
* Basic stuff such as USB, SATA, SCSI, USB DRIVERS etc.

# Serial Adapter

U-Boot is only available via a serial console on the board. I've used a bus-pirate to interface with the board. Make sure your cable is only outputting 3.3V.

`|*|-|*|*|*|` --> pinout on the board: `|GND|NC|RX|TX|VCC|`

## Prepare usb stick

Since uboot on the NSA310 doesn't work with ethernet, it is necessary to make an usb drive with the necessary files.

* Get an usb disk with a size that supports FAT16 (<4GB)
* Format it to FAT16
* Copy the needed files to the stick

# Start Debian Install

Uboot:

Prepare u-boot for mainline kernel:
```
setenv mainlineLinux yes
setenv arcNumber 3339
saveenv
reset
```

After reset hit any key to stop autoboot process.

Now we can begin to install Debian Squeeze:

```
usb start
fatload usb 0 0x800000 uImage
fatload usb 0 0xb00000 initrd.gz
setenv bootargs 'console=ttyS0,115200 root=/dev/ram initrd=0xb00000,0x900000 ramdisk=32768'
bootm 0x800000
```

In the case that the Debian installer is unable to see your HDD(s) during installation -> Start a shell from installer menu and create dev nodes for disks and partitions manually:
```
mknod /dev/sda b 8 0
mknod /dev/sda1 b 8 1
mknod /dev/sda2 b 8 2
```

Partition your drive like you want. I've created three partitions:
* 150MB ext2 partition (/boot,primary) -> we can boot uImage from this partition
* 5GB ext4 partition (/,primary)
* 1800GB ext4 partition (/home,logical)
* 500MB swap partition (,logical)

# After install:

Now we can try to boot from harddisk
```
usb start
fatload usb 0 0x800000 uImage
```

Next, try to boot and mount root filesystem.
If you installed root filesystem into `/dev/sda1`:

```
setenv bootargs 'console=ttyS0,115200 root=/dev/sda1'
bootm 0x800000
```

In case if you successfully booted into installed Debian, you can put kernel into NAND flash or on HDD.

# NAND install:

If you are confident everything is working...

Download kernel from usb and save it into nand kernel_2 partition:
```
fatload usb 0 0x800000 uImage
nand erase 0x4640000 0x300000
nand write.e 0x800000 0x4640000 0x300000
```

Following command depends on where you installed Debian.
In case if it on sda1:
```
setenv bootargs 'console=ttyS0,115200 root=/dev/sda1'
```

Prepare boot command
```
setenv bootcmd 'nand read.e 0x2000000 0x04640000 0x400000; bootm 0x2000000'
saveenv
```

Try to boot
```
boot
```

# HDD install:

If you are confident everything is working...

Put uImage in `/boot`

cat `/etc/fstab` to see which partition is is mounted on `/boot` (in my case `/dev/sda3`)

Following command depends on where you installed Debian.
In case if it on sda1:
```
setenv bootargs 'console=ttyS0,115200 root=/dev/sda1'
```

Prepare boot command
```
setenv bootcmd 'ide reset; ext2load ide 0:3 0x800000 /uImage; bootm 0x800000'
saveenv
```

change ide 0:3 according to your findings at(cat /etc/fstab) and change 3 to the number after sda.

Try to boot
```
boot
```

# Fix wrong mac address

Add these lines to `/etc/rc.local`:

```
ifdown eth1
/sbin/ifconfig eth1 hw ether XX:XX:XX:XX:XX:XX
ifup eth1
```

Where `XX:XX:XX:XX:XX:XX` is your original mac extracted from uboot's enviroment variables

# Alloc kernel panics

Add these lines in `/etc/sysctl.conf`:

```
# Set VM min memory
vm.min_free_kbytes=8192
```

# Ledssssss

Put these lines in `/etc/rc.local` to stop blinking once system has booted:

```
#Setup leds
echo 1 > /sys/class/leds/nsa320:green:System/brightness
```

# Shutdown via Powerbutton

It is possible to shutdown the box by pressing power button.
Install [input-event-daemon](https://github.com/gandro/input-event-daemon) and put following to `/etc/input-event-daemon.conf`:

```
[Global]
listen = /dev/input/event0
[Keys]
POWER = shutdown -h now
```

# Upgrade to wheezy

* Cd to `/etc/apt`
* `mv sources.list sources.list.bak`
* `vi sources.list`
* Add these contents:

```
#############################################################
################### OFFICIAL DEBIAN REPOS ###################
#############################################################

###### Debian Main Repos
deb http://ftp.nl.debian.org/debian/ wheezy main contrib non-free
deb-src http://ftp.nl.debian.org/debian/ wheezy main contrib non-free

###### Debian Update Repos
deb http://security.debian.org/ wheezy/updates main contrib non-free
deb http://ftp.nl.debian.org/debian/ wheezy-proposed-updates main contrib non-free
deb-src http://security.debian.org/ wheezy/updates main contrib non-free
deb-src http://ftp.nl.debian.org/debian/ wheezy-proposed-updates main contrib non-free

##############################################################
##################### UNOFFICIAL  REPOS ######################
##############################################################

###### 3rd Party Binary Repos

#### Debian Multimedia - http://www.debian-multimedia.org/
## Run this command: apt-get update && apt-get install debian-multimedia-keyring && apt-get update
deb http://www.debian-multimedia.org testing main non-free

#### Webmin - http://webmin.com/
## Run this command: wget -q http://www.webmin.com/jcameron-key.asc -O- | apt-key add -
deb http://download.webmin.com/download/repository sarge contrib
```

* `apt-get update`
* `apt-get full-upgrade`
* Follow on screen instructions

# Known Bugs

* Samba crashes whole system
* Ethernet leds don't work @ 1gbit
* All mtd blocks read as bad. The chip is somehow not supported by kernel.

If somebody knows a fix, let us know :)

Also, if there is a nice contribution in this topic I will add it to the topic start.  :D
