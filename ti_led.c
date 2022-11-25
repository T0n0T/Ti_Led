/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-24     liwentai       the first version
 */
#include "ti_led.h"
#include "stdlib.h"
#include <stdint.h>
#include <board.h>
#include <string.h>
#include <drv_i2c_nca9555.h>

#define DBG_TAG "ti_led"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

ALIGN(RT_ALIGN_SIZE)
static rt_slist_t _slist_head = RT_SLIST_OBJECT_INIT(_slist_head); /**< Led 链表头节点 */
static struct rt_mutex led_mtx;                                    /**< Led 互斥锁 */


/**
 * @addtogroup 具体LED操作方法
 * @brief 在这里实现针对于led硬件的初始化，动作方法
 * @{
 * 对于nca9555引脚上led的初始化方法
 * @param argc 参数量
 * @param argv 参数字符串存储指针
 * @return 返回一个用于led应用包的对象
 */
static led_t nca9555_led_init(const void* buf)
{
    led_t led = RT_NULL;
    char device_name[RT_NAME_MAX];
    rt_strncpy(device_name, (char*)buf, RT_NAME_MAX);

    rt_device_t dev = rt_device_find(device_name);

    if (led == RT_NULL) {
        LOG_E("led device is not exit.");
        return RT_NULL;
    }
    char str[6];
    rt_device_open(dev, RT_DEVICE_OFLAG_RDWR);
    for(int j=0; j < 2; j++)
    {
        for(int i=0; i < 8; i++)
        {
            rt_sprintf(str, "%s%d%s%d","IO",j,"_",i);
            rt_device_control(dev, OUTPUT, str);
        }
    }
    led->user_data = (void*)dev;
    return led;
}

/**
 * nca9555的led操作方法
 * @param led   led控制对象
 * @param stat  写入的逻辑电平
 * @return      0为操作成功，-1为操作失败
 */
static rt_err_t nca9555_led_action(led_t led, uint8_t stat)
{
    rt_device_t dev = (rt_device_t)led->user_data;
    if (rt_device_write(dev, stat, led->name, 1) != 1) {
        return -RT_ERROR;
    }
    return RT_EOK;
}

/** 将nca9555方法写入led对象*/
const static struct led_ops nca9555_ops =
{
        .init = nca9555_led_init,
        .action = nca9555_led_action
};
/**@}*/

/**
 * @addtogroup Ti_LED的应用函数组
 * @{
 * 动作完成后回调函数
 * @param led
 */
static void led_default_compelete_callback(led_t led)
{
    RT_ASSERT(led);
    LOG_D("led pin:%d compeleted.", led->name);
}

/**
 * 用于解析闪烁数组
 * @param led ti_led操作对象
 * @param blink_cmd 一个代表闪烁方式的字符串
 *  @verbatim
    例子:
    "100,200,100,200"
    需要正整数，单位为ms，按照亮灭亮灭规律
 @endverbatim
 * @return
 */
static rt_err_t led_get_blink_arr(led_t led, const char *blink_cmd)
{
    RT_ASSERT(led);
    RT_ASSERT(led->blink_arr == RT_NULL);

    const char *ptr = blink_cmd;
    uint16_t *blink_arr = RT_NULL;
    while (*ptr) {
        if (*ptr == ',')
            led->blink_num++;
        ptr++;
    }
    if (*(ptr - 1) != ',')
        led->blink_num++;

    if (led->blink_num == 0)
        return -RT_ERROR;

    blink_arr = rt_malloc(led->blink_num * sizeof(uint16_t));
    if (blink_arr == RT_NULL)
        return -RT_ENOMEM;
    rt_memset(blink_arr, 0, led->blink_num * sizeof(uint16_t));

    ptr = blink_cmd;
    for (uint16_t i = 0; i < led->blink_num; i++) {
        blink_arr[i] = atoi(ptr);
        ptr = strchr(ptr, ',');
        if (ptr)
            ptr++;
    }

    led->blink_arr = blink_arr;

    return RT_EOK;
}

led_t led_create(const char* name, uint8_t active_logic, const char *blink_cmd, int16_t loop_num)
{
    led_t led = (led_t)rt_malloc(sizeof(led_t));
    if (led == RT_NULL){
        LOG_E("can not allocate memory for ti_led %s.",name);
        return RT_NULL;
    }

    led->name = name;
    led->active = TI_LED_NOACT;
    led->active_logic = active_logic;
    led->blink_arr = RT_NULL;
    led->blink_index = 0;
    /** 记录闪烁模式*/
    if (blink_cmd) {
        if (led_get_blink_arr(led, blink_cmd) != RT_EOK) {
            rt_free(led);
            return RT_NULL;
        }
    }

    led->loop_num = loop_num;
    led->loop_cnt = 0;
    led->tick_timeout = rt_tick_get();
    led->compelete = led_default_compelete_callback;

    led->ops = nca9555_ops;

    rt_slist_init(&led->slist);

    return led;
}

rt_err_t led_delete(led_t led)
{
    RT_ASSERT(led);

    rt_mutex_take(&led_mtx, RT_WAITING_FOREVER);
    rt_slist_remove(&_slist_head, &(led->slist));
    led->slist.next = RT_NULL;
    rt_mutex_release(&led_mtx);

    if (led->blink_arr) {
        rt_free((uint32_t *)led->blink_arr);
        led->blink_arr = RT_NULL;
    }
    rt_free(led);

    return RT_EOK;
}

rt_err_t led_start(led_t led)
{
    RT_ASSERT(led);

    rt_mutex_take(&led_mtx, RT_WAITING_FOREVER);
    if (led->active) {
        rt_mutex_release(&led_mtx);
        return -RT_ERROR;
    }
    if ((led->blink_arr == RT_NULL) || (led->blink_num == 0)) {
        rt_mutex_release(&led_mtx);
        return -RT_ERROR;
    }
    led->blink_index = 0;
    led->loop_cnt = 0;
    led->tick_timeout = rt_tick_get();
    rt_slist_append(&_slist_head, &(led->slist));
    led->active = TI_LED_ACTIVE;
    rt_mutex_release(&led_mtx);

    return RT_EOK;
}

rt_err_t led_stop(led_t led)
{
    RT_ASSERT(led);

    rt_mutex_take(&led_mtx, RT_WAITING_FOREVER);
    if (!led->active) {
        rt_mutex_release(&led_mtx);
        return RT_EOK;
    }

    rt_slist_remove(&_slist_head, &(led->slist));
    led->slist.next = RT_NULL;
    led->active = TI_LED_NOACT;
    rt_mutex_release(&led_mtx);
}

void led_toggle(led_t led)
{
    if (!led->active) {
        led->ops.action(led, led->active_logic);
    }else {
        led->ops.action(led, !led->active_logic);
    }
}

void led_on(led_t led)
{
    led->ops.action(led, led->active_logic);
}

void led_off(led_t led)
{
    led->ops.action(led, !led->active_logic);
}

void led_process(void)
{
    rt_slist_t *node;

    rt_mutex_take(&led_mtx, RT_WAITING_FOREVER);
    rt_slist_for_each(node, &_slist_head)
    {
        led_t led = rt_slist_entry(node, struct led_item, slist);
        if (led->loop_cnt == 0) {
            led_stop(led);
            if (led->compelete) {
                led->compelete(led);
            }

            node = &_slist_head;
            continue;
        }
    __repeat:
        if ((rt_tick_get() - led->tick_timeout) < (RT_TICK_MAX / 2)) {
            if (led->blink_index < led->blink_num) {
                if (led->blink_arr[led->blink_index] == 0) {
                    led->blink_index++;
                    goto __repeat;
                }
                if (led->blink_index % 2) {
                    led_off(led);
                } else {
                    led_on(led);
                }
                led->tick_timeout = rt_tick_get() + rt_tick_from_millisecond(led->blink_arr[led->blink_index]);
                led->blink_index++;
            } else {
                led->blink_index = 0;
                if (led->loop_cnt < led->loop_num)
                    led->loop_cnt;
            }
        }
    }
    rt_mutex_release(&led_mtx);
}
