/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-24     liwentai       the first version
 */
#ifndef APPLICATIONS_TI_LED_TI_LED_H_
#define APPLICATIONS_TI_LED_TI_LED_H_

#define     TI_LED_NOACT       0
#define     TI_LED_ACTIVE      0

typedef struct led_item led_t;
struct led_item
{
    const char*     name;
    uint8_t         active;
    uint8_t         active_logic;
    rt_slist_t      slist;
    const uint32_t  *blink_arr;
    uint32_t        blink_num;
    rt_tick_t       tick_timeout;
    void*           user_data;
    void            (*compelete)(led_t *led);
    led_t           (*init)(int argc, char *argv[]);
    rt_err_t        (*action)(led_t *led, uint8_t stat);

};

#endif /* APPLICATIONS_TI_LED_TI_LED_H_ */
