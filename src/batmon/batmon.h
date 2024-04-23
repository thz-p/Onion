#ifndef ADC_H__ // 防止头文件重复包含
#define ADC_H__

#include <fcntl.h> // 文件控制操作
#include <pthread.h> // 线程操作
#include <signal.h> // 信号处理
#include <sqlite3/sqlite3.h> // SQLite3 库
#include <stdbool.h> // 布尔类型
#include <stdint.h> // 整数类型
#include <stdio.h> // 标准输入输出
#include <stdlib.h> // 标准库函数
#include <string.h> // 字符串操作
#include <sys/file.h> // 文件锁
#include <sys/ioctl.h> // 设备控制
#include <unistd.h> // Unix 标准库

#ifdef PLATFORM_MIYOOMINI // 如果定义了 PLATFORM_MIYOOMINI 宏
#include "shmvar/shmvar.h" // 包含共享内存变量头文件
#endif

#include "system/battery.h" // 电池系统头文件
#include "system/display.h" // 显示系统头文件
#include "system/system.h" // 系统头文件
#include "utils/config.h" // 配置工具头文件
#include "utils/file.h" // 文件工具头文件
#include "utils/flags.h" // 标志工具头文件
#include "utils/log.h" // 日志工具头文件

#include "batmonDB.h" // 电池监控数据库头文件

#define CHECK_BATTERY_TIMEOUT_S 15 // 检查电池百分比的超时时间（秒）

// 电池日志
#define BATTERY_LOG_THRESHOLD 2 // 定义何时记录新的电池条目
#define FILO_MIN_SIZE 1000 // 文件最小大小
#define MAX_DURATION_BEFORE_UPDATE 600 // 最大更新前的持续时间

// 读取电池用
#define SARADC_IOC_MAGIC 'a' // SARADC 魔术数字
#define IOCTL_SAR_INIT _IO(SARADC_IOC_MAGIC, 0) // 初始化 SAR
#define IOCTL_SAR_SET_CHANNEL_READ_VALUE _IO(SARADC_IOC_MAGIC, 1) // 设置通道读取值

typedef struct {
    int channel_value; // 通道值
    int adc_value; // ADC 值
} SAR_ADC_CONFIG_READ;

static bool adcthread_active = false; // ADC 线程是否活动的标志
static pthread_t adc_pt; // ADC 线程的 pthread 句柄
static bool quit = false; // 是否退出的标志
static int sar_fd, adc_value_g; // SAR 文件描述符、全局 ADC 值
static bool is_suspended = false; // 是否挂起的标志

static void sigHandler(int sig); // 信号处理函数声明
void cleanup(void); // 清理函数声明

void update_current_duration(void); // 更新当前持续时间函数声明
void log_new_percentage(int new_bat_value, int is_charging); // 记录新百分比函数声明
int get_current_session_time(void); // 获取当前会话时间函数声明
int set_best_session_time(int best_session); // 设置最佳会话时间函数声明
void saveFakeAxpResult(int current_percentage); // 保存虚拟 AXP 结果函数声明
bool isCharging(void); // 是否正在充电函数声明
int updateADCValue(int); // 更新 ADC 值函数声明
int getBatPercMMP(void); // 获取电池百分比函数声明
int batteryPercentage(int); // 电池百分比函数声明
static void *batteryWarning_thread(void *param); // 电池警告线程函数声明
void batteryWarning_show(void); // 显示电池警告函数声明
void batteryWarning_hide(void); // 隐藏电池警告函数声明
bool warningDisabled(void); // 是否禁用警告函数声明

#endif // ADC_H__ 结束头文件定义

// 这是一个用于处理电池相关操作的头文件。
// 它包含了必要的系统库、工具库和其他相关头文件，并声明了一些函数和结构体。