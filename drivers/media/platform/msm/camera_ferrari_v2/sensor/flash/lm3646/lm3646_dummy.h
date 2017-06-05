/*
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

#ifndef MSM_LED_FLASH_LM3646_DUMMY_H
#define MSM_LED_FLASH_LM3646_DUMMY_H

#include "../msm_led_flash.h"

#define LM3646_DUMMY_LOGTAG "[LM3646]"
#define LM3646_DUMMY_LOGTAGD LM3646_DUMMY_LOGTAG "[D] "
#define LM3646_DUMMY_LOGTAGE LM3646_DUMMY_LOGTAG "[E] "
#define LM3646_DUMMY_DBG(fmt, args...) pr_debug(LM3646_DUMMY_LOGTAGD fmt, ##args)
#define LM3646_DUMMY_ERR(fmt, args...) pr_err(LM3646_DUMMY_LOGTAGE fmt, ##args)

#define DEFAULT_MAX_TORCH_CURRENT 0x7
#define DEFAULT_MAX_FLASH_CURRENT 0xA

#define LM3646_REG_MAX_CURRENT(a,b) ( (((a) & 0x7) << 4) | (b) )

extern struct msm_led_flash_ctrl_t fctrl;

extern struct msm_camera_i2c_reg_array lm3646_init_array[];
extern struct msm_camera_i2c_reg_array lm3646_off_array[];
extern struct msm_camera_i2c_reg_array lm3646_release_array[];
extern struct msm_camera_i2c_reg_array lm3646_low_array[];
extern struct msm_camera_i2c_reg_array lm3646_high_array[];

struct __lm3646_dummy_data {
	enum msm_camera_led_config_t state;
	u8 brightness;
	uint16_t wcf; // warmth correction factor
};

extern struct __lm3646_dummy_data lm3646_dummy_data;

uint16_t lm3646_get_wcf_torch_current(unsigned int c);
uint16_t lm3646_get_wcf_flash_current(unsigned int c);

#endif // MSM_LED_FLASH_LM3646_DUMMY_H
