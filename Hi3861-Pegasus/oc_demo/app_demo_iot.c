/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hi_task.h>
#include <string.h>
#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_profile.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "app_demo_multi_sample.h"
#include "app_demo_config.h"
#include "wifi_connecter.h"
//-------------------------------------------------------
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "iot_uart.h"
#include "hi_uart.h"
#include "iot_watchdog.h"
#include "iot_errno.h"
#include"hi_i2c.h"
#include "iot_i2c.h"
/* report traffic light count */
unsigned char ocBeepStatus = BEEP_OFF;
static int g_iState = 0;

/* attribute initiative to report */
#define TAKE_THE_INITIATIVE_TO_REPORT
/* oc request id */
#define CN_COMMADN_INDEX                    "commands/request_id="
/* oc report HiSpark attribute */
#define IO_FUNC_GPIO_OUT 0
#define IOT_GPIO_INDEX_10 10
#define IOT_GPIO_INDEX_11 11
#define IOT_GPIO_INDEX_12 12
#define TRAFFIC_LIGHT_CMD_PAYLOAD           "led_value"
#define TRAFFIC_LIGHT_CMD_CONTROL_MODE      "ControlModule"
#define TRAFFIC_LIGHT_CMD_AUTO_MODE         "AutoModule"
#define TRAFFIC_LIGHT_CMD_HUMAN_MODE        "HumanModule"
#define TRAFFIC_LIGHT_YELLOW_ON_PAYLOAD     "YELLOW_LED_ON"
#define TRAFFIC_LIGHT_RED_ON_PAYLOAD        "RED_LED_ON"
#define TRAFFIC_LIGHT_GREEN_ON_PAYLOAD      "GREEN_LED_ON"
#define TRAFFIC_LIGHT_SERVICE_ID_PAYLOAD    "TrafficLight"
#define TRAFFIC_LIGHT_BEEP_CONTROL          "BeepControl"
#define TRAFFIC_LIGHT_BEEP_ON               "BEEP_ON"
#define TRAFFIC_LIGHT_BEEP_OFF              "BEEP_OFF"
#define TRAFFIC_LIGHT_HUMAN_INTERVENTION_ON     "HUMAN_MODULE_ON"
#define TRAFFIC_LIGHT_HUMAN_INTERVENTION_OFF    "HUMAN_MODULE_OFF"
#define TASK_SLEEP_1000MS (1000)
//----------------------------------------------------------------------------
#define IOT_GPIO_IDX_13 13//SDA将3->13
#define IOT_GPIO_IDX_14 14//SCL
#define HMC_I2C_BAUDRATE 400000
//HMC5883寄存器定义
//寄存器地址定义
#define HMC_CONFIG_A_REG	0X00	//配置寄存器A
//bit0-bit1 xyz是否使用偏压,默认为0正常配置
//bit2-bit4 数据输出速率, 110为最大75HZ 100为15HZ 最小000 0.75HZ
//bit5-bit5每次采样平均数 11为8次 00为一次
#define HMC_CONFIG_B_REG	0X01	//配置寄存器B
//bit7-bit5磁场增益 数据越大,增益越小 默认001
#define HMC_MODE_REG		0X02	//模式设置寄存器
//bit0-bit1 模式设置 00为连续测量 01为单一测量 
#define HMC_XMSB_REG		0X03	//X输出结果
#define HMC_XLSB_REG		0X04 
#define HMC_ZMSB_REG		0X05	//Z输出结果
#define HMC_ZLSB_REG		0X06 
#define HMC_YMSB_REG		0X07	//Y输出结果
#define HMC_YLSB_REG		0X08
#define HMC_STATUS_REG		0X09	//只读的状态
//bit1 数据更新时该位自动锁存,等待用户读取,读取到一半的时候防止数据改变
//bit0 数据已经准备好等待读取了,DRDY引脚也能用
#define HMC_CHEAK_A_REG		0X0A	//三个识别寄存器,用于检测芯片完整性
#define HMC_CHEAK_B_REG		0X0B
#define HMC_CHEAK_C_REG		0X0C
#define HMC_CHECKA_VALUE	0x48	//三个识别寄存器的默认值
#define HMC_CHECKB_VALUE	0x34
#define HMC_CHECKC_VALUE	0x33
#define HMC5883_ADDR        0X3C

#define WRITE_BIT 0x00
#define READ_BIT 0x01

#define UART_BUFF_SIZE 1024
#define U_SLEEP_TIME   100


#define LED_INTERVAL_TIME     10
#define LED_TASK_STACK_SIZE   512
#define LED_LEFT_GPIO         10  // for hispark_pegasus
#define LED_RIGHT_GPIO        2
#define IOT_GPIO_KEY          5
#define WIFI_IOT_GPIO_VALUE0  0
#define WIFI_IOT_GPIO_VALUE1  1
#define NUM 1

char* globalDataString = {0};
void TrafficLightAppOption(HiTrafficLightMode appOptionMode, HiControlModeType appOptionType)
{
    unsigned char currentMode = 0;

    currentMode = SetKeyStatus(appOptionMode);
    switch (GetKeyStatus(CURRENT_MODE)) {
        case TRAFFIC_CONTROL_MODE:
            TrafficLightStatusReport(TRAFFIC_CONTROL_MODE, SetupTrflControlModule);
            break;
        case TRAFFIC_AUTO_MODE:
            TrafficLightStatusReport(TRAFFIC_AUTO_MODE, SetupTrflAutoModule);
            break;
        case TRAFFIC_HUMAN_MODE:
            TrafficLightStatusReport(TRAFFIC_HUMAN_MODE, SetupTrflHumanModule);
            break;
        default:
            break;
    }
}

static void TrafficLightMsgRcvCallBack(char *payload)
{
    unsigned char currentMode = 0;
    unsigned char currentType = 0;
    IOT_LOG_DEBUG("PAYLOAD:%s\r\n", payload);
    if (strstr(payload, TRAFFIC_LIGHT_CMD_CONTROL_MODE) != NULL) {
        currentMode = SetKeyStatus(TRAFFIC_CONTROL_MODE);
        if (strstr(payload, TRAFFIC_LIGHT_YELLOW_ON_PAYLOAD) != NULL) { // YELLOW LED
            OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, "2.Yellow On     ",
                        OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
            GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
        } else if (strstr(payload, TRAFFIC_LIGHT_RED_ON_PAYLOAD) != NULL) { // RED LED
            OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, "1.Red On       ",
                        OLED_DISPLAY_STRING_TYPE_1);  /* 0, 7, xx, 1 */
            GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
            GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
        } else if (strstr(payload, TRAFFIC_LIGHT_GREEN_ON_PAYLOAD) != NULL) { // GREEN LED
            OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, "3.Green On      ",
                        OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
            GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
        }
        TrafficLightAppOption(currentMode, currentType);
    } else if (strstr(payload, TRAFFIC_LIGHT_CMD_AUTO_MODE) != NULL) { // Auto module
        currentMode = SetKeyStatus(TRAFFIC_AUTO_MODE);
        TrafficLightAppOption(currentMode, currentType);
    } else if (strstr(payload, TRAFFIC_LIGHT_CMD_HUMAN_MODE) != NULL) { // Human module
        currentMode = SetKeyStatus(TRAFFIC_HUMAN_MODE);
        if (strstr(payload, TRAFFIC_LIGHT_HUMAN_INTERVENTION_ON) != NULL) {
            currentType = SetKeyType(TRAFFIC_HUMAN_TYPE);
        } else if (strstr(payload, TRAFFIC_LIGHT_HUMAN_INTERVENTION_OFF)) {
            currentType = SetKeyType(TRAFFIC_NORMAL_TYPE);
        }
        TrafficLightAppOption(currentMode, currentType);
    } else if (strstr(payload, TRAFFIC_LIGHT_BEEP_CONTROL) != NULL) { // BEEP option
        if (strstr(payload, TRAFFIC_LIGHT_BEEP_ON) != NULL) { // BEEP ON
            ocBeepStatus = BEEP_ON;
        } else if (strstr(payload, TRAFFIC_LIGHT_BEEP_OFF) != NULL) { // BEEP OFF
            ocBeepStatus = BEEP_OFF;
        }
    }
}

// /< this is the callback function, set to the mqtt, and if any messages come, it will be called
// /< The payload here is the json string
static void DemoMsgRcvCallBack(int qos, const char *topic, char *payload)
{
    const char *requesID;
    char *tmp;
    IoTCmdResp resp;
    IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", qos, topic, payload);
    /* app 下发的操作 */
    TrafficLightMsgRcvCallBack(payload);
    tmp = strstr(topic, CN_COMMADN_INDEX);
    if (tmp != NULL) {
        // /< now you could deal your own works here --THE COMMAND FROM THE PLATFORM
        // /< now er roport the command execute result to the platform
        requesID = tmp + strlen(CN_COMMADN_INDEX);
        resp.requestID = requesID;
        resp.respName = NULL;
        resp.retCode = 0;   ////< which means 0 success and others failed
        resp.paras = NULL;
        (void)IoTProfileCmdResp(CONFIG_DEVICE_PWD, &resp);
    }
    return;
}

void SetupCleanTrflStatus(HiTrafficLightMode earlyMode)
{
    IoTProfileService service;
    IoTProfileKV property;
    if (earlyMode == TRAFFIC_CONTROL_MODE) {
        memset_s(&property, sizeof(property), 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "HumanModule";
        property.value = "OFF";
        memset_s(&service, sizeof(service), 0, sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    } else if (earlyMode == TRAFFIC_AUTO_MODE) {
        memset_s(&property, sizeof(property), 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "ControlModule";
        property.value = "OFF";
        memset_s(&service, sizeof(service), 0, sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    } else if (earlyMode == TRAFFIC_HUMAN_MODE) {
        memset_s(&property, sizeof(property), 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "AutoModule";
        property.value = "OFF";
        memset_s(&service, sizeof(service), 0, sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    }
}
/* traffic light:1.control module */
void SetupTrflControlModule(HiTrafficLightMode currentMode, HiControlModeType currentType)
{

    IoTProfileService service;
    IoTProfileKV property;
    unsigned char status = 0;

    printf("traffic light:control module\r\n");
    if (currentMode != TRAFFIC_CONTROL_MODE && currentType != RED_ON) {
        printf("select current module is not the TRAFFIC_CONTROL_MODE\r\n");
        return HI_NULL;
    }
    status = SetKeyStatus(TRAFFIC_CONTROL_MODE);

    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "ControlModule";
    if (currentType == RED_ON) {
            property.value = globalDataString;
    } else if (currentType == YELLOW_ON) {
            property.value = globalDataString;
    } else if (currentType == GREEN_ON) {
            property.value = globalDataString;
    }
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /* report beep status */
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "AutoModule";
    if (ocBeepStatus == BEEP_ON) {
        property.value = globalDataString;
    } else {
        property.value = globalDataString;
    }
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
}
/* report light time count */
void ReportLedLightTimeCount(void)
{
    IoTProfileService service;
    IoTProfileKV property;
    /* report red led light time count */
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleRLedTC";
    property.iValue = GetLedStatus(RED_LED_AUTOMODE_TIMECOUNT);
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /* report yellow led light time count */
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleYLedTC";
    property.iValue = GetLedStatus(YELLOW_LED_AUTOMODE_TIMECOUNT) ;
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /* report green led light time count */
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleGLedTC";
    property.iValue = GetLedStatus(GREEN_LED_AUTOMODE_TIMECOUNT);
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
}
/* traffic light:2.auto module */
void SetupTrflAutoModule(HiTrafficLightMode currentMode, HiControlModeType currentType)
{
    IoTProfileService service;
    IoTProfileKV property;
    unsigned char status = 0;

    printf("traffic light:auto module\r\n");
    if (currentMode != TRAFFIC_AUTO_MODE) {
        printf("select current module is not the CONTROL_MODE\r\n");
        return HI_NULL;
    }
    /* report beep status */
    status = SetKeyStatus(TRAFFIC_AUTO_MODE);
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "AutoModule";
    if (ocBeepStatus == BEEP_ON) {
        property.value = "BEEP_ON";
    } else {
        property.value = "BEEP_OFF";
    }
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /* report light time count */
    ReportLedLightTimeCount();
}

/* traffic light:3.human module */
void SetupTrflHumanModule(HiTrafficLightMode currentMode, HiControlModeType currentType)
{
    IoTProfileService service;
    IoTProfileKV property;
    unsigned char status = 0;

    printf("traffic light:human module\r\n");
    if (currentMode != TRAFFIC_HUMAN_MODE) {
        printf("select current module is not the CONTROL_MODE\r\n");
        return HI_NULL;
    }
    status = GetKeyStatus(TRAFFIC_HUMAN_MODE);
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "HumanModule";
    if (ocBeepStatus == BEEP_ON) {
        property.value = "BEEP_ON";
    } else {
        property.value = "BEEP_OFF";
    }
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);

    /* red led light time count */
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleRledTC";
    property.iValue = GetLedStatus(RED_LED_HUMANMODE_TIMECOUNT);
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /* yellow led light time count */
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleYledTC";
    property.iValue = GetLedStatus(YELLOW_LED_HUMANMODE_TIMECOUNT);
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /* green led light time count */
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleGledTC";
    property.iValue = GetLedStatus(GREEN_LED_HUMANMODE_TIMECOUNT);
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
}

void TrafficLightStatusReport(HiTrafficLightMode currentMode, const TrflCallBackFunc msgReport)
{
    printf("tarffic light: reporting status...\r\n");
    switch (currentMode) {
        case TRAFFIC_CONTROL_MODE:
            msgReport(TRAFFIC_CONTROL_MODE, NULL);
            break;
        case TRAFFIC_AUTO_MODE:
            msgReport(TRAFFIC_AUTO_MODE, NULL);
            break;
        case TRAFFIC_HUMAN_MODE:
            msgReport(TRAFFIC_HUMAN_MODE, NULL);
            break;
        default:
            break;
    }
    return HI_NULL;
}

static void ReportTrafficLightMsg(void)
{
    switch (GetKeyStatus(CURRENT_MODE)) {
        case TRAFFIC_CONTROL_MODE:
            TrafficLightStatusReport(TRAFFIC_CONTROL_MODE, SetupTrflControlModule);
            break;
        case TRAFFIC_AUTO_MODE:
            TrafficLightStatusReport(TRAFFIC_AUTO_MODE, SetupTrflAutoModule);
            break;
        case TRAFFIC_HUMAN_MODE:
            TrafficLightStatusReport(TRAFFIC_HUMAN_MODE, SetupTrflHumanModule);
            break;
        default:
            break;
    }
}

//-----------------------------------------------------------------------------------------------

enum LedState {
    LED_ON = 0,
    LED_OFF,
};

enum LedState g_ledState = LED_ON;
//振动器模块
static int *LedTaskleft(const char *arg)
{   
                IoTGpioSetOutputVal(LED_TEST_GPIO, WIFI_IOT_GPIO_VALUE1);
                osDelay(LED_INTERVAL_TIME);
                IoTGpioSetOutputVal(LED_TEST_GPIO, WIFI_IOT_GPIO_VALUE0);
    return NULL;
}
static int *LedTaskright(const char *arg)
{   
                IoTGpioSetOutputVal(LED_RIGHT_GPIO, WIFI_IOT_GPIO_VALUE1);
                osDelay(LED_INTERVAL_TIME);
                IoTGpioSetOutputVal(LED_RIGHT_GPIO, WIFI_IOT_GPIO_VALUE0);
    return NULL;
}
/*static int *LedTaskleft(const char *arg)
{   int flag=0;
    (void)arg;
    while (NUM && flag<=5) {
        switch (g_ledState) {
            case LED_ON:
                IoTGpioSetOutputVal(LED_TEST_GPIO, WIFI_IOT_GPIO_VALUE1);
                osDelay(LED_INTERVAL_TIME);
                g_ledState=LED_OFF;
                flag++;
                break;
            case LED_OFF:
                IoTGpioSetOutputVal(LED_TEST_GPIO, WIFI_IOT_GPIO_VALUE0);
                osDelay(LED_INTERVAL_TIME);
                g_ledState=LED_ON;
                flag++;
                break;
            default:
                osDelay(LED_INTERVAL_TIME);
                break;
        }
    }
    return NULL;
}*/
/*static int *LedTaskright(const char *arg)
{   int flag=0;
    (void)arg;
    while (NUM && flag<=5) {
        switch (g_ledState) {
            case LED_ON:
                IoTGpioSetOutputVal(LED_RIGHT_GPIO , WIFI_IOT_GPIO_VALUE1);
                osDelay(LED_INTERVAL_TIME);
                g_ledState=LED_OFF;
                flag++;
                break;
            case LED_OFF:
                IoTGpioSetOutputVal(LED_RIGHT_GPIO , WIFI_IOT_GPIO_VALUE0);
                osDelay(LED_INTERVAL_TIME);
                g_ledState=LED_ON;
                flag++;
                break;
            default:
                osDelay(LED_INTERVAL_TIME);
                break;
        }
    }
    return NULL;
}*/
uint8_t I2C_HMC8553L_ReadData(uint8_t Reg)
{
    uint8_t value = 0;
    uint32_t status = 0;
    uint8_t  buffer[1] = {Reg};
    status=IoTI2cWrite(HI_I2C_IDX_1,(HMC5883_ADDR<<1)|WRITE_BIT, buffer,sizeof(buffer));
     if (status != IOT_SUCCESS)
    {
        printf("===== Error: I2C write status = 0x%x! =====\r\n", status);
        return status;
    }
    status=IoTI2cRead(HI_I2C_IDX_1,(HMC5883_ADDR<<1)|READ_BIT, &value,1);
    if (status != IOT_SUCCESS)
    {
        printf("===== Error: I2C write status = 0x%x! =====\r\n", status);
        return status;
    }
    return value;
}
uint32_t I2C_HMC8553L_WriteData(uint8_t Reg,uint8_t *data)
{
    uint32_t status = 0;
    uint8_t  buffer[1] = {Reg};
    status=IoTI2cWrite(HI_I2C_IDX_1,(HMC5883_ADDR<<1)|WRITE_BIT, buffer,sizeof(buffer));
     if (status != IOT_SUCCESS)
    {
        printf("===== Error: I2C write status = 0x%x! =====\r\n", status);
        return status;
    }
    status=IoTI2cWrite(HI_I2C_IDX_1,(HMC5883_ADDR<<1)|WRITE_BIT, data,sizeof(data));
    if (status != IOT_SUCCESS)
    {
        printf("===== Error: I2C write status = 0x%x! =====\r\n", status);
        return status;
    }
}
void HMC8553L_init()
{
    uint8_t initdata[]={(3<<5)|(1<<4)|(0),(1<<5)|0,0x00};
    I2C_HMC8553L_WriteData(0x00,initdata[0]);//配置寄存器A，8倍平均值滤波，15Hz
    I2C_HMC8553L_WriteData(0x01,initdata[1]);//配置寄存器B，
    I2C_HMC8553L_WriteData(0x02,initdata[2]);//配置模式寄存器，连续测量模式
}


void gpioinit(void)//改了gpio
{   //printf("test here!!!");

    IoTGpioInit(IOT_GPIO_IDX_13);//初始化
    IoSetFunc(IOT_GPIO_IDX_13,6);//引脚复用
    IoTGpioInit(IOT_GPIO_IDX_14);//初始化
    IoSetFunc(IOT_GPIO_IDX_13,6);
    IoTI2cInit(HI_I2C_IDX_1, HMC_I2C_BAUDRATE);
}
void Uart1GpioInit(void)
{
    IoTGpioInit(IOT_IO_NAME_GPIO_0);
    // 设置GPIO0的管脚复用关系为UART1_TX Set the pin reuse relationship of GPIO0 to UART1_ TX
    IoSetFunc(IOT_IO_NAME_GPIO_0, IOT_IO_FUNC_GPIO_0_UART1_TXD);
    IoTGpioInit(IOT_IO_NAME_GPIO_1);
    // 设置GPIO1的管脚复用关系为UART1_RX Set the pin reuse relationship of GPIO1 to UART1_ RX
    IoSetFunc(IOT_IO_NAME_GPIO_1, IOT_IO_FUNC_GPIO_1_UART1_RXD);
}

void Uart1Config(void)
{
    uint32_t ret;
    /* 初始化UART配置，波特率 115200，数据bit为8,停止位1，奇偶校验为NONE */
    /* Initialize UART configuration, baud rate is 9600, data bit is 8, stop bit is 1, parity is NONE */
    IotUartAttribute uart_attr = {
        .baudRate = 115200,
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };
    ret = IoTUartInit(HI_UART_IDX_1, &uart_attr);
    if (ret != IOT_SUCCESS) {
        printf("Init Uart1 Falied Error No : %d\n", ret);
        return;
    }
}

static void HMCTask(void)
{printf("here is ok!!!");
    gpioinit();
    HMC8553L_init();
        int angle;
       uint16_t x,y,z;
      // osDelay(1000);
       x=((I2C_HMC8553L_ReadData(0x03)<<8)|I2C_HMC8553L_ReadData(0x04));
       y=((I2C_HMC8553L_ReadData(0x05)<<8)|I2C_HMC8553L_ReadData(0x06));
       z=((I2C_HMC8553L_ReadData(0x07)<<8)|I2C_HMC8553L_ReadData(0x08));
       printf("x=%d\r\ny=%d\r\nz=%d\r\n",x,y,z);
       //angle=atan((double)y,(double)x)*(180/3.14159265)+180;
       //printf("A=%f\r\n",angle);
       printf("\r\n");
    
}
//此函数可能因为编译器的不同存在一定的问题,字符串最后要以\0结尾
int trans(char arr[])
{
    int ans = 0, temp;
    int i = 0;
    while (arr[i] == ' ' || (arr[i]<='9'&&arr[i]>='0'))
    {
        i++;
    }
    for (int j = 0; j < i; j++)
    {
        if (arr[j] == ' ')
            continue;
        int k = j + 1;
        temp = (int)arr[j] - (int)'0';
        ans = ans + temp * pow(10, (i - k));
    }
    return ans;
}

///< this is the demo main task entry,here we will set the wifi/cjson/mqtt ready ,and
///< wait if any work to do in the while
static void *DemoEntry(const char *arg) 
{ IoTGpioSetOutputVal(LED_LEFT_GPIO, WIFI_IOT_GPIO_VALUE0);
IoTGpioSetOutputVal(LED_RIGHT_GPIO, WIFI_IOT_GPIO_VALUE0);
 hi_watchdog_disable();
 WifiStaReadyWait();
 CJsonInit();
 printf("cJsonInit init \r\n");
 IoTMain();
 IoTSetMsgCallback(DemoMsgRcvCallBack);
/* 主动上报 */
#ifdef TAKE_THE_INITIATIVE_TO_REPORT
//-----------------------------------------------------------------------
 uint32_t count = 0;
 uint32_t len = 0;
 unsigned char uartReadBuff[UART_BUFF_SIZE] = {0};
 // 对UART1的一些初始化
 Uart1GpioInit();
 // 对UART1参数的一些配置
 Uart1Config();
//---------------------------------------------------------------------
 while (1) {
 // /< here you could add your own works here--we report the data to the IoTplatform
 hi_sleep(TASK_SLEEP_1000MS);
 // /< now we report the data to the iot platform
//--------------------------------------------------------------------------------------
// 通过UART1 接收数据 Receive data through UART1

 len = IoTUartRead(HI_UART_IDX_1, uartReadBuff, UART_BUFF_SIZE);

 printf("len is %d",len);
int class;//类别
 if (len > 0) 
 {
 char num1[8]={0},num2[8]={0},num3[8]={0},num4[8]={0};//int x1;
for(int i=0;i<UART_BUFF_SIZE-30;i++)
 {
    if(uartReadBuff[i]=='1')
    {
        class=1;
    }
    else class=0;
if(uartReadBuff[i]=='{')
 {
int j=i+1,k=0;
 while(uartReadBuff[j]!=',')
 {
 num1[k]=uartReadBuff[j];
 k++;
 j++;
}
j++;
k=0;
int x1=trans(num1);
while(uartReadBuff[j]!=',')
{   num2[k]=uartReadBuff[j];
    k++;
    j++;
}j++;
k=0;
int y1=trans(num2);
while(uartReadBuff[j]!=','){
     num3[k]=uartReadBuff[j];
 k++;
 j++;
}
j++;
k=0;
int x2=trans(num3);
while(uartReadBuff[j]!='}'){
     num4[k]=uartReadBuff[j];
 k++;
 j++;
}int y2=trans(num4);
if(class=1)     //改变系数
    {   if(x1+x2<1640)
         globalDataString="左前方有车车";
         else if(x1+x2>1920)
         globalDataString="右前方有车车";
         else
            globalDataString="正前方有车车";}
else if(class=0)
    {
        if(x1+x2<1640)
         globalDataString="左前方有人";
         else if(x1+x2>1920)
         globalDataString="右前方有人";
         else
         globalDataString="正前方有人";
    }

printf("global is %s",globalDataString);
 ReportTrafficLightMsg();
printf("the cordinate is [%d,%d,%d,%d]",x1,y1,x2,y2);
if(1){
 if(x1+x2>1920){
    IoTGpioSetOutputVal(LED_LEFT_GPIO, WIFI_IOT_GPIO_VALUE1);
    osDelay(10*LED_INTERVAL_TIME);
    IoTGpioSetOutputVal(LED_LEFT_GPIO, WIFI_IOT_GPIO_VALUE0);
    //printf("test here!!!!");
 }
else{ IoTGpioSetOutputVal(LED_RIGHT_GPIO,WIFI_IOT_GPIO_VALUE1);
osDelay(10*LED_INTERVAL_TIME);
  IoTGpioSetOutputVal(LED_RIGHT_GPIO, WIFI_IOT_GPIO_VALUE0);}
 }}
/*else if((uartReadBuff[i]=='o')&&(uartReadBuff[i+1]=='b')&&(uartReadBuff[i+2]=='j')&&(uartReadBuff[i+3]=='N')&&(uartReadBuff[i+7]!='0'))
 {
 // ReportTrafficLightMsg();
LedTaskleft(NULL);//震动函数
 }*/
}

 printf("Uart Read Data is: [ %d ] %s \r\n", count, uartReadBuff); 
 //---------------------------------------------------------------------------------

 /*for(int i=0;i<1024;i++)
 { globalDataString[i]=uartReadBuff[i];//将变量值赋给负载
 }*/


}
//---------------------------------------------------------------------------------------
 //ReportTrafficLightMsg();
if (g_iState == 0xffff) {
g_iState = 0;
 break;
 }
 }
#endif
}

///< This is the demo entry, we create a task here, and all the works has been done in the demo_entry
#define CN_IOT_TASK_STACKSIZE  0x1000
#define CN_IOT_TASK_PRIOR 28
#define CN_IOT_TASK_NAME "IOTDEMO"
void AppDemoIot(void)//原来有static被我去了
{
    gpioinit();
    IoTGpioInit(LED_LEFT_GPIO); IoTGpioInit(LED_RIGHT_GPIO);
    IoTGpioSetDir(LED_LEFT_GPIO, IOT_GPIO_DIR_OUT); IoTGpioSetDir(LED_RIGHT_GPIO, IOT_GPIO_DIR_OUT);
    IoTGpioInit(IOT_GPIO_KEY);
    IoSetFunc(IOT_GPIO_KEY, 0);
    IoTGpioSetDir(IOT_GPIO_KEY, IOT_GPIO_DIR_IN);
    IoSetPull(IOT_GPIO_KEY, IOT_IO_PULL_UP);
    //IoTGpioRegisterIsrFunc(IOT_GPIO_KEY, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, OnButtonPressed, NULL);
 IoTGpioSetOutputVal(LED_LEFT_GPIO, WIFI_IOT_GPIO_VALUE0); IoTGpioSetOutputVal(LED_RIGHT_GPIO, WIFI_IOT_GPIO_VALUE0);
    osThreadAttr_t attr;
    IoTWatchDogDisable();

    attr.name = "IOTDEMO";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CN_IOT_TASK_STACKSIZE;//uart里面是5*1024
    attr.priority = CN_IOT_TASK_PRIOR;

    if (osThreadNew((osThreadFunc_t)DemoEntry, NULL, &attr) == NULL) {
        printf("[TrafficLight] Falied to create IOTDEMO!\n");
    }
}

SYS_RUN(AppDemoIot);