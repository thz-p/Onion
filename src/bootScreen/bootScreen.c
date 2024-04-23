#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "system/battery.h"
#include "system/settings.h"
#include "theme/config.h"
#include "theme/render/battery.h"
#include "theme/resources.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/log.h"

int main(int argc, char *argv[])
{
    // Boot : 加载屏幕
    // End_Save : 带保存的结束屏幕
    // End : 不带保存的结束屏幕

    // 初始化 SDL
    SDL_Init(SDL_INIT_VIDEO);

    // 获取主题路径
    char theme_path[STR_MAX];
    theme_getPath(theme_path);

    // 创建视频和屏幕表面
    SDL_Surface *video = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE);
    SDL_Surface *screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 640, 480, 32, 0, 0, 0, 0);

    // 背景表面、显示电池标志和显示版本号的标志
    SDL_Surface *background;
    bool show_battery = false;
    bool show_version = true;

    // 根据命令行参数加载不同的背景图像
    if (argc > 1 && strcmp(argv[1], "End_Save") == 0) {
        background = theme_loadImage(theme_path, "extra/Screen_Off_Save");
        show_battery = true;
    }
    else if (argc > 1 && strcmp(argv[1], "End") == 0) {
        background = theme_loadImage(theme_path, "extra/Screen_Off");
        show_battery = true;
    }
    else if (argc > 1 && strcmp(argv[1], "lowBat") == 0) {
        background = theme_loadImage(theme_path, "extra/lowBat");
        if (!background) {
            show_battery = true;
        }
        show_version = false;
    }
    else {
        background = theme_loadImage(theme_path, "extra/bootScreen");
    }

    // 如果背景加载成功，将其绘制到屏幕上
    if (background) {
        SDL_BlitSurface(background, NULL, screen, NULL);
        SDL_FreeSurface(background);
    }

    // 初始化 TTF 字体
    TTF_Init();

    // 加载主题字体和颜色
    TTF_Font *font = theme_loadFont(theme_path, theme()->hint.font, 18);
    SDL_Color color = theme()->total.color;

    // 如果需要显示版本号，从文件中读取版本号并在屏幕上显示
    if (show_version) {
        const char *version_str = file_read("/mnt/SDCARD/.tmp_update/onionVersion/version.txt");
        if (strlen(version_str) > 0) {
            SDL_Surface *version = TTF_RenderUTF8_Blended(font, version_str, color);
            if (version) {
                SDL_BlitSurface(version, NULL, screen, &(SDL_Rect){20, 450 - version->h / 2});
                SDL_FreeSurface(version);
            }
        }
    }

    // 如果有消息文本，绘制到屏幕上
    char message_str[STR_MAX] = "";
    if (argc > 2) {
        strncpy(message_str, argv[2], STR_MAX - 1);
    }
    if (strlen(message_str) > 0) {
        SDL_Surface *message = TTF_RenderUTF8_Blended(font, message_str, color);
        if (message) {
            SDL_BlitSurface(message, NULL, screen, &(SDL_Rect){620 - message->w, 450 - message->h / 2});
            SDL_FreeSurface(message);
        }
    }

    // 如果需要显示电池状态，获取电池百分比并绘制到屏幕上
    if (show_battery) {
        SDL_Surface *battery = theme_batterySurface(battery_getPercentage());
        SDL_Rect battery_rect = {596 - battery->w / 2, 30 - battery->h / 2};
        SDL_BlitSurface(battery, NULL, screen, &battery_rect);
        SDL_FreeSurface(battery);
        resources_free();
    }

    // 双缓冲，清空视频缓冲区
    SDL_BlitSurface(screen, NULL, video, NULL);
    SDL_Flip(video);
    SDL_BlitSurface(screen, NULL, video, NULL);
    SDL_Flip(video);

    // 如果不是启动模式，则将 .offOrder 标志设置为 false
    if (argc > 1 && strcmp(argv[1], "Boot") != 0)
        temp_flag_set(".offOrder", false);

    // 在 MIYOO MINI 平台上等待 4 秒钟（调试用）
#ifndef PLATFORM_MIYOOMINI
    sleep(4);
#endif

    // 释放资源，关闭 SDL
    SDL_FreeSurface(screen);
    SDL_FreeSurface(video);
    SDL_Quit();

    return EXIT_SUCCESS;
}

// 这段代码主要用于显示不同场景下的屏幕内容，
// 具体根据命令行参数来选择显示的内容，如启动画面、结束画面等。