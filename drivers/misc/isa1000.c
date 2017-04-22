/*
 * Copyright (C) 2015 Balázs Triszka <balika011@protonmail.ch>
 * Copyright (C) 2017 thewisenerd <thewisenerd@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#ifdef CONFIG_MACH_XIAOMI_FERRARI
#include <linux/clk.h>
#include <soc/qcom/clock-local2.h>
#endif

#include "../staging/android/timed_output.h"

struct isa1000_vib {
	int gpio_isa1000_en;
	int gpio_haptic_en;
	int timeout;
	unsigned int pwm_frequency;
	int pwm_duty_percent;
	struct work_struct work;
	struct mutex lock;
	struct hrtimer vib_timer;
	struct timed_output_dev timed_dev;
	int state;
#ifdef CONFIG_MACH_XIAOMI_FERRARI
	struct clk* vclk;
	bool clk_enabled;
#endif
};

static struct isa1000_vib vib_dev = {
#ifdef CONFIG_MACH_XIAOMI_FERRARI
	.pwm_frequency = 30000,
	.pwm_duty_percent = 80,
#else
	.pwm_frequency = 25000,
	.pwm_duty_percent = 100,
#endif
	.timeout = 15000,
	.state = 0,
};

#ifdef CONFIG_MACH_XIAOMI_FERRARI
static int isa1000_set_duty(struct isa1000_vib *vib, int pwm_duty)
{
	struct rcg_clk *rcg;
	struct clk_freq_tbl *nf;
	int duty;

	if (!vib)
		return -ENODEV;

	if (IS_ERR_OR_NULL(vib->vclk) || !vib->vclk->parent) {
			pr_err("%s: vib->vclk error!\n", __func__);
			return -ENODEV;
	}

	rcg = to_rcg_clk(vib->vclk->parent);
	nf = rcg->current_freq;

	// nf->d_val maps from 150-100 after which clk seems to get stuck
	// make sure pwm_duty range is 0-100
	duty = 150 - (pwm_duty / 2);

	pr_info("%s: pwm_duty = %d, d_val = %d\n", __func__, pwm_duty, duty);

	nf->d_val = duty;
	set_rate_mnd(rcg, nf);

	return 0;
}

static int isa1000_init_clock(struct platform_device *pdev, struct isa1000_vib *vib)
{
	int ret;
	struct pinctrl_state *set_state = NULL;

	vib->vclk = devm_clk_get(&pdev->dev, "vibrator_pwm");
	if (IS_ERR_OR_NULL(vib->vclk)) {
		pr_err("%s: vib->vclk error!\n", __func__);
		return -1;
	}

	ret = clk_set_rate(vib->vclk, vib->pwm_frequency);
	if (ret) {
		pr_err("%s: clk_set_rate err", __func__);
		return ret;
	}

	// set default duty
	isa1000_set_duty(vib, vib->pwm_duty_percent);

	pdev->dev.pins = kmalloc(sizeof(struct dev_pin_info),GFP_KERNEL);
	pdev->dev.pins->p = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pdev->dev.pins->p)) {
		pr_err("%s: return error in lines %d\n", __func__, __LINE__);
	}
	set_state = pinctrl_lookup_state(pdev->dev.pins->p, "pwm_default");
	if (IS_ERR_OR_NULL(set_state)) {
		pr_err("%s: return error in lines %d\n", __func__, __LINE__);
	}
	pinctrl_select_state(pdev->dev.pins->p, set_state);

	set_state = pinctrl_lookup_state(pdev->dev.pins->p, "pwm_sleep");
	if (IS_ERR_OR_NULL(set_state)) {
		pr_err("%s: return error in lines %d\n", __func__, __LINE__);
	}
	pinctrl_select_state(pdev->dev.pins->p, set_state);

	return 0;
}

int isa1000_enable_clock(struct isa1000_vib *vib, int on)
{
	int ret;

	if (!vib)
		return -ENODEV;

	mutex_lock(&vib->lock);

	if (on) {
		if (!vib->clk_enabled) {
			ret = clk_prepare_enable(vib->vclk);
			vib->clk_enabled = true;
		}
	} else {
		if (vib->clk_enabled) {
			clk_disable_unprepare(vib->vclk);
			vib->clk_enabled = false;
		}
	}

	mutex_unlock(&vib->lock);

	return ret;
}
#endif

static int isa1000_set_state(struct isa1000_vib *vib, int on)
{
	if (on) {
#ifdef CONFIG_MACH_XIAOMI_FERRARI
		isa1000_enable_clock(vib, 1);
#endif
		gpio_set_value_cansleep(vib->gpio_isa1000_en, 1);
#ifndef CONFIG_MACH_XIAOMI_FERRARI
		udelay(10);
#endif
		gpio_set_value_cansleep(vib->gpio_haptic_en, 1);
	} else {
		gpio_set_value_cansleep(vib->gpio_isa1000_en, 0);
		gpio_set_value_cansleep(vib->gpio_haptic_en, 0);
#ifdef CONFIG_MACH_XIAOMI_FERRARI
		isa1000_enable_clock(vib, 0);
#endif
	}

	return 0;
}

static void isa1000_enable(struct timed_output_dev *dev, int value)
{
	struct isa1000_vib *vib = container_of(dev, struct isa1000_vib, timed_dev);

	mutex_lock(&vib->lock);
	hrtimer_cancel(&vib->vib_timer);

	if (value == 0)
		vib->state = 0;
	else {
		vib->state = 1;
		value = value > vib->timeout ? vib->timeout : value;
		hrtimer_start(&vib->vib_timer, ktime_set(value / 1000, (value % 1000) * 1000000), HRTIMER_MODE_REL);
	}

	mutex_unlock(&vib->lock);
	schedule_work(&vib->work);
}

static void isa1000_update(struct work_struct *work)
{
	struct isa1000_vib *vib = container_of(work, struct isa1000_vib, work);
	isa1000_set_state(vib, vib->state);
}

static int isa1000_get_time(struct timed_output_dev *dev)
{
	struct isa1000_vib *vib = container_of(dev, struct isa1000_vib, timed_dev);

	if (hrtimer_active(&vib->vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&vib->vib_timer);
		return (int) ktime_to_us(r);
	}

	return 0;
}

static enum hrtimer_restart isa1000_timer_func(struct hrtimer *timer)
{
	struct isa1000_vib *vib = container_of(timer, struct isa1000_vib, vib_timer);

	vib->state = 0;
	schedule_work(&vib->work);

	return HRTIMER_NORESTART;
}

static ssize_t isa1000_amp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *timed_dev = dev_get_drvdata(dev);
	struct isa1000_vib *vib = container_of(timed_dev, struct isa1000_vib, timed_dev);

	return sprintf(buf, "%d\n", vib->pwm_duty_percent);
}

static ssize_t isa1000_amp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct timed_output_dev *timed_dev = dev_get_drvdata(dev);
	struct isa1000_vib *vib = container_of(timed_dev, struct isa1000_vib, timed_dev);

	int tmp;
	sscanf(buf, "%d", &tmp);
	if (tmp > 100)
		tmp = 100;
	else if (tmp < 0)
		tmp = 0;

	vib->pwm_duty_percent = tmp;

#ifdef CONFIG_MACH_XIAOMI_FERRARI
	isa1000_set_duty(vib, vib->pwm_duty_percent);
#endif

	return size;
}

static ssize_t isa1000_pwm_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *timed_dev = dev_get_drvdata(dev);
	struct isa1000_vib *vib = container_of(timed_dev, struct isa1000_vib, timed_dev);

	return sprintf(buf, "%u\n", vib->pwm_frequency);
}

static ssize_t isa1000_pwm_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct timed_output_dev *timed_dev = dev_get_drvdata(dev);
	struct isa1000_vib *vib = container_of(timed_dev, struct isa1000_vib, timed_dev);

	int tmp;
	sscanf(buf, "%u", &tmp);
	if (tmp > 50000)
		tmp = 50000;
	else if (tmp < 10000)
		tmp = 10000;

	vib->pwm_frequency = tmp;

	return size;
}
static struct device_attribute isa1000_device_attrs[] = {
	__ATTR(amp, S_IRUGO | S_IWUSR, isa1000_amp_show, isa1000_amp_store),
	__ATTR(pwm, S_IRUGO | S_IWUSR, isa1000_pwm_show, isa1000_pwm_store),
};

static int isa1000_parse_dt(struct platform_device *pdev, struct isa1000_vib *vib)
{
	int ret;

	ret = of_property_read_u32(pdev->dev.of_node, "gpio-isa1000-en", &vib->gpio_isa1000_en);
	if (ret < 0) {
		dev_err(&pdev->dev, "please check isa1000 enable gpio");
		return ret;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "gpio-haptic-en", &vib->gpio_haptic_en);
	if (ret < 0) {
		dev_err(&pdev->dev, "please check haptic enable gpio");
		return ret;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "timeout-ms", &vib->timeout);
	if (ret < 0)
		dev_err(&pdev->dev, "please check timeout");

	/* print values*/
	dev_info(&pdev->dev, "gpio-isa1000-en: %i, gpio-haptic-en: %i, timeout-ms: %i",
				vib->gpio_isa1000_en, vib->gpio_haptic_en, vib->timeout);

	return 0;
}

static int isa1000_probe(struct platform_device *pdev)
{
	struct isa1000_vib *vib;
	int i, ret;

	platform_set_drvdata(pdev, &vib_dev);
	vib = (struct isa1000_vib *) platform_get_drvdata(pdev);

	/* parse dt */
	ret = isa1000_parse_dt(pdev, vib);
	if (ret < 0) {
		dev_err(&pdev->dev, "error occured while parsing dt\n");
	}

	ret = gpio_is_valid(vib->gpio_isa1000_en);
	if (ret) {
		ret = gpio_request(vib->gpio_isa1000_en, "gpio_isa1000_en");
		if (ret) {
			dev_err(&pdev->dev, "gpio %d request failed",vib->gpio_isa1000_en);
			return ret;
		}
	} else {
		dev_err(&pdev->dev, "invalid gpio %d\n", vib->gpio_isa1000_en);
		return ret;
	}

	ret = gpio_is_valid(vib->gpio_haptic_en);
	if (ret) {
		ret = gpio_request(vib->gpio_haptic_en, "gpio_haptic_en");
		if (ret) {
			dev_err(&pdev->dev, "gpio %d request failed\n", vib->gpio_haptic_en);
			return ret;
		}
	} else {
		dev_err(&pdev->dev, "invalid gpio %d\n", vib->gpio_haptic_en);
		return ret;
	}

#ifdef CONFIG_MACH_XIAOMI_FERRARI
	isa1000_init_clock(pdev, vib);
#endif
	gpio_direction_output(vib->gpio_isa1000_en, 0);
	gpio_direction_output(vib->gpio_haptic_en, 0);

	mutex_init(&vib->lock);

	INIT_WORK(&vib->work, isa1000_update);

	hrtimer_init(&vib->vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib->vib_timer.function = isa1000_timer_func;

	vib->timed_dev.name = "vibrator";
	vib->timed_dev.get_time = isa1000_get_time;
	vib->timed_dev.enable = isa1000_enable;
	ret = timed_output_dev_register(&vib->timed_dev);
	if (ret < 0)
		return ret;

	for (i = 0; i < ARRAY_SIZE(isa1000_device_attrs); i++) {
		ret = device_create_file(vib->timed_dev.dev, &isa1000_device_attrs[i]);
		if (ret < 0) {
			pr_err("%s: failed to create sysfs\n", __func__);
			goto err_sysfs;
		}
	}

	return 0;

err_sysfs:
	for (; i >= 0; i--)
		device_remove_file(vib->timed_dev.dev, &isa1000_device_attrs[i]);

	timed_output_dev_unregister(&vib->timed_dev);
	return ret;
}

static int isa1000_remove(struct platform_device *pdev)
{
	struct isa1000_vib *vib = platform_get_drvdata(pdev);

	timed_output_dev_unregister(&vib->timed_dev);

	hrtimer_cancel(&vib->vib_timer);

	cancel_work_sync(&vib->work);

	mutex_destroy(&vib->lock);

	gpio_free(vib->gpio_haptic_en);
	gpio_free(vib->gpio_isa1000_en);

	return 0;
}

static struct of_device_id vibrator_match_table[] = {
	{ .compatible = "imagis,isa1000", },
	{ },
};

static struct platform_driver isa1000_driver = {
	.probe		= isa1000_probe,
	.remove		= isa1000_remove,
	.driver		= {
		.name	= "isa1000",
		.of_match_table = vibrator_match_table,
	},
};

static int __init isa1000_init(void)
{
	return platform_driver_register(&isa1000_driver);
}
module_init(isa1000_init);

static void __exit isa1000_exit(void)
{
	return platform_driver_unregister(&isa1000_driver);
}
module_exit(isa1000_exit);

MODULE_AUTHOR("Balázs Triszka <balika011@protonmail.ch>");
MODULE_DESCRIPTION("ISA1000 Haptic Motor driver");
MODULE_LICENSE("GPL v2");
