/*
 * LM3646 Dummy driver to allow userspace control
 *
 * Copyright (c) 2017, thewisenerd <thewisenerd@protonmail.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/leds.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/sysfs.h>
#include "lm3646_dummy.h"

#define DRIVER_NAME "dummy,lm3646"

struct __lm3646_dummy_data lm3646_dummy_data;

inline uint16_t lm3646_get_wcf_torch_current(unsigned int c) {
	unsigned int t = c * lm3646_dummy_data.wcf / 8;
	if (t > 0x7F)
		return 0;
	return (uint16_t)(0x7F - t);
}

inline uint16_t lm3646_get_wcf_flash_current(unsigned int c) {
	unsigned int t = c * lm3646_dummy_data.wcf / 8;
	if (t > 0x81)
		return 0;
	return (uint16_t)((0x81 - t) * 2 / 3);
}

static inline uint16_t lm3646_get_torch_brightness_value(unsigned int brightness) {
	return LM3646_REG_MAX_CURRENT(			\
		(brightness >> 5),			\
		DEFAULT_MAX_FLASH_CURRENT		\
	);
};

static void lm3646_set_brightness(unsigned int brightness, u8 force) {
	int rc = 0;

	LM3646_DUMMY_DBG("%s: enter!\n", __func__);

	brightness = brightness & 0xFF;

	if (lm3646_dummy_data.brightness == brightness && !force) {
		LM3646_DUMMY_DBG("%s: same brightness value = %u; return\n",
				__func__, brightness);
		return;
	}

	LM3646_DUMMY_DBG("%s: %u\n", __func__, brightness);

	if (brightness == 0) {
		if (fctrl.func_tbl->flash_led_off) {
			rc = fctrl.func_tbl->flash_led_off(&fctrl);
			if (rc < 0) {
				LM3646_DUMMY_ERR("%s:%d fail\n", __func__, __LINE__);
				return;
			}
		}
		if (fctrl.func_tbl->flash_led_release) {
			rc = fctrl.func_tbl->flash_led_release(&fctrl);
			if (rc < 0) {
				LM3646_DUMMY_ERR("%s:%d fail\n", __func__, __LINE__);
				return;
			}
		}
	} else {
		lm3646_init_array[4].reg_data =		\
			lm3646_get_torch_brightness_value(brightness);

		if (fctrl.func_tbl->flash_led_init) {
			rc = fctrl.func_tbl->flash_led_init(&fctrl);
			if (rc < 0) {
				LM3646_DUMMY_ERR("%s:%d fail\n", __func__, __LINE__);
				return;
			}
		}
		if (fctrl.func_tbl->flash_led_low) {
			rc = fctrl.func_tbl->flash_led_low(&fctrl);
			if (rc < 0) {
				LM3646_DUMMY_ERR("%s:%d fail\n", __func__, __LINE__);
				return;
			}
		}
	}

	lm3646_dummy_data.brightness = brightness;
}

static void lm3646_dummy_led_set_brightness(struct led_classdev *cdev,
						enum led_brightness brightness)
{
	lm3646_set_brightness(brightness, 0);
}

static enum led_brightness lm3646_dummy_led_get_brightness(struct led_classdev *cdev)
{
	LM3646_DUMMY_DBG("%s: enter!\n", __func__);
	return lm3646_dummy_data.brightness;
}

static ssize_t lm3646_dummy_led_store_flash(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	u8 val;
	int rc;

	if (kstrtou8(buf, 0, &val))
		return -EINVAL;

	if (!val)
		return size;

	if (fctrl.func_tbl->flash_led_init) {
		rc = fctrl.func_tbl->flash_led_init(&fctrl);
		if (rc < 0) {
			LM3646_DUMMY_ERR("%s:%d fail\n", __func__, __LINE__);
			return rc;
		}
	}

	if (fctrl.func_tbl->flash_led_high) {
		rc = fctrl.func_tbl->flash_led_high(&fctrl);
		if (rc < 0) {
			LM3646_DUMMY_ERR("%s:%d fail\n", __func__, __LINE__);
			return rc;
		}
	}

	msleep_interruptible(400);

	if (lm3646_dummy_data.brightness) {
		lm3646_set_brightness(lm3646_dummy_data.brightness, 1);
	} else {
		if (fctrl.func_tbl->flash_led_off) {
			rc = fctrl.func_tbl->flash_led_off(&fctrl);
			if (rc < 0) {
				LM3646_DUMMY_ERR("%s:%d fail\n", __func__, __LINE__);
				return rc;
			}
		}
		if (fctrl.func_tbl->flash_led_release) {
			rc = fctrl.func_tbl->flash_led_release(&fctrl);
			if (rc < 0) {
				LM3646_DUMMY_ERR("%s:%d fail\n", __func__, __LINE__);
				return rc;
			}
		}
	}

	return size;
}
static DEVICE_ATTR(flash, S_IWUSR, NULL, lm3646_dummy_led_store_flash);

static ssize_t lm3646_dummy_led_get_wcf(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", lm3646_dummy_data.wcf);
}
static ssize_t lm3646_dummy_led_store_wcf(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	u8 val;

	if (kstrtou8(buf, 0, &val))
		return -EINVAL;

	if (val > 16) {
		return -EINVAL;
	}

	lm3646_dummy_data.wcf = val;

	return size;
}
static DEVICE_ATTR(			\
	wcf,				\
	(S_IRUSR|S_IWUSR),		\
	lm3646_dummy_led_get_wcf,	\
	lm3646_dummy_led_store_wcf);


static struct led_classdev lm3646_dummy_led_cdev = {
	.name = "torch",
	.brightness_set = lm3646_dummy_led_set_brightness,
	.brightness_get = lm3646_dummy_led_get_brightness,
};

static int lm3646_dummy_led_probe(struct platform_device *pdev)
{
	return led_classdev_register(&pdev->dev, &lm3646_dummy_led_cdev);
}

static int lm3646_dummy_led_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&lm3646_dummy_led_cdev);
	return 0;
}

static struct platform_driver lm3646_dummy_led_driver = {
	.probe = lm3646_dummy_led_probe,
	.remove = lm3646_dummy_led_remove,
	.driver = {
		.name  = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static struct platform_device lm3646_dummy_led_pdev = {
	.name   = DRIVER_NAME,
	.id     = 0,
};

static int __init lm3646_dummy_led_init(void)
{
	int ret;

	LM3646_DUMMY_DBG("%s: enter!\n", __func__);

	if ((ret = platform_driver_register(&lm3646_dummy_led_driver)) != 0) {
		LM3646_DUMMY_ERR("%s: platform_driver_register failed!\n", __func__);
		goto err_out;
	}

	if ((ret = platform_device_register(&lm3646_dummy_led_pdev)) != 0) {
		LM3646_DUMMY_ERR("%s: platform_device_register failed!\n", __func__);
		platform_driver_unregister(&lm3646_dummy_led_driver);
		goto err_out;
	}

	ret = device_create_file(lm3646_dummy_led_cdev.dev, &dev_attr_flash);
	if (ret < 0) {
		LM3646_DUMMY_ERR("%s: failed to create flash file\n", __func__);
		goto err_create_flash_file;
	}

	ret = device_create_file(lm3646_dummy_led_cdev.dev, \
				&dev_attr_wcf);
	if (ret < 0) {
		LM3646_DUMMY_ERR("%s: failed to create wcf file\n", __func__);
		goto err_create_wcf_file;
	}

	lm3646_dummy_data.wcf = 8;

	goto err_out;

err_create_flash_file:
err_create_wcf_file:
	led_classdev_unregister(&lm3646_dummy_led_cdev);
err_out:
	return ret;
}

static void __exit lm3646_dummy_led_exit(void)
{
	platform_driver_unregister(&lm3646_dummy_led_driver);
}

module_init(lm3646_dummy_led_init);
module_exit(lm3646_dummy_led_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("LM3646 Dummy LED driver");
MODULE_AUTHOR("thewisenerd <thewisenerd@protonmail.com>");
