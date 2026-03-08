# Kinetis Agent 团队架构设计文档

## 📋 文档信息

| 项目 | 内容 |
|------|------|
| **版本** | v1.0 |
| **创建日期** | 2026-03-06 |
| **最后更新** | 2026-03-06 |
| **作者** | Auto |
| **项目路径** | `e:/Code/Kinetis` |

---

## 🎯 设计目标

1. **精简高效**：最少化 Agent 数量，降低协作复杂度
2. **功能聚合**：同类功能集中管理，避免碎片化
3. **知识共享**：所有 Agent 共享基础 Linux 内核知识
4. **易于扩展**：新功能扩展现有 Agent，无需频繁创建新 Agent
5. **职责清晰**：每个 Agent 负责一个完整的功能域

---

## 🏗️ 架构总览

```
┌─────────────────────────────────────────────────────────────┐
│                   Coordinator Agent                         │
│              主协调 Agent (coordinator)                       │
│                   负责任务分解、全局调度                        │
└────────────────────────┬────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┬───────────────┐
         ▼               ▼               ▼               ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ flight       │  │ hydrology    │  │ protocol     │  │ peripheral   │
│ Agent        │  │ Agent        │  │ Agent        │  │ Agent        │
│ (飞行控制)    │  │ (水文监测)    │  │ (通信协议)    │  │ (外设管理)    │
└──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘
       │                 │                 │               │
       │                 │                 │               │
       └─────────────────┴─────────┬───────┴───────┬───────┘
                           │                       │
                           ▼                       ▼
                   ┌──────────────┐        ┌──────────────┐
                   │ sensor       │        │ storage      │
                   │ Agent        │        │ Agent        │
                   │ (传感器管理)  │        │ (存储系统)    │
                   └──────┬───────┘        └──────┬───────┘
                          │                       │
                          └───────────┬───────────┘
                                      ▼
                              ┌──────────────┐
                              │ base         │
                              │ Agent        │
                              │ (基础服务)    │
                              └──────┬───────┘
                                     │
                                     ▼
                              ┌──────────────┐
                              │ kernel       │
                              │ Agent        │
                              │ (内核核心)    │
                              └──────────────┘

说明：所有 Agent 都继承 kernel Agent 的 Linux 内核知识
```

---

## 📊 Agent 详细列表

### A01 - kernel Agent（内核核心）

**职责**：
- 提供所有 Agent 共享的 Linux 内核知识
- 内核核心服务（调度、内存、时间、中断）

**负责代码**：
```
kernel/sched/*         # 调度器
kernel/time/*          # 时间管理
kernel/irq/*           # 中断处理
kernel/printk/*        # 日志系统
kernel/locking/*       # 锁机制
mm/*                   # 内存管理
lib/*                  # 基础库
arch/arm/*             # ARM 架构支持
```

**共享知识（所有 Agent 继承）**：
- Linux 设备驱动模型
- kmalloc/kfree 内存分配
- device_register/driver_register 设备注册
- 互斥锁与自旋锁（mutex, spinlock）
- 工作队列（workqueue）
- GPIO 操作 API
- 延时函数（mdelay, udelay）
- 错误处理（ERR_PTR, IS_ERR, PTR_ERR）
- 设备树（device tree）解析

---

### A02 - base Agent（基础服务）

**职责**：
- 设备模型核心
- GPIO 子系统
- 引脚控制（pinctrl）
- Regmap 框架

**负责代码**：
```
drivers/base/drv-core.c           # 设备模型核心
drivers/base/drv-bus.c            # 总线管理
drivers/base/drv-platform.c        # 平台设备
drivers/base/drv-devres.c         # 资源管理
drivers/base/regmap/*             # Regmap 框架
  ├── regmap.c                    # Regmap 核心
  ├── regcache.c                  # 缓存管理
  └── regcache-rbtree.c           # 红黑树缓存
drivers/gpio/*                    # GPIO 子系统
  ├── gpiolib.c                   # GPIO 核心
  ├── gpiolib-devres.c            # GPIO 资源
  └── gpiolib-legacy.c            # 旧版 GPIO
drivers/pinctrl/*                 # 引脚控制
  ├── pinctrl-core.c              # pinctrl 核心
  └── pinctrl-utils.c             # pinctrl 工具
```

**依赖**：
- A01（kernel Agent）- 依赖内核核心服务

**提供服务**：
- gpio_request / gpio_free - GPIO 申请与释放
- gpio_direction_input / gpio_direction_output - GPIO 方向设置
- gpio_get_value / gpio_set_value - GPIO 读写
- regmap_init / regmap_exit - Regmap 初始化/退出
- regmap_write / regmap_read / regmap_update_bits - 寄存器操作
- device_register / device_unregister - 设备注册/注销
- driver_register / driver_unregister - 驱动注册/注销

---

### A03 - sensor Agent（传感器管理）⭐

**职责**：
- 所有传感器驱动管理
- 传感器数据读取与校准
- **I2C/SPI 总线驱动**（软件模拟）
- 传感器数据融合

**负责代码**：
```
drivers/kinetis/mpu6050.c/h            # MPU6050 IMU (6轴)
drivers/kinetis/ak8975.c/h             # AK8975 磁力计 (3轴)
drivers/kinetis/max30205.c/h           # MAX30205 温度传感器
drivers/kinetis/iic_soft.c/h           # I2C 软件模拟驱动 ⭐
drivers/kinetis/spi_soft.c/h           # SPI 软件模拟驱动 ⭐
drivers/kinetis/regmap-user-bus.c/h    # 用户层 Regmap 适配
drivers/kinetis/delay.c/h              # 延时函数
drivers/iio/*                          # 工业I/O框架
  ├── industrialio-core.c              # IIO 核心
  ├── industrialio-buffer.c            # IIO 缓冲
  └── inkern.c                         # IIO 内核接口
```

**重要说明**：
- **I2C 和 SPI 驱动由 sensor Agent 管理**，因为它们主要用于传感器访问
- `iic_soft.c/h` - I2C 软件模拟驱动，提供 bit-bang 实现
- `spi_soft.c/h` - SPI 软件模拟驱动，提供 bit-bang 实现
- 这些总线驱动被 MPU6050、AK8975、MAX30205、DS3231、AT24CXX 等大量传感器使用

**依赖**：
- A01（kernel Agent）- 使用 Linux API（kmalloc, delay 等）
- A02（base Agent）- 使用 GPIO（I2C/SPI 需要操作 GPIO）、Regmap 框架

**领域知识**：
- **I2C 总线时序**：启动、停止、读写、ACK/NACK
- **SPI 总线时序**：时钟极性/相位、片选控制
- **MPU6050**：6轴 IMU（3轴加速度 + 3轴陀螺仪）
- **AK8975**：3轴磁力计
- **MAX30205**：高精度温度传感器（±0.1°C）
- 传感器初始化序列（复位、配置采样率、设置量程）
- 传感器校准算法（偏移校准、比例因子校准）
- 传感器数据融合（MPU6050 + AK8975 → 9轴姿态）

**提供服务**：
```c
// I2C 总线接口
int iic_soft_init(struct iic_master *master);
int iic_soft_read_bytes(struct iic_master *master, uint8_t addr,
                        uint8_t reg, uint8_t *buf, uint16_t len);
int iic_soft_write_bytes(struct iic_master *master, uint8_t addr,
                         uint8_t reg, uint8_t *buf, uint16_t len);

// SPI 总线接口
int spi_soft_init(struct spi_master *master);
int spi_soft_read_bytes(struct spi_master *master, uint8_t *tx_buf,
                        uint8_t *rx_buf, uint16_t len);

// 传感器接口
int mpu6050_init(void);
int mpu6050_read_accel(int16_t *x, int16_t *y, int16_t *z);
int mpu6050_read_gyro(int16_t *x, int16_t *y, int16_t *z);
int ak8975_init(void);
int ak8975_read_mag(int16_t *x, int16_t *y, int16_t *z);
int max30205_read_temp(float *temp);

// 传感器校准
int mpu6050_calibrate(void);
int ak8975_calibrate(void);
```

**总线驱动详情**：

**I2C 软件驱动（iic_soft）**：
- 实现 I2C 协议的 bit-bang 方式
- 支持 SCL/SDA 双向控制
- 支持标准速度（100kHz）和快速模式（400kHz）
- 提供 start/stop/send_byte/receive_byte 等基础操作
- 被 MPU6050、AK8975、MAX30205、DS3231、AT24CXX 等驱动使用

**SPI 软件驱动（spi_soft）**：
- 实现 SPI 协议的 bit-bang 方式
- 支持 CPOL/CPHA 配置（4种模式）
- 支持全双工/半双工模式
- 提供 transfer/write/read 等操作
- 被 W25QXX Flash 等驱动使用

---

### A04 - peripheral Agent（外设管理）

**职责**：
- 所有外设驱动管理
- LED、按键、开关控制
- 显示驱动（LED驱动芯片）

**负责代码**：
```
drivers/kinetis/button.c/h             # 按键驱动
drivers/kinetis/switch.c/h             # 开关驱动
drivers/kinetis/led.c/h                # LED 驱动
drivers/kinetis/my9221.c/h             # MY9221 LED 驱动芯片
drivers/kinetis/tlc5971.c/h            # TLC5971 LED 驱动芯片
drivers/leds/*                         # LED 子系统
  ├── led-class.c                      # LED 设备类
  ├── led-core.c                       # LED 核心
  └── led-triggers.c                   # LED 触发器
drivers/input/*                        # 输入子系统
  ├── input.c                          # 输入核心
  └── keyboard/gpio_keys.c            # GPIO 按键
```

**依赖**：
- A01（kernel Agent）- 使用 Linux API
- A02（base Agent）- 使用 GPIO
- A07（application Agent）- 使用 tim-task 进行去抖动

**领域知识**：
- 按键去抖动算法（硬件去抖 + 软件滤波）
- LED PWM 控制（亮度调节）
- 开关状态机（消抖、长按、双击检测）
- 显示驱动协议（MY9221: 12通道恒流LED，TLC5971: 12通道RGB）
- 输入子系统（input_event, evdev）
- LED 子系统（brightness, triggers）

**提供服务**：
```c
// 按键接口
int button_init(void);
int button_read(int *state);

// 开关接口
int switch_init(void);
int switch_read(int *state);

// LED 接口
int led_init(void);
int led_set(int index, int brightness);
int led_on(int index);
int led_off(int index);
```

---

### A05 - storage Agent（存储系统）

**职责**：
- 文件系统管理
- Flash/EEPROM 驱动
- 内存分配器

**负责代码**：
```
drivers/kinetis/fatfs-intf.c/h        # FATFS 接口层
drivers/kinetis/w25qxxx.c/h           # W25QXX Flash 驱动
drivers/kinetis/at24cxx.c/h           # AT24CXX EEPROM 驱动
drivers/kinetis/memory_allocator.c/h  # 内存分配器
fs/fatfs/*                            # FAT 文件系统
  ├── ff.c                            # FatFs 核心 (260KB)
  ├── ffsystem.c                      # 系统接口
  ├── ffunicode.c                     # Unicode 支持
  ├── diskio.c                        # 磁盘 I/O
  ├── ff_gen_drv.c                    # 通用驱动
  └── drivers/                        # 底层驱动
      ├── flash_diskio.c/h            # Flash 磁盘
      ├── sd_diskio.c/h               # SD 卡
      └── fake_ram_diskio.c/h         # RAM 磁盘
```

**依赖**：
- A01（kernel Agent）- 使用 Linux API
- A02（base Agent）- 使用 GPIO（片选）
- A03（sensor Agent）- 使用 spi_soft（Flash 通过 SPI）、iic_soft（EEPROM 通过 I2C）

**领域知识**：
- FATFS 文件系统（FAT12/FAT16/FAT32）
- W25QXX Flash 驱动（页编程、扇区擦除、全片擦除）
- AT24CXX EEPROM 驱动（字节读写、页写）
- 文件读写操作（open, read, write, close, seek）
- 内存池管理（固定大小块分配）
- 存储可靠性优化（坏块管理、磨损均衡）

**提供服务**：
```c
// 文件系统接口
int file_open(const char *path, const char *mode);
int file_read(int fd, void *buf, size_t len);
int file_write(int fd, const void *buf, size_t len);
int file_close(int fd);

// Flash 接口
int flash_init(void);
int flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
int flash_write(uint32_t addr, const uint8_t *buf, uint32_t len);
int flash_erase(uint32_t addr, uint32_t len);

// EEPROM 接口
int eeprom_init(void);
int eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len);
int eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len);
```

---

### A06 - protocol Agent（通信协议）

**职责**：
- 所有通信协议管理
- 蓝牙、NB-IoT、GPS、RS485

**负责代码**：
```
drivers/kinetis/serial-port.c/h       # 串口核心驱动
drivers/kinetis/hc-05.c/h             # HC-05 蓝牙模块
drivers/kinetis/nb_bc95.c/h           # NB-IoT BC95 模块
drivers/kinetis/nb_app.c/h            # NB-IoT 应用层
drivers/kinetis/nb_board.c/h          # NB-IoT 板级配置
drivers/kinetis/nb_serialport.c/h     # NB-IoT 串口适配
drivers/kinetis/nb_timer.c/h          # NB-IoT 定时器
drivers/kinetis/nema0183.c/h          # NEMA0183 GPS 协议
drivers/kinetis/rs485.c/h             # RS485 通信
drivers/tty/*                         # TTY 终端
  ├── tty_core.c                      # TTY 核心
  └── serial/                         # 串口驱动
```

**依赖**：
- A01（kernel Agent）- 使用 Linux API
- A02（base Agent）- 使用 GPIO（串口 RTS/CTS）
- A07（application Agent）- 使用 tim-task 提供超时

**领域知识**：
- 串口通信（UART, 波特率、数据位、停止位、校验位）
- HC-05 蓝牙协议（AT 命令、SPP 配置文件）
- NB-IoT BC95 模块（CoAP 协议、MQTT、LwM2M）
- NEMA0183 GPS 协议（$GPGGA, $GPRMC 等）
- RS485 通信（差分信号、多主通信）
- AT 命令解析（AT+CMD=xxx, AT+CMD?）
- 数据包编解码（CRC、校验和）

**提供服务**：
```c
// 串口接口
int uart_init(int baudrate);
int uart_send(const uint8_t *buf, uint16_t len);
int uart_receive(uint8_t *buf, uint16_t len);

// 蓝牙接口
int bluetooth_init(void);
int bluetooth_connect(const char *addr);
int bluetooth_send(const uint8_t *buf, uint16_t len);

// NB-IoT 接口
int nbiot_init(void);
int nbiot_send(const char *topic, const char *data);
int nbiot_receive(char *buf, uint16_t len);

// GPS 接口
int gps_init(void);
int gps_parse(char *nema, struct gps_data *data);

// RS485 接口
int rs485_init(int baudrate);
int rs485_send(const uint8_t *buf, uint16_t len);
int rs485_receive(uint8_t *buf, uint16_t len);
```

---

### A07 - application Agent（应用层）

**职责**：
- 飞行控制
- 水文监测（SL651-2014 规约）
- 时间管理
- 任务调度

**负责代码**：
```
drivers/kinetis/flight/*                 # 飞行控制模块
  ├── fmu.c                            # FMU 飞行管理单元
  ├── pid.c                            # PID 控制器
  ├── imu.c                            # IMU 姿态解算
  ├── fmu_rc.c                         # 遥控接口
  ├── fmu_schedule.c                   # 飞行任务调度
  ├── ano_protocol.c                   # ANO 通信协议
  └── filter.c                         # 滤波器
drivers/kinetis/hydrology*              # 水文监测模块
  ├── hydrology.c/h                    # 水文核心 (SL651-2014)
  ├── hydrology-device.c               # 遥测站设备端
  ├── hydrology-host.c                 # 中心站主机端
  ├── hydrology-cmd.c/h                # 命令解析
  ├── hydrology-lib.c                  # 库函数
  ├── hydrology-task.c                 # 任务管理
  ├── hydrology-config.h               # 配置参数
  └── hydrology-identifier.h           # 元素标识符
drivers/kinetis/tim-task.c/h            # 定时任务调度器
drivers/kinetis/ds3231.c/h              # DS3231 RTC
drivers/kinetis/real-time-clock.c/h    # 系统实时时钟
drivers/kinetis/rtc-task.c/h            # RTC 任务管理
```

**依赖**：
- A01（kernel Agent）- 使用 Linux API
- A02（base Agent）- 使用 Regmap（DS3231 通过 I2C）
- A03（sensor Agent）- 使用传感器数据（MPU6050、AK8975 用于飞行控制）
- A04（peripheral Agent）- 使用 LED 指示
- A05（storage Agent）- 存储参数到文件系统
- A06（protocol Agent）- 数据上报和通信

**领域知识**：
- **FMU 飞行管理单元**：姿态控制、高度控制、航向控制
- **PID 控制算法**：P、I、D 参数整定、抗积分饱和
- **IMU 姿态解算**：四元数、欧拉角、DMP 融合
- **SL651-2014 水文规约**：遥测站协议、中心站协议
- **水文数据格式**：雨量、水位、流量、水质等要素
- **DS3231 RTC**：高精度实时时钟（±2ppm）
- **任务调度**：tim-task 提供的周期性任务调度
- **ANO 协议**：飞行数据通信协议

**提供服务**：
```c
// 飞行控制接口
int fmu_init(void);
int fmu_set_attitude(float roll, float pitch, float yaw);
int fmu_get_attitude(float *roll, float *pitch, float *yaw);
int pid_set_param(int axis, float kp, float ki, float kd);

// 水文接口
int hydrology_init(void);
int hydrology_report(struct hydrology_data *data);
int hydrology_parse_command(const char *cmd, struct hydrology_cmd *result);

// 时间接口
int rtc_init(void);
int rtc_gettime(struct rtc_time *time);
int rtc_settime(const struct rtc_time *time);

// 任务调度接口
int tim_task_add(struct tim_task *task, void (*func)(void *), void *arg, uint32_t period);
int tim_task_del(struct tim_task *task);
```

---

## 🔄 Agent 协作关系

### 依赖关系图

```
A07 (application)
├── A03 (sensor) ────┐
│                    ├── A02 (base)
│                    │       └── A01 (kernel)
├── A05 (storage) ───┤
│                    └── A03 (sensor) ─┐
├── A06 (protocol) ──┤                  │
│                    └── A02 (base) ────┴── A01 (kernel)
└── A04 (peripheral) ──┐
                       └── A02 (base) ──────── A01 (kernel)

说明：
- A01 是最底层，所有其他 Agent 都依赖它
- A02 提供基础服务（GPIO、Regmap）
- A03 提供传感器和总线驱动
- A04、A05、A06、A07 是上层应用
```

### 典型协作场景

#### 场景 1：飞行控制姿态解算

```
用户请求: "解算飞行姿态"
  │
  ▼
Coordinator Agent
  │
  ├─→ A07 (application): 负责飞控逻辑
  │       │
  │       ├─→ A03 (sensor): 读取 MPU6050 + AK8975 数据
  │       │       │
  │       │       ├─→ iic_soft: 通过 I2C 读取传感器
  │       │       └─→ regmap-user-bus: 寄存器映射
  │       │
  │       └─→ A05 (storage): 从文件系统读取 PID 参数
  │
  └─→ A01 (kernel): 提供系统时间戳

协作链: Coordinator → A07 → A03 → A02 → A01
```

#### 场景 2：水文数据上报

```
用户请求: "上报水文数据"
  │
  ▼
Coordinator Agent
  │
  ├─→ A07 (application): 水文数据处理
  │       │
  │       ├─→ A05 (storage): 从 Flash 读取历史数据
  │       │       │
  │       │       └─→ A03 (sensor): Flash 通过 SPI 读写
  │       │
  │       └─→ A06 (protocol): 通过 NB-IoT 上报
  │               │
  │               └─→ serial-port: 串口通信
  │
  └─→ A04 (peripheral): LED 指示灯闪烁

协作链: Coordinator → A07 → A05/A06 → A03/A04 → A02 → A01
```

#### 场景 3：按键检测

```
用户请求: "检测按键状态"
  │
  ▼
Coordinator Agent
  │
  └─→ A04 (peripheral): 按键读取
          │
          └─→ A07 (application): tim-task 去抖动
                  │
                  └─→ A01 (kernel): 延时函数

协作链: Coordinator → A04 → A07 → A01
```

#### 场景 4：Flash 读写

```
用户请求: "读取 Flash 数据"
  │
  ▼
Coordinator Agent
  │
  └─→ A05 (storage): Flash 操作
          │
          └─→ A03 (sensor): 使用 spi_soft 驱动
                  │
                  └─→ A02 (base): GPIO 片选控制
                          │
                          └─→ A01 (kernel): 延时函数

协作链: Coordinator → A05 → A03 → A02 → A01
```

---

## 💡 关键设计决策

### 1. I2C/SPI 驱动的归属

**决策**：I2C 和 SPI 驱动由 **A03 (sensor Agent)** 管理

**理由**：
- 项目中 I2C 和 SPI 的主要使用者是传感器（MPU6050、AK8975、MAX30205、DS3231 等）
- 将总线驱动与传感器驱动放在同一个 Agent，便于整体优化时序和性能
- 减少跨 Agent 调用，提高协作效率
- 便于积累 I2C/SPI 传感器的共性知识（时序优化、错误处理等）

**如果未来有非传感器设备使用 I2C/SPI**：
- 可以通过 A03 的服务接口访问
- 或者将 I2C/SPI 驱动迁移到 A02（base Agent），作为更底层的总线服务

### 2. 所有 Agent 共享 Linux 内核知识

**决策**：通过继承机制，所有 Agent 都自动获得 A01 的 Linux 内核知识

**理由**：
- Linux 内核 API 是所有驱动开发的基石
- 避免在每个 Agent 中重复定义相同的 Linux 知识
- 统一的 Linux API 调用规范，保证代码风格一致
- 降低 Agent 间的知识冗余

### 3. 按功能域聚合而非按模块拆分

**决策**：同类功能集中管理（所有传感器在 A03，所有外设在 A04）

**理由**：
- 减少跨 Agent 调用（例如，读取 MPU6050 和 AK8975 只需调用 A03）
- 便于积累该功能域的共性知识
- 扩展性强（新增传感器只需扩展 A03）
- 职责清晰，易于理解

---

## 📈 扩展性分析

### 添加新传感器

**操作**：在 A03 (sensor Agent) 中添加新传感器驱动

**示例**：添加 BMP280 气压传感器
```yaml
# A03-sensor-agent.yaml 更新
code_files:
  - drivers/kinetis/bmp280.c/h  # 新增

domain_knowledge:
  - "BMP280: 气压传感器"

services:
  - name: "bmp280_init"
  - name: "bmp280_read_pressure"
```

**优点**：无需创建新 Agent，直接扩展现有 Agent

### 添加新总线

**操作**：在 A03 (sensor Agent) 中添加新总线驱动

**示例**：添加硬件 I2C 驱动
```yaml
code_files:
  - drivers/kinetis/iic_hw.c/h  # 新增硬件 I2C

domain_knowledge:
  - "硬件 I2C 驱动（高速度）"
```

**优点**：与现有的 I2C/SPI 驱动统一管理

### 添加新外设

**操作**：在 A04 (peripheral Agent) 中添加新外设驱动

**示例**：添加蜂鸣器驱动
```yaml
code_files:
  - drivers/kinetis/buzzer.c/h  # 新增
```

### 添加新通信协议

**操作**：在 A06 (protocol Agent) 中添加新协议

**示例**：添加 LoRa 协议
```yaml
code_files:
  - drivers/kinetis/lora.c/h  # 新增
```

---

## 🗂️ 配置文件结构

```
.codebuddy/agents/
├── ARCHITECTURE.md              # 本文档（架构设计）
├── coordinator-agent.yaml       # 主协调 Agent
├── A01-kernel-agent.yaml        # 内核核心
├── A02-base-agent.yaml          # 基础服务
├── A03-sensor-agent.yaml        # 传感器管理 + I2C/SPI ⭐
├── A04-peripheral-agent.yaml    # 外设管理
├── A05-storage-agent.yaml       # 存储系统
├── A06-protocol-agent.yaml     # 通信协议
└── A07-application-agent.yaml   # 应用层
```

---

## 🎯 总结

### 方案优势

1. **精简高效**：从 17 个 Agent 精简到 7 个，协作路径清晰
2. **功能聚合**：同类功能集中管理，避免碎片化
3. **知识共享**：所有 Agent 共享 Linux 内核知识（A01）
4. **易于扩展**：新功能扩展现有 Agent，无需频繁创建新 Agent
5. **职责明确**：每个 Agent 负责一个完整的功能域
6. **总线归属清晰**：I2C/SPI 由 sensor Agent 管理，与传感器紧密协作

### I2C/SPI 归属说明

**明确**：I2C 和 SPI 软件驱动（`iic_soft.c/h`, `spi_soft.c/h`）由 **A03 (sensor Agent)** 管理

**原因**：
- 项目中 I2C/SPI 的主要使用者是传感器
- 便于整体优化总线时序和性能
- 减少跨 Agent 调用
- 便于积累总线与传感器的协同优化知识

---

## 📝 附录：关键代码文件索引

| 功能域 | 代码文件 | 归属 Agent |
|--------|---------|-----------|
| **I2C 驱动** | `drivers/kinetis/iic_soft.c/h` | A03 (sensor) ⭐ |
| **SPI 驱动** | `drivers/kinetis/spi_soft.c/h` | A03 (sensor) ⭐ |
| **MPU6050** | `drivers/kinetis/mpu6050.c/h` | A03 (sensor) |
| **AK8975** | `drivers/kinetis/ak8975.c/h` | A03 (sensor) |
| **MAX30205** | `drivers/kinetis/max30205.c/h` | A03 (sensor) |
| **按键** | `drivers/kinetis/button.c/h` | A04 (peripheral) |
| **开关** | `drivers/kinetis/switch.c/h` | A04 (peripheral) |
| **LED** | `drivers/kinetis/led.c/h` | A04 (peripheral) |
| **Flash** | `drivers/kinetis/w25qxxx.c/h` | A05 (storage) |
| **EEPROM** | `drivers/kinetis/at24cxx.c/h` | A05 (storage) |
| **FATFS** | `fs/fatfs/*` | A05 (storage) |
| **串口** | `drivers/kinetis/serial-port.c/h` | A06 (protocol) |
| **蓝牙** | `drivers/kinetis/hc-05.c/h` | A06 (protocol) |
| **NB-IoT** | `drivers/kinetis/nb_*.c/h` | A06 (protocol) |
| **GPS** | `drivers/kinetis/nema0183.c/h` | A06 (protocol) |
| **RS485** | `drivers/kinetis/rs485.c/h` | A06 (protocol) |
| **飞行控制** | `drivers/kinetis/flight/*` | A07 (application) |
| **水文监测** | `drivers/kinetis/hydrology*` | A07 (application) |
| **RTC** | `drivers/kinetis/ds3231.c/h` | A07 (application) |
| **任务调度** | `drivers/kinetis/tim-task.c/h` | A07 (application) |
| **内核核心** | `kernel/*`, `mm/*`, `lib/*`, `arch/*` | A01 (kernel) |
| **设备模型** | `drivers/base/*`, `drivers/gpio/*` | A02 (base) |

---

**文档结束**
