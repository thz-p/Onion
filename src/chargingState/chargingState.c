#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <assert.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "system/battery.h"
#include "system/device_model.h"
#include "system/display.h"
#include "system/keymap_hw.h"
#include "system/rumble.h"
#include "system/settings.h"
#include "system/system.h"
#include "theme/config.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/msleep.h"

#define RELEASED 0
#define PRESSED 1
#define REPEATING 2

#define DISPLAY_TIMEOUT 15000

// 定义全局变量
static bool quit = false;
static bool suspended = false;
static int input_fd;
static struct input_event ev;
static struct pollfd fds[1];

// 获取图片目录
void getImageDir(const char *theme_path, char *image_dir)
{
    // 尝试加载图片
    char image0_path[STR_MAX * 2];
    sprintf(image0_path, "%s/skin/extra/chargingState0.png", THEME_OVERRIDES);
    if (exists(image0_path)) {
        // 如果图片存在，使用覆盖的主题目录
        sprintf(image_dir, "%s/skin/extra", THEME_OVERRIDES);
        return;
    }

    sprintf(image0_path, "%sskin/extra/chargingState0.png", theme_path);
    if (exists(image0_path)) {
        // 如果图片存在，使用主题目录
        sprintf(image_dir, "%sskin/extra", theme_path);
        return;
    }

    // 否则使用默认目录
    strcpy(image_dir, "res");
}

// 挂起/恢复
void suspend(bool enabled, SDL_Surface *video)
{
    suspended = enabled;
    if (suspended) {
        // 清空视频表面以挂起显示
        SDL_FillRect(video, NULL, 0);
        SDL_Flip(video);
    }
    // system_powersave(suspended);
    // 更新显示状态
    display_setScreen(!suspended);
}

// 信号处理函数
static void sigHandler(int sig)
{
    switch (sig) {
    case SIGINT:
    case SIGTERM:
        quit = true;
        break;
    default:
        break;
    }
}

int main(void)
{
    // 注册信号处理函数
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    bool turn_off = false;

    // 加载系统设置
    settings_load();
    // 设置显示亮度
    display_setBrightness(settings.brightness);

    char image_dir[STR_MAX];
    // 获取图片目录
    getImageDir(settings.theme, image_dir);

    // 获取设备型号
    getDeviceModel();

    // 初始化 SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_EnableKeyRepeat(300, 50);

    // 设置视频模式
    SDL_Surface *video = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE);
    SDL_Surface *screen =
        SDL_CreateRGBSurface(SDL_HWSURFACE, 640, 480, 32, 0, 0, 0, 0);

    int min_delay = 15;
    int frame_delay = 80;
    int frame_count = 0;

    // 存储动画帧的表面数组
    SDL_Surface *frames[24];
    SDL_Surface *image;

    // 加载动画帧
    for (int i = 0; i < 24; i++) {
        char image_path[STR_MAX + 50];
        snprintf(image_path, STR_MAX + 49, "%s/chargingState%d.png", image_dir,
                 i);
        if ((image = IMG_Load(image_path)))
            frames[frame_count++] = image;
    }

    char json_path[STR_MAX + 20];
    snprintf(json_path, STR_MAX + 19, "%s/chargingState.json", image_dir);
    if (is_file(json_path)) {
        int value;
        char json_value[STR_MAX];
        if (file_parseKeyValue(json_path, "frame_delay", json_value, ':', 0) !=
            NULL) {
            value = atoi(json_value);
            // 接受微秒和毫秒两种单位
            frame_delay = value >= 10000 ? value / 1000 : value;
        }
    }

    // 准备轮询按钮输入
    input_fd = open("/dev/input/event0", O_RDONLY);
    memset(&fds, 0, sizeof(fds));
    fds[0].fd = input_fd;
    fds[0].events = POLLIN;

    // 确保帧延迟不小于最小延迟
    if (frame_delay < min_delay)
        frame_delay = min_delay;

    printf_debug("Frame count: %d\n", frame_count);
    printf_debug("Frame delay: %d ms\n", frame_delay);

    bool power_pressed = false;
    int repeat_power = 0;

    int current_frame = 0;

    // 设置 CPU 到省电模式（充电更快？）
    system_powersave_on();

    uint32_t acc_ticks = 0, last_ticks = SDL_GetTicks(),
             display_timer = last_ticks;

    while (!quit) {
        // 轮询按钮输入
        while (poll(fds, 1, suspended ? 1000 - min_delay : 0)) {
            read(input_fd, &ev, sizeof(ev));

            if (ev.type != EV_KEY || ev.value > REPEATING)
                continue;

            if (ev.code == HW_BTN_POWER) {
                if (ev.value == PRESSED) {
                    power_pressed = true;
                    repeat_power = 0;
                }
                else if (ev.value == RELEASED && power_pressed) {
                    if (suspended) {
                        acc_ticks = 0;
                        last_ticks = SDL_GetTicks();
                    }
                    // 挂起/恢复
                    suspend(!suspended, video);
                    power_pressed = false;
                }
                else if (ev.value == REPEATING) {
                    if (repeat_power >= 5) {
                        quit = true; // 启动
                        break;
                    }
                    repeat_power++;
                }
            }

            display_timer = SDL_GetTicks();
        }

        // 如果不在充电，则退出并关闭系统
        if (!battery_isCharging()) {
            quit = true;
            turn_off = true;
            break;
        }

        if (quit)
            break;

        uint32_t ticks = SDL_GetTicks();

        if (!suspended) {
            // 如果超过显示超时时间，则挂起系统
            if (ticks - display_timer >= DISPLAY_TIMEOUT) {
                if (DEVICE_ID == MIYOO354) {
                    quit = true;
                    turn_off = true;
                    break;
                }
                else {
                    suspend(true, video);
                    continue;
                }
            }

            acc_ticks += ticks - last_ticks;
            last_ticks = ticks;

            if (acc_ticks >= frame_delay) {
                // 清空屏幕
                SDL_FillRect(screen, NULL, 0);

                if (current_frame < frame_count) {
                    // 绘制当前帧
                    SDL_Surface *frame = frames[current_frame];
                    SDL_Rect frame_rect = {320 - frame->w / 2,
                                           240 - frame->h / 2};
                    SDL_BlitSurface(frame, NULL, screen, &frame_rect);
                    current_frame = (current_frame + 1) % frame_count;
                }

                // 将绘制好的屏幕内容传输到视频表面
                SDL_BlitSurface(screen, NULL, video, NULL);
                SDL_Flip(video);

                acc_ticks -= frame_delay;
            }
        }

        // 休眠一段时间
        msleep(min_delay);
    }

    // 清空屏幕
    SDL_FillRect(video, NULL, 0);
    SDL_Flip(video);

#ifndef PLATFORM_MIYOOMINI
    msleep(100);
#endif

    // 释放动画帧表面内存
    for (int i = 0; i < frame_count; i++)
        SDL_FreeSurface(frames[i]);
    // 释放屏幕表面内存
    SDL_FreeSurface(screen);
    // 释放视频表面内存
    SDL_FreeSurface(video);
    // 退出 SDL
    SDL_Quit();

    if (turn_off) {
#ifdef PLATFORM_MIYOOMINI
        // 关闭屏幕并关闭系统
        display_setScreen(false);
        system("shutdown; sleep 10");
#endif
    }
    else {
#ifdef PLATFORM_MIYOOMINI
        // 打开屏幕并短暂振动
        display_setScreen(true);
        short_pulse();
#endif
    }

    // 恢复 CPU 性能模式
    system_powersave_off();

    return EXIT_SUCCESS;
}
