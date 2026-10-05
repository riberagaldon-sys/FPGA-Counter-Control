# GX 数码管计数器与 LED 控制终端

## 开发环境

- Qt Creator 20.0.1
- Qt 6.11.1 MinGW 64-bit
- CMake + Ninja
- Qt 模块：Widgets、SerialPort
- 串口：115200、8 数据位、无校验、1 停止位、无流控

工程使用 Qt Widgets 和 CMake，可直接在 Qt Creator 中打开根目录的 `CMakeLists.txt`。

## 界面功能

- 8 位大尺寸十六进制数码管，与 FPGA `PO[31:0]` 同步。
- 12 个大尺寸 LED，与 FPGA `PIO[11:0]` one-hot 状态同步。
- F1~F8 保留参考面板布局，但全部置灰、不可点击。
- F9 为“清零”，向 FPGA 发送 `C\n`。
- F10 为“调频”，向 FPGA 发送 `F\n`。
- 键盘实体 F9、F10 与界面按钮作用相同。
- “核心板六键功能状态”区域同步显示暂停/继续、清零、递增、递减、LED 方向和调频状态。
- LCD1602 详细区同时显示硬件两行内容和完整参数。

## LCD1602 详细参数

界面显示以下全部参数：

- 串口端口、本地串口开关状态、FPGA 心跳联机状态、115200/8N1 配置
- 100 MHz 系统时钟、当前更新频率、分频系数、五档频率索引
- 32 位计数 HEX/DEC、运行/暂停状态、递增/递减方向
- 12 位 LED 掩码、当前亮灯、LED 滚动方向
- 最近按键功能、事件序号、接收状态包数、发送命令数、最后更新时间

界面中的两行绿色 LCD 区与硬件 LCD1602 保持一致：

```text
UART:ONLINE
INC     F:1.00Hz
```

## 连接步骤

1. 先烧录配套 Vivado 工程，并确认底板 LCD1602 总线选择为 C 组 B4。
2. 用核心板 CH340E USB-UART 连接电脑。
3. 启动本终端，点击“刷新端口”。
4. 选择 CH340 对应 COM 口后点击“连接”。
5. 软件会每 500 ms 自动发送一次心跳；通常 1 秒内顶部状态变成“FPGA 联机”，硬件 LCD 第一行变为 `UART:ONLINE`。

如果一直显示“串口已开 · 等待 FPGA”，不需要一直等待：检查选中的是否为 CH340 COM 口、FPGA 是否烧录了本次 bitstream、波特率是否为 115200，以及 RX/TX 管脚是否使用 `AA14/V14`。

## 在 Qt Creator 中编译

1. 打开 Qt Creator。
2. 选择“打开文件或项目”，打开本目录 `CMakeLists.txt`。
3. Kit 选择 `Desktop Qt 6.11.1 MinGW 64-bit`。
4. 点击“配置项目”，再点击“构建”。
5. 运行目标选择 `GXCounterTerminal`。

也可以直接运行 `build_debug.bat`。脚本按现有环境默认使用：

- `D:\Qt\6.11.1\mingw_64`
- `D:\Qt\Tools\CMake_64\bin\cmake.exe`
- `D:\Qt\Tools\Ninja\ninja.exe`

若安装目录不同，只改脚本开头的三个变量。

## 串口数据格式

FPGA 状态包：

```text
S,CCCCCCCC,R,D,L,I,DDDDDDDD,LLL,AA,O,EEEE\r\n
```

例如：

```text
S,12AB34CD,1,U,R,2,05F5E100,020,04,1,003A
```

表示计数值 `12AB34CD`、运行、递增、LED 向右、频率档 2、分频系数 100,000,000、LED6 点亮、最近功能为递增、UART 心跳联机、事件序号 58。

## 离线界面预览

调试界面布局时可在命令行运行：

```text
GXCounterTerminal.exe --demo
```

该模式只加载演示数据，不访问串口，不影响正常运行。
