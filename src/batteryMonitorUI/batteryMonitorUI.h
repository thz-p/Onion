#ifndef BATTERY_MONITOR_UI_H
#define BATTERY_MONITOR_UI_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <math.h>
#include <signal.h>
#include <sqlite3/sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "system/display.h"
#include "system/keymap_sw.h"
#include "system/system.h"
#include "utils/file.h"
#include "utils/keystate.h"
#include "utils/log.h"
#include "utils/str.h"
#include "utils/surfaceSetAlpha.h"

#define GRAPH_LINE_WIDTH 1 // 图表线宽

// 像素之间的间距，数值越高越透明
#define GRAPH_BACKGROUND_OPACITY 4

// 建议使用4的倍数
#define GRAPH_MAX_FULL_PAGES 8

#define GRAPH_DISPLAY_SIZE_X 583 // 图表显示尺寸 X
#define GRAPH_DISPLAY_SIZE_Y 324 // 图表显示尺寸 Y
#define GRAPH_DISPLAY_START_X 29 // 图表显示起始位置 X
#define GRAPH_DISPLAY_START_Y 79 // 图表显示起始位置 Y
#define GRAPH_DISPLAY_DURATION 16200 // 图表显示时长
#define GRAPH_PAGE_SCROLL_SMOOTHNESS 12 // 图表页滚动平滑度
#define GRAPH_MIN_SESSION_FOR_ESTIMATION 1200 // 用于估算的最小会话时间
#define GRAPH_MAX_PLAUSIBLE_ESTIMATION 54000 // 估算的最大合理时间
#define GRAPH_ESTIMATED_LINE_GAP 20 // 估算线间隔

#define LABEL_Y 410 // 标签 Y 坐标

#define LABEL1_X 150 // 标签1 X 坐标
#define LABEL2_X 278 // 标签2 X 坐标
#define LABEL3_X 407 // 标签3 X 坐标
#define LABEL4_X 538 // 标签4 X 坐标

#define SUB_TITLE_X 255 // 副标题 X 坐标
#define SUB_TITLE_Y 32 // 副标题 Y 坐标

#define LABEL_SESSION_X 100 // 会话标签 X 坐标
#define LABEL_SESSION_Y 439 // 会话标签 Y 坐标
#define LABEL_CURRENT_X 108 // 当前标签 X 坐标
#define LABEL_CURRENT_Y 454 // 当前标签 Y 坐标
#define LABEL_LEFT_X 630 // 剩余标签 X 坐标
#define LABEL_LEFT_Y 438 // 剩余标签 Y 坐标
#define LABEL_BEST_X 630 // 最佳标签 X 坐标
#define LABEL_BEST_Y 453 // 最佳标签 Y 坐标

#define LABEL_SIZE_X 65 // 标签尺寸 X
#define LABEL_SIZE_Y 15 // 标签尺寸 Y

#define RIGHT_ARROW_X 591 // 右箭头 X 坐标
#define RIGHT_ARROW_Y 13 // 右箭头 Y 坐标

#define LEFT_ARROW_X 0 // 左箭头 X 坐标
#define LEFT_ARROW_Y 13 // 左箭头 Y 坐标

#define ARROW_LENGHT 48 // 箭头长度
#define ARROW_WIDTH 28 // 箭头宽度

#endif //BATTERY_MONITOR_UI_H

// 这段代码定义了用于电池监视器界面的宏和常量。
// 它包含了一些用于图表显示、标签位置、箭头位置等的常量定义。这些常量用于界面布局和图表绘制。