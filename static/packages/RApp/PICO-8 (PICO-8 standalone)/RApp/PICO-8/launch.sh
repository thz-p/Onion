#!/bin/sh
echo $0 $*
picodir=$(dirname "$0")  # 获取当前脚本的所在目录，并赋值给变量 picodir
export picodir="$picodir"  # 将 picodir 导出，使其在其他脚本或程序中可用
export picoconfig="/mnt/SDCARD/Saves/CurrentProfile/saves/PICO-8"  # 设置 PICO-8 配置目录的路径，并导出
export rompath="$1"  # 将传递给脚本的 PICO-8 游戏 ROM 的路径赋值给变量 rompath，并导出
export filename=$(basename "$rompath")  # 从 rompath 中提取 PICO-8 游戏的文件名，并导出

cd $picodir  # 切换当前工作目录到脚本所在目录

# 添加脚本目录下的 bin 子目录到 PATH 环境变量中
export PATH=$PATH:$PWD/bin

# 设置 PICO-8 配置目录为 HOME 目录
export HOME=$picoconfig

# 设置 BBS（BBS 车带系统）目录的路径，即存放 BBS 车带的位置
export BBS_DIR="/mnt/SDCARD/Roms/PICO/splore/"

# 如果 BBS 目录不存在，则创建
if [ ! -d "$BBS_DIR" ]; then
    mkdir -p "$BBS_DIR"
fi

# 一些用户报告启动时出现黑屏。我们将检查文件是否存在，然后检查键是否与已知的良好配置匹配
fixconfig() {
    config_file="${picoconfig}/.lexaloffle/pico-8/config.txt"  # 设置配置文件的路径

    # 如果配置文件不存在，则输出提示信息
    if [ ! -f "$config_file" ]; then
        echo "Config file not found, creating with default values."
    fi

    echo "Config checker: Validating display settings in config.txt"  # 输出检查显示设置的提示信息

    # 设置各个显示相关的默认值
    set_window_size="window_size 640 480"
    set_screen_size="screen_size 640 480"
    set_windowed="windowed 0"
    set_window_position="window_position -1 -1"
    set_frameless="frameless 1"
    set_fullscreen_method="fullscreen_method 2"
    set_blit_method="blit_method 0"
    set_transform_screen="transform_screen 134"
    set_host_framerate_control="host_framerate_control 0"

    # 遍历各个显示设置，并根据需要更新或添加到配置文件中
    for setting in window_size screen_size windowed window_position frameless fullscreen_method blit_method transform_screen host_framerate_control; do
        case $setting in
        window_size) new_value="$set_window_size" ;;  # 根据设置的情况选择相应的新值
        screen_size) new_value="$set_screen_size" ;;
        windowed) new_value="$set_windowed" ;;
        window_position) new_value="$set_window_position" ;;
        frameless) new_value="$set_frameless" ;;
        fullscreen_method) new_value="$set_fullscreen_method" ;;
        blit_method) new_value="$set_blit_method" ;;
        transform_screen) new_value="$set_transform_screen" ;;
        host_framerate_control) new_value="$set_host_framerate_control" ;;
        esac

        # 检查配置文件中是否包含当前设置，并根据需要更新或添加到配置文件中
        if grep -q "^$setting" "$config_file"; then
            sed -i "s/^$setting.*/$new_value/" "$config_file"  # 替换配置文件中已有的设置
            echo "Updated setting: $setting"
        else
            echo "$new_value" >>"$config_file"  # 将新的设置添加到配置文件中
            echo "Added missing setting: $setting"
        fi
    done

    echo "Updated settings:"  # 输出更新后的设置
    grep -E "window_size|screen_size|windowed|window_position|frameless|fullscreen_method|blit_method|transform_screen|host_framerate_control" "$config_file"
}

purge_devil() {
    # 检查是否有名为 "/dev/l" 的进程正在运行，如果有则终止它
    if pgrep -f "/dev/l" >/dev/null; then
        echo "Process /dev/l is running. Killing it now..."
        killall -2 l
    else
        echo "Process /dev/l is not running."
    fi

    # 处理 pico-8 的第二次启动，如果 /dev/l 已经被 disp_init 替换
    if pgrep -f "disp_init" >/dev/null; then
        echo "Process disp_init is running. Killing it now..."
        killall -9 disp_init
    else
        echo "Process disp_init is not running."
    fi
}

start_pico() {
    # 检查是否存在 PICO-8 的动态链接库文件，如果不存在则显示信息面板并退出
    if [ ! -e "$picodir/bin/pico8_dyn" ]; then
        infoPanel --title "PICO-8 binaries not found" --message "Native PICO-8 engine requires to purchase official \nbinaries which are not provided in Onion. \nGo to Lexaloffle's website, get Raspberry Pi version\n and copy \"pico8_dyn\" and \"pico8.dat\"\n to your SD card in \"/RApp/PICO-8/bin\"."
        cd /mnt/SDCARD/.tmp_update/script
        ./remove_last_recent_entry.sh
        exit
    fi

    # 调用 purge_devil 函数，用于终止相关进程
    purge_devil

    # 设置 LD_LIBRARY_PATH 和各种 SDL 变量
    export LD_LIBRARY_PATH="$picodir/lib:$LD_LIBRARY_PATH"
    export SDL_VIDEODRIVER=mmiyoo
    export SDL_AUDIODRIVER=mmiyoo
    export EGL_VIDEODRIVER=mmiyoo

    # 修复配置文件中的显示设置
    fixconfig

    # 调用 stop_audioserver.sh 脚本停止音频服务器
    . /mnt/SDCARD/.tmp_update/script/stop_audioserver.sh

    # 如果启动的是 Splore，则在启动前后检查 BBS_DIR 目录下的文件数量
    if [ "$filename" = "~Run PICO-8 with Splore.png" ]; then
        num_files_before=$(ls -1 "$BBS_DIR" | wc -l)
        LD_PRELOAD="$picodir/lib/libcpt_hook.so" pico8_dyn -splore -preblit_scale 3
        num_files_after=$(ls -1 "$BBS_DIR" | wc -l)
        if [ "$num_files_before" -ne "$num_files_after" ]; then
            rm -f /mnt/SDCARD/Roms/PICO/PICO_cache6.db
        fi
    else
        # 启动 PICO-8 运行指定的游戏
        LD_PRELOAD="$picodir/lib/libcpt_hook.so" pico8_dyn -preblit_scale 3 -run "$rompath"
    fi
}

main() {
    echo performance >/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
    sv=$(cat /proc/sys/vm/swappiness)
    echo 10 >/proc/sys/vm/swappiness
    start_pico
    disp_init & # re-init mi_disp and push csc in
    echo $sv >/proc/sys/vm/swappiness
}

main