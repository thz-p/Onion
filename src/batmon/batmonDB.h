#ifndef BATMON_DB_H
#define BATMON_DB_H
#define BATTERY_LOG_FILE "/mnt/SDCARD/Saves/Common/battery_logs/battery_logs.sqlite"

sqlite3 *bat_log_db = NULL;

// 打开电池日志数据库函数
int open_battery_log_db(void)
{
    bool bat_log_db_created = is_file(BATTERY_LOG_FILE); // 检查电池日志数据库是否已创建

    // 如果电池日志数据库未创建，则创建相应文件夹
    if (!bat_log_db_created) {
        mkdir("/mnt/SDCARD/Saves/Common", 0777);
        mkdir("/mnt/SDCARD/Saves/Common/battery_logs", 0777);
    }

    // 打开电池日志数据库文件
    if (sqlite3_open(BATTERY_LOG_FILE, &bat_log_db) != SQLITE_OK) {
        printf("%s\n", sqlite3_errmsg(bat_log_db)); // 输出错误信息
        sqlite3_close(bat_log_db); // 关闭数据库连接
        bat_log_db = NULL;
        return -1; // 返回错误代码
    }

    // 如果电池日志数据库未创建，则创建表格
    if (!bat_log_db_created) {
        // 创建电池活动记录表
        sqlite3_exec(bat_log_db,
                     "DROP TABLE IF EXISTS bat_activity;"
                     "CREATE TABLE bat_activity(id INTEGER PRIMARY KEY, device_serial TEXT, bat_level INTEGER, duration INTEGER, is_charging INTEGER);"
                     "CREATE INDEX bat_activity_device_SN_index ON bat_activity(device_serial);",
                     NULL, NULL, NULL);
        // 创建设备特定信息表
        sqlite3_exec(bat_log_db,
                     "DROP TABLE IF EXISTS device_specifics;"
                     "CREATE TABLE device_specifics(id INTEGER PRIMARY KEY, device_serial TEXT, best_session INTEGER);"
                     "CREATE INDEX device_specifics_index ON device_specifics(device_serial);",
                     NULL, NULL, NULL);
    }

    return 1; // 返回成功代码
}

// 关闭电池日志数据库函数
void close_battery_log_db(void)
{
    sqlite3_close(bat_log_db); // 关闭数据库连接
    bat_log_db = NULL; // 将数据库指针置为空
}

// 获取最佳使用时长函数
int get_best_session_time(void)
{
    int best_time = 0; // 初始化最佳使用时长为0

    if (open_battery_log_db() == 1) { // 打开电池日志数据库
        if (bat_log_db != NULL) { // 检查数据库指针是否有效
            const char *sql = "SELECT * FROM device_specifics WHERE device_serial = ? ORDER BY id LIMIT 1;"; // 查询语句
            sqlite3_stmt *stmt; // 声明 SQLite 语句对象

            int rc = sqlite3_prepare_v2(bat_log_db, sql, -1, &stmt, 0); // 准备查询语句
            if (rc == SQLITE_OK) { // 检查准备是否成功
                sqlite3_bind_text(stmt, 1, DEVICE_SN, -1, SQLITE_STATIC); // 绑定设备序列号参数
                rc = sqlite3_step(stmt); // 执行查询
                if (rc == SQLITE_ROW) { // 检查是否有结果
                    best_time = sqlite3_column_int(stmt, 2); // 获取最佳使用时长
                }
                else { // 如果没有结果
                    char *sql2 = sqlite3_mprintf("INSERT INTO device_specifics(device_serial, best_session) VALUES(%Q, %d);", DEVICE_SN, 0); // 构建插入语句
                    sqlite3_exec(bat_log_db, sql2, NULL, NULL, NULL); // 执行插入语句
                    sqlite3_free(sql2); // 释放内存
                }
            }
            sqlite3_finalize(stmt); // 释放语句对象
        }
    }
    return best_time; // 返回最佳使用时长
}

#endif // BATMON_DB_H