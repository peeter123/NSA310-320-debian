#include <linux/module.h>
#include <linux/init.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/syscalls.h>
#include <linux/kmod.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <asm/fcntl.h>
#include <asm/hardirq.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>

#include <mach/kirkwood.h>
#include <plat/gpio.h>
#include <linux/gpio.h>
#include "common.h"
#include "mpp.h"



#define MCU_DATA_PIN	14
#define MCU_CLK_PIN	16
#define MCU_ACT_PIN	17

MODULE_LICENSE("GPL v2");
#define MODULE_NAME     "mcu"

unsigned long mcu_data = 0;
unsigned long mcu_counter = 0;

static struct proc_dir_entry *mcu_proc;

static int mcu_status_read_fn(char *buf, char **start, off_t offset,
                int count, int *eof, void *data)
{
	unsigned long len, i;

	mcu_data = 0;

	gpio_request(14, "MCU_DATA");
	gpio_request(16, "MCU_CLK");
	gpio_request(17, "MCU_ACT");

	gpio_direction_output(MCU_ACT_PIN, 0);
	msleep(100);
	gpio_direction_input(MCU_DATA_PIN);
	gpio_direction_output(MCU_CLK_PIN, 0);
	udelay(100);

	for(i = 0 ; i < 32 ; i++)
	{		
		mcu_data <<= 1;
		gpio_set_value(MCU_CLK_PIN,1);

		mcu_data |= gpio_get_value(MCU_DATA_PIN)?0x1:0;

		udelay(100);

		gpio_set_value(MCU_CLK_PIN,0);

		udelay(100);		
	}


	gpio_set_value(MCU_CLK_PIN,1);
	gpio_set_value(MCU_ACT_PIN,1);


	gpio_free(14);
	gpio_free(16);
	gpio_free(17);

	len = sprintf(buf, "MagicNum:%u\nRpss:%u\nTemperature:%u\n",(unsigned int)((mcu_data & 0xff000000) >> 24),
					   (unsigned int)((mcu_data & 0xff0000) >> 16),
					   (unsigned int)((mcu_data & 0xffff)));
	*eof = 1;

	return len;
}

static int mcu_status_write_fn(struct file *file, const char __user *buffer,
                unsigned long count, void *data)
{
	return 0;
}


static int __init mcu_init(void)
{

	mcu_proc = create_proc_entry("mcu", 0644, NULL);
	if(mcu_proc != NULL)
	{
		mcu_proc->read_proc = mcu_status_read_fn;
		mcu_proc->write_proc = mcu_status_write_fn;
	}

	return 0;
}

static void __exit mcu_exit(void)
{
	remove_proc_entry("mcu", NULL);

}

module_init(mcu_init);
module_exit(mcu_exit);

