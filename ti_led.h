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

#include <rtthread.h>
#include <rtdevice.h>

#define     TI_LED_NOACT       0
#define     TI_LED_ACTIVE      0

typedef struct led_item* led_t;
struct led_ops
{
    led_t           (*init)(const void* buf);               /**< 对象初始化方法 */
    rt_err_t        (*action)(led_t led, uint8_t stat);    /**< 对象动作方法 */
};

struct led_item
{
    const char*     name;                                   /**< 对象编号 */
    uint8_t         active;                                 /**< 激活标志 */
    uint8_t         active_logic;                           /**< 激活需要逻辑电平 */
    rt_slist_t      slist;                                  /**<  */
    const uint16_t  *blink_arr;                             /**< 闪烁数组指针 */
    uint16_t        blink_num;                              /**< 一次闪烁动作次数 */
    uint16_t        blink_index;                              /**< 数组索引 */
    int16_t         loop_num;                               /**< 动作循环次数 */
    int16_t         loop_cnt;                               /**< 循环次数计数 */
    rt_tick_t       tick_timeout;                           /**< 用于计算持续tick */

    void*           user_data;                              /**< 存储led对象操作方式 */

    void            (*compelete)(led_t led);                /**< 操作完成回调函数 */
    struct led_ops  ops;

};

#endif /* APPLICATIONS_TI_LED_TI_LED_H_ */
