/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-24     liwentai       the first version
 */
#include <ti_led.h>
#include <rtthread.h>
#include <string.h>
#include <drv_i2c_nca9555.h>

#define DBG_TAG "ti_led"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/**
 * @addtogroup 具体LED操作方法
 * @brief 在这里实现针对于led硬件的初始化，动作方法
 * @{
 * 对于nca9555引脚上led的初始化方法
 * @param argc 参数量
 * @param argv 参数字符串存储指针
 * @return 返回一个用于led应用包的对象
 */
static led_t nca9555_led_init(int argc, char *argv[])
{
    led_t led = RT_NULL;
    char device_name[RT_NAME_MAX];
    if (argc == 2)
    {
        rt_strncpy(device_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        LOG_I("put in a char represent of nca9555 led device.");
        return RT_NULL;
    }
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
    led.user_data = (void*)dev;
    return led;
}

/**
 * nca9555的led操作方法
 * @param led   led控制对象
 * @param stat  写入的逻辑电平
 * @return      0为操作成功，-1为操作失败
 */
static rt_err_t nca9555_led_action(led_t *led, uint8_t stat)
{
    rt_device_t dev = (rt_device_t)led->user_data;
    if (rt_device_write(dev, stat, led->name, 1) != 1) {
        return -RT_ERROR;
    }
    return RT_EOK;
}

/** 将nca9555方法写入led对象*/
led_t nca9555_led =
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
static void led_default_compelete_callback(led_t *led)
{
    RT_ASSERT(led);
    LOG_D("led pin:%d compeleted.", led->pin);
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
static rt_err_t led_get_blink_arr(led_t *led, const char *blink_cmd)
{
    RT_ASSERT(led);
    RT_ASSERT(led->blink_arr == RT_NULL);

    const char *ptr = blink_cmd;
    uint32_t *blink_arr = RT_NULL;
    while (*ptr) {
        if (*ptr == ',')
            led->blink_num++;
        ptr++;
    }
    if (*(ptr - 1) != ',')
        led->blink_num++;

    if (led->blink_num == 0)
        return -RT_ERROR;

    blink_arr = rt_malloc(led->blink_num * sizeof(uint32_t));
    if (blink_arr == RT_NULL)
        return -RT_ENOMEM;
    rt_memset(blink_arr, 0, led->blink_num * sizeof(uint32_t));

    ptr = blink_cmd;
    for (uint32_t i = 0; i < led->blink_num; i++) {
        blink_arr[i] = atoi(ptr);
        ptr = strchr(ptr, ',');
        if (ptr)
            ptr++;
    }

    led->blink_arr = blink_arr;

    return RT_EOK;
}

led_t *led_create(const char* name, uint8_t active_logic, const char *light_mode, int32_t loop_cnt)
{
    led_t *led = (led_t *)rt_malloc(sizeof(led_t));
    if (led == RT_NULL){
        LOG_E("can not allocate memory for ti_led %s.",name);
        return RT_NULL;
    }

    led->name = name;
    led->active = TI_LED_NOACT;
    led->active_logic = active_logic;
    led->blink_arr = RT_NULL;
}
