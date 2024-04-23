#include "system/axp.h" // 包含 AXP 系统相关的头文件

int main(int argc, char **argv)
{
    unsigned int reg_address, i, j; // 声明寄存器地址、循环变量i、j
    int w_val, val, ret; // 声明写入值、寄存器值、函数返回值

    // 如果没有提供参数，则打印使用说明并显示可用的寄存器及其值
    if (argc < 2) {
        printf("Usage: %s reg_addr [+][write_val]\n\nRegisters:\n", argv[0]);
        for (i = 0; i < 0x100; i += 0x10) {
            printf("%02X :", i);
            for (j = i; j < i + 0x10; j++) {
                val = axp_read(j); // 读取寄存器的值
                if (val < 0)
                    printf(" --"); // 如果值无效，则打印 "--"
                else
                    printf(" %02X", val); // 否则打印十六进制的寄存器值
            }
            printf("\n");
        }
        return 0;
    }

    // 解析寄存器地址参数
    sscanf(argv[1], "%x", &reg_address);

    // 如果只提供了寄存器地址，则读取该寄存器的值并打印
    if (argc == 2) {
        val = axp_read(reg_address); // 读取寄存器的值
        if (val < 0) {
            fprintf(stderr, "axp read error: %d %s\n", val, strerror(errno)); // 打印读取错误信息
            return 1;
        }
        printf("Read %s-%x reg %x, read value:%x\n", AXPDEV, AXPID, reg_address,
               val); // 打印读取的寄存器值
    }
    else {
        // 解析写入值参数
        sscanf(argv[2], "%x", &w_val);
        if ((w_val & ~0xff) != 0)
            fprintf(stderr, "Error on written value %s\n", argv[2]); // 如果写入值超出范围，则打印错误信息
        if (argv[2][0] == '+') {
            // 如果写入值以 "+" 开头，则进行按位或操作
            val = axp_read(reg_address); // 读取寄存器的值
            val |= w_val; // 进行按位或操作
            printf("Bit "); // 打印提示信息
        }
        else
            val = w_val;
        // 将值写入指定的寄存器
        ret = axp_write(reg_address, val); // 写入寄存器的值
        if (ret < 0) {
            fprintf(stderr, "axp write error: %d %s\n", ret, strerror(errno)); // 打印写入错误信息
            return 1;
        }
        printf("Write %s-%x reg %x, write value:%x\n", AXPDEV, AXPID,
               reg_address, val); // 打印写入的寄存器值
    }

    return 0;
}

// 这段代码是一个命令行程序，用于读取或写入指定的寄存器。
// 如果没有提供参数，则打印使用说明并显示可用的寄存器及其值。
// 如果提供了寄存器地址参数，则读取该寄存器的值并打印；
// 如果同时提供了寄存器地址和写入值参数，则将指定的值写入寄存器。