/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) Copyright 2012-2016 Stephen Warren
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>
#include <asm/arch/timer.h>
#include <linux/stringify.h>

#ifndef __ASSEMBLY__
#include <asm/arch/base.h>
#endif // __ASSEMBLY__

/*
 * 2835 is a SKU in a series for which the 2708 is the first or primary SoC,
 * so 2708 has historically been used rather than a dedicated 2835 ID.
 *
 * We don't define a machine type for bcm2709/bcm2836 since the RPi Foundation
 * chose to use someone else's previously registered machine ID (3139, MX51_GGC)
 * rather than obtaining a valid ID:-/
 *
 * For the bcm2837, hopefully a machine type is not needed, since everything
 * is DT.
 */
#ifdef CONFIG_BCM2835
#define CONFIG_MACH_TYPE		MACH_TYPE_BCM2708
#endif // CONFIG_BCM2835

/* Memory layout */
#define CONFIG_SYS_SDRAM_BASE		0x00000000
#define CONFIG_SYS_UBOOT_BASE		CONFIG_SYS_TEXT_BASE
/*
 * The board really has 256M. However, the VC (VideoCore co-processor) shares
 * the RAM, and uses a configurable portion at the top. We tell U-Boot that a
 * smaller amount of RAM is present in order to avoid stomping on the area
 * the VC uses.
 */
#define CONFIG_SYS_SDRAM_SIZE		SZ_128M
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_SDRAM_BASE + \
					 CONFIG_SYS_SDRAM_SIZE - \
					 GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_MALLOC_LEN		SZ_4M
#define CONFIG_SYS_BOOTM_LEN		SZ_64M

/* Devices */
/* GPIO */
#define CONFIG_BCM2835_GPIO


/* Console configuration */
#define CONFIG_SYS_CBSIZE		1024

/* Environment */
#define CONFIG_SYS_LOAD_ADDR		0x00008000
#define CONFIG_LOADADDR			CONFIG_SYS_LOAD_ADDR


/* ATAGs support for bootm/bootz */
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG

#define FDT_HIGH "ffffffffffffffff"
#define INITRD_HIGH "ffffffffffffffff"

/*
 * Memory layout for where various images get loaded by boot scripts:
 *
 * I suspect address 0 is used as the SMP pen on the RPi2, so avoid this.
 *
 * Older versions of the boot firmware place the firmware-loaded DTB at 0x100,
 * newer versions place it in high memory. So prevent U-Boot from doing its own
 * DTB + initrd relocation so that we won't accidentally relocate the initrd
 * over the firmware-loaded DTB and generally try to lay out things starting
 * from the bottom of RAM.
 *
 * kernel_addr_r has different constraints on ARM and Aarch64.  For 32-bit ARM,
 * it must be within the first 128M of RAM in order for the kernel's
 * CONFIG_AUTO_ZRELADDR option to work. The kernel itself will be decompressed
 * to 0x8000 but the decompressor clobbers 0x4000-0x8000 as well. The
 * decompressor also likes to relocate itself to right past the end of the
 * decompressed kernel, so in total the sum of the compressed and and
 * decompressed kernel needs to be reserved.
 *
 *   For Aarch64, the kernel image is uncompressed and must be loaded at
 *   text_offset bytes (specified in the header of the Image) into a 2MB
 *   boundary. The 'booti' command relocates the image if necessary. Linux uses
 *   a default text_offset of 0x80000.  In summary, loading at 0x80000
 *   satisfies all these constraints and reserving memory up to 0x02400000
 *   permits fairly large (roughly 36M) kernels.
 *
 * scriptaddr and pxefile_addr_r can be pretty much anywhere that doesn't
 * conflict with something else. Reserving 1M for each of them at
 * 0x02400000-0x02500000 and 0x02500000-0x02600000 should be plenty.
 *
 * On ARM, both the DTB and any possible initrd must be loaded such that they
 * fit inside the lowmem mapping in Linux. In practice, this usually means not
 * more than ~700M away from the start of the kernel image but this number can
 * be larger OR smaller depending on e.g. the 'vmalloc=xxxM' command line
 * parameter given to the kernel. So reserving memory from low to high
 * satisfies this constraint again. Reserving 1M at 0x02600000-0x02700000 for
 * the DTB leaves rest of the free RAM to the initrd starting at 0x02700000.
 * Even with the smallest possible CPU-GPU memory split of the CPU getting
 * only 64M, the remaining 25M starting at 0x02700000 should allow quite
 * large initrds before they start colliding with U-Boot.
 */
#define CONFIG_ENV_FLAGS_LIST_STATIC	"pn:so,sn:so,rev:so\0"
#define CONFIG_SYS_I2C_SLAVE		1
#define CONFIG_SYS_I2C_SPEED		400000

#define CONFIG_EXTRA_ENV_SETTINGS										\
	"fdt_high=" FDT_HIGH "\0"											\
	"initrd_high=" INITRD_HIGH "\0"										\
	"kernel_addr_r=" __stringify(CONFIG_LOADADDR) "\0"					\
	"scriptaddr=0x02400000\0"											\
	"fdto_addr_r=0x02500000\0"											\
	"fdt_addr=0x2eff2f00\0"												\
	"ramdisk_addr_r=0x02700000\0"										\
	/* Error handling */												\
	"save=saveenv; saveenv;\0"											\
	"error_cnt=0\0"														\
	"error_max=3\0"														\
	"err=setexpr error_cnt $error_cnt + 1; run save; reset;\0"			\
	"clean=setenv tmp; setenv filesize;\0"								\
	"halt=while true; do sleep 1; done;\0"								\
	/* Factory functions */											\
	"pending_factory_values=1\0"										\
	"produce_me=echo \"Checking factory file\";"						\
		"if test -n \"$pending_factory_values\" && test -e mmc 0:1 /factory.scr; then"	\
			" load mmc 0:1 ${scriptaddr} /factory.scr;"				\
			" source ${scriptaddr}; setenv pending_factory_values; run save; fi;\0"			\
	/* Boot arguments */												\
	"status=blue\0"														\
	"args=setenv bootargs \"coherent_pool=1M 8250.nr_uarts=1"			\
		" snd_bcm2835.enable_compat_alsa=0 snd_bcm2835.enable_hdmi=1"	\
		" bcm2708_fb.fbwidth=0 bcm2708_fb.fbheight=0"					\
		" bcm2708_fb.fbdepth=24 bcm2708_fb.fbswap=1"					\
		" vc_mem.mem_base=0x3ec00000"									\
		" vc_mem.mem_size=0x40000000 dwc_otg.lpm_enable=0"				\
		" console=ttyAMA0,115200 rootfstype=ext4 rootwait"				\
		" video=DSI-1:1920x1080-24"										\
		" logo.nologo"													\
		" vt.global_cursor_default=0"									\
		" fsck.mode=force fsck.repair=yes\";\0"							\
	/* Boot functions */												\
	"partsel=if test $error_cnt > $error_max; then"						\
		" if test $status = green; then"								\
			" setenv status blue; else setenv status green;"			\
		" fi; setenv error_cnt 0; run save; fi;\0"						\
	"boot_me=echo \"Booting ${status}\"; if test -e mmc 0:1 /boot.scr; then"					\
		" load mmc 0:1 ${scriptaddr} /boot.scr;"						\
		" source ${scriptaddr}; fi;\0"									\
	"bootcmd=run produce_me; run partsel; run boot_me; run clean; run err; run halt;\0"

#endif // __CONFIG_H
