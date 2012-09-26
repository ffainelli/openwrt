/*
 *  Linksys WAP4410N board support
 *
 *  Copyright (C) 2012 Florian Fainelli <florian@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>

#include <asm/mach-ath79/ar71xx_regs.h>
#include <asm/mach-ath79/ath79.h>

#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define WAP4410N_GPIO_LED_WLAN		0	/* active low */
#define WAP4410N_GPIO_LED_POWER		1	/* active high */

#define WAP4410N_GPIO_BTN_RESET		21	/* active low */

#define WAP4410N_KEYS_POLL_INTERVAL	20	/* msecs */
#define WAP4410N_KEYS_DEBOUNE_INTERVAL	(3 * WAP4410N_KEYS_POLL_INTERVAL)

#define WAP4410N_SERCOMM_OFFSET		0x3ff70
#define WAP4410N_MAC0_OFFSET		0x30

static struct mtd_partition wap4410n_partitions[] = {
	{
		.name		= "uboot",
		.offset		= 0,
		.size		= 0x040000,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "env",
		.offset		= 0x040000,
		.size		= 0x010000,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "rootfs",
		.offset		= 0x050000,
		.size		= 0x650000,
	}, {
		.name		= "linux",
		.offset		= 0x6a0000,
		.size		= 0x140000,
	}, {
		.name		= "nvram",
		.offset		= 0x7e0000,
		.size		= 0x010000,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "caldata",
		.offset		= 0x7f0000,
		.size		= 0x010000,
		.mask_flags	= MTD_WRITEABLE,
	},
};

static struct physmap_flash_data wap4410n_flash_data = {
	.width		= 2,
	.parts		= wap4410n_partitions,
	.nr_parts	= ARRAY_SIZE(wap4410n_partitions),
};

static struct resource wap4410n_flash_resources[] = {
	[0] = {
		.start	= AR71XX_SPI_BASE,
		.end	= AR71XX_SPI_BASE + AR71XX_SPI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device wap4410n_flash_device = {
	.name		= "physmap-flash",
	.id		= -1,
	.resource	= wap4410n_flash_resources,
	.num_resources	= ARRAY_SIZE(wap4410n_flash_resources),
	.dev		= {
		.platform_data = &wap4410n_flash_data,
	},
};

static struct gpio_led wap4410n_leds_gpio[] __initdata = {
	{
		.name		= "wap4410n:green:wlan",
		.gpio		= WAP4410N_GPIO_LED_WLAN,
		.active_low	= 1,
	}, {
		.name		= "wap4410n:green:power",
		.gpio		= WAP4410N_GPIO_LED_POWER,
		.active_low	= 1,
		.default_trigger = "default-on",
	}
};

static struct gpio_keys_button wap4410n_gpio_keys[] __initdata = {
	{
		.desc		= "reset",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = WAP4410N_KEYS_DEBOUNE_INTERVAL,
		.gpio		= WAP4410N_GPIO_BTN_RESET,
		.active_low	= 1,
	}
};

static void __init wap4410n_setup(void)
{
	u8 *eeprom = (u8 *)KSEG1ADDR(0x1fff1000);
	u8 *mac = (u8 *)KSEG1ADDR(0x1f000000 + WAP4410N_SERCOMM_OFFSET +
				WAP4410N_MAC0_OFFSET);

	ath79_register_mdio(0, 0x0);

	ath79_init_mac(ath79_eth0_data.mac_addr, mac, 0);
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII;
	ath79_eth0_data.phy_mask = 0x1;

	ath79_register_eth(0);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(wap4410n_leds_gpio),
				 wap4410n_leds_gpio);

	ath79_register_gpio_keys_polled(-1, WAP4410N_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(wap4410n_gpio_keys),
					wap4410n_gpio_keys);

	ath79_register_wmac(eeprom, NULL);

	platform_device_register(&wap4410n_flash_device);
}

MIPS_MACHINE(ATH79_MACH_WAP4410N, "WAP4410N", "Linksys WAP4410N", wap4410n_setup);
