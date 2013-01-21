 /*
 * Zyxel NSA-310 Setup, by AA666 and Peeter123
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/mtd/partitions.h>
#include <mtd/mtd-abi.h>
#include <linux/ata_platform.h>
#include <linux/mv643xx_i2c.h>
#include <linux/ethtool.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/leds.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/kirkwood.h>
#include <plat/mvsdio.h>
#include "common.h"
#include "mpp.h"
#include <linux/timer.h>
#include <linux/jiffies.h>

static void nsa310_timerfunc(unsigned long data);
static DEFINE_TIMER(timer, nsa310_timerfunc, 0, 0);

static struct mtd_partition nsa310_nand_parts[] = {
	{
		.name = "uboot",
		.offset = 0,
		.size = 0x100000,
		.mask_flags = MTD_WRITEABLE
	}, {
		.name = "uboot_env",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 0x80000
	}, {
		.name = "key_store",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 0x80000
	}, {
		.name = "info",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 0x80000
	}, {
		.name = "etc",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 0xA00000
	}, {
		.name = "kernel_1",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 0xA00000
	}, {
		.name = "rootfs1",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 0x2FC0000
	}, {
		.name = "kernel_2",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 0xA00000
	}, {
		.name = "rootfs2",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 0x2FC0000
	},
};

static struct mv_sata_platform_data nsa310_sata_data = {
	.n_ports	= 2,
};

static unsigned int nsa310_mpp_config[] __initdata = {
	MPP36_GPIO, // Reset button
	MPP37_GPIO, // Copy button
	MPP46_GPIO, // Power button

	MPP48_GPIO, // Power Off
	MPP21_GPIO, // USB Power Off

	MPP28_GPIO, // Sys LED Green
	MPP29_GPIO, // Sys LED Yellow
	MPP41_GPIO, // SATA1 LED Green
	MPP42_GPIO, // SATA1 LED Red
	MPP12_GPO, // SATA2 LED Green
	MPP13_GPIO, // SATA2 LED Red
	MPP39_GPIO, // Copy LED Green
	MPP40_GPIO, // Copy LED Red
	MPP15_GPIO, // USB LED Green

	MPP14_GPIO, // MCU Data
	MPP16_GPIO, // MCU Clk
	MPP17_GPIO, // MCU Act

	MPP38_GPIO, // VID B0
	MPP45_GPIO, // VID B1

	MPP44_GPIO, // Buzzer
	MPP43_GPIO, // HTP

	MPP47_GPIO, // Power Resume Data
	MPP49_GPIO, // Power Resume Clock

	0
};

static struct gpio_led nsa310_gpio_led[] = {
        {
            .name		= "nsa310:green:System",
            .default_trigger	= "timer",
            .gpio		= 28,
            .active_low		= 0,

        },
	{
            .name		= "nsa310:red:System",
            .default_trigger	= "none",
            .gpio		= 29,
            .active_low		= 0,
        },
	{
            .name		= "nsa310:green:SATA1",
            .default_trigger	= "none",
            .gpio		= 41,
            .active_low		= 0,
        },
	{
            .name		= "nsa310:red:SATA1",
            .default_trigger	= "sata-disk",
            .gpio		= 42,
            .active_low		= 0,
        },
	{
            .name		= "nsa310:green:SATA2",
            .default_trigger	= "none",
            .gpio		= 12,
            .active_low		= 0,
        },
	{
            .name		= "nsa310:red:SATA2",
            .default_trigger	= "none",
            .gpio		= 13,
            .active_low		= 0,
        },
	{
            .name		= "nsa310:green:USB",
            .default_trigger	= "none",
            .gpio		= 15,
            .active_low		= 0,
        },
        {
            .name		= "nsa310:green:Copy",
            .default_trigger	= "none",
            .gpio		= 39,
            .active_low		= 0,
        },
	{
            .name		= "nsa310:red:Copy",
            .default_trigger	= "none",
            .gpio		= 40,
            .active_low		= 0,
        },
};


static int nsa310_gpio_blink_set(unsigned gpio, int state,
	unsigned long *delay_on, unsigned long *delay_off)
{

// Use hardware acceleration
//    if (delay_on && delay_off && !*delay_on && !*delay_off)
//	*delay_on = *delay_off = 100;

	switch(state) {
	    case GPIO_LED_NO_BLINK_LOW:
	    case GPIO_LED_NO_BLINK_HIGH:
		orion_gpio_set_blink(gpio, 0);
		gpio_set_value(gpio, state);
	    break;
	    case GPIO_LED_BLINK:
		orion_gpio_set_blink(gpio, 1);
	    break;
	}
	return 0;
}


static struct gpio_led_platform_data nsa310_led_data = {
        .leds           = nsa310_gpio_led,
        .num_leds       = ARRAY_SIZE(nsa310_gpio_led),
	.gpio_blink_set	= nsa310_gpio_blink_set,
};

static struct platform_device nsa310_leds = {
        .name   = "leds-gpio",
        .id     = -1,
        .dev    = { .platform_data  = &nsa310_led_data, }
};

static struct gpio_keys_button nsa310_gpio_keys_button[] = {
        {
            .code           	= KEY_POWER,
            .type		= EV_KEY,
            .gpio           	= 46,
            .desc           	= "Power Button",
            .active_low     	= 0,
            .debounce_interval 	= 1000,
        },
        {
            .code           	= KEY_COPY,
            .type		= EV_KEY,
            .gpio           	= 37,
            .desc           	= "USB Copy",
            .active_low     	= 1,
            .debounce_interval 	= 1000,
        },
        {
            .code           	= KEY_OPTION,
            .type		= EV_KEY,
            .gpio           	= 36,
            .desc           	= "Reset",
            .active_low     	= 1,
            .debounce_interval 	= 1000,
        },
};

static struct gpio_keys_platform_data nsa310_keys_data = {
        .buttons        = nsa310_gpio_keys_button,
        .nbuttons       = ARRAY_SIZE(nsa310_gpio_keys_button),
};


static struct platform_device nsa310_buttons = {
        .name           = "gpio-keys",
        .id             = -1,
        .dev            = { .platform_data  = &nsa310_keys_data, }
};

static void nsa310_power_off(void)
{
//
//don't work with sysfs
    	printk(KERN_INFO "NSA310: Activating power off GPIO pin...\n");
	gpio_set_value(48, 1);

// If machine goes to restart, uncomment next lines for infinite loop
/*    	printk(KERN_INFO "System halted, please turn off power manually\n");
	gpio_set_value(28, 0);
	do {
	    mdelay(1000);
	} while(1);
*/
}

static void nsa310_timerfunc(unsigned long data)
{
// Activate USB Power
	if (gpio_request(21, "USB Power") != 0 || gpio_direction_output(21, 1) != 0)
	    printk(KERN_ERR "NSA310: Failed to setup USB power GPIO\n");
	else
    	    printk(KERN_INFO "NSA310: USB power enabled\n");
	gpio_free(21);
}

static void __init nsa310_init(void)
{
	u32 dev, rev;

	kirkwood_init();

	kirkwood_mpp_conf(nsa310_mpp_config);
	kirkwood_nand_init(ARRAY_AND_SIZE(nsa310_nand_parts), 40);
	kirkwood_pcie_id(&dev, &rev);

	kirkwood_sata_init(&nsa310_sata_data);
	kirkwood_uart0_init();
	kirkwood_i2c_init();

	platform_device_register(&nsa310_leds);
	platform_device_register(&nsa310_buttons);
	
	kirkwood_ehci_init();
//	USB Power delay for 20 sec	
	timer.function = nsa310_timerfunc;
        timer.data = 0;
	timer.expires = jiffies + msecs_to_jiffies(20000);
	add_timer(&timer);


/*  Power resume control */
	    gpio_request(49, "Power-clk");
	    gpio_direction_output(49, 1);
	    gpio_request(47, "Power-data");
// Clear power resume
//	    gpio_direction_output(47, 0);
// Set power resume
	    gpio_direction_output(47, 1);
	    udelay(1000);
//	    gpio_direction_output(49, 0);
	    gpio_set_value(49, 0);
// release GPIO?
//test
	    gpio_free(47);
	    gpio_free(49);
    	    printk(KERN_INFO "NSA310: Power resume enabled\n");


// Activate Power-off GPIO
	if (gpio_request(48, "Power-off") == 0 && gpio_direction_output(48, 0) == 0) {
//    	    gpio_free(48);
            pm_power_off = nsa310_power_off;
    	    printk(KERN_INFO "NSA310: Power-off GPIO enabled\n");
    	} else
		printk(KERN_ERR "NSA310: Failed to configure Power-off GPIO\n");

};

static int __init nsa310_pci_init(void)
{
	if (machine_is_nsa310())
		kirkwood_pcie_init(KW_PCIE0);
	return 0;
}

subsys_initcall(nsa310_pci_init);

MACHINE_START(NSA310, "Zyxel NSA-310")
	.atag_offset	= 0x100,
	.init_machine	= nsa310_init,
	.map_io		= kirkwood_map_io,
	.init_early	= kirkwood_init_early,
	.init_irq	= kirkwood_init_irq,
	.timer		= &kirkwood_timer,
MACHINE_END

