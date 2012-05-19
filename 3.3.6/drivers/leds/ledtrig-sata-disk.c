/*
 * LED SATA-Disk Activity Trigger
 *
 * Copyright 2006 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/leds.h>

static void ledtrig_sata_timerfunc(unsigned long data);

DEFINE_LED_TRIGGER(ledtrig_sata);
static DEFINE_TIMER(ledtrig_sata_timer, ledtrig_sata_timerfunc, 0, 0);
static int sata_activity;
static int sata_lastactivity;

void ledtrig_sata_activity(void)
{
	sata_activity++;
	if (!timer_pending(&ledtrig_sata_timer))
		mod_timer(&ledtrig_sata_timer, jiffies + msecs_to_jiffies(10));
}
EXPORT_SYMBOL(ledtrig_sata_activity);

static void ledtrig_sata_timerfunc(unsigned long data)
{
	if (sata_lastactivity != sata_activity) {
		sata_lastactivity = sata_activity;
		/* INT_MAX will set each LED to its maximum brightness */
		led_trigger_event(ledtrig_sata, INT_MAX);
		mod_timer(&ledtrig_sata_timer, jiffies + msecs_to_jiffies(10));
	} else {
		led_trigger_event(ledtrig_sata, LED_OFF);
	}
}

static int __init ledtrig_sata_init(void)
{
	led_trigger_register_simple("sata-disk", &ledtrig_sata);
	return 0;
}

static void __exit ledtrig_sata_exit(void)
{
	led_trigger_unregister_simple(ledtrig_sata);
}

module_init(ledtrig_sata_init);
module_exit(ledtrig_sata_exit);

MODULE_AUTHOR("Richard Purdie <rpurdie@openedhand.com>");
MODULE_DESCRIPTION("LED IDE Disk Activity Trigger");
MODULE_LICENSE("GPL");
