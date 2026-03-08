# Kinetis 项目编码习惯记录

## 📋 文档信息

| 项目 | 内容 |
|------|------|
| **版本** | v1.0 |
| **创建日期** | 2026-03-08 |
| **最后更新** | 2026-03-08 |
| **作者** | Auto |
| **项目路径** | `e:/Code/Kinetis` |

---

## 🎯 编码习惯总结

### 1. 设备驱动架构模式

**核心原则**：
- 所有传感器驱动必须使用 `struct xxx_device` 封装设备状态
- 必须使用 regmap API 进行所有硬件访问
- 所有新 API 函数接受 `struct xxx_device *dev` 作为第一个参数
- 保留向后兼容的包装函数，使用全局设备指针

**标准模式**：
```c
/* 1. 设备结构体定义 */
struct mpu6050_device {
    struct regmap *regmap;
    u8 temp_sensitivity;
    u16 gyro_full_scale;
    // ... 设备特定字段
};

/* 2. 全局设备指针（向后兼容） */
static struct mpu6050_device *g_mpu6050_dev = NULL;

/* 3. 新 API 函数（推荐） */
struct mpu6050_device *mpu6050_device_init(
    enum regmap_user_bus_type bus_type, 
    void *bus_master
) {
    // 初始化逻辑
}

/* 4. 向后兼容包装函数 */
void mpu6050_initialize_legacy(void) {
    g_mpu6050_dev = mpu6050_device_init(REGMAP_BUS_IIC_SOFT, mpu6050_iic);
}
```

### 2. Regmap 使用模式

**必须使用 regmap API，禁止直接调用端口函数**：

```c
/* ✅ 正确：使用 regmap API */
regmap_read(dev->regmap, MPU6050_REG_PWR_MGMT_1, &val);
regmap_write(dev->regmap, MPU6050_REG_PWR_MGMT_1, 0x00);
regmap_bulk_read(dev->regmap, ACCEL_XOUT_H, buf, 6);
regmap_bulk_write(dev->regmap, MPU6050_REG_FIFO, buf, len);

/* ❌ 错误：直接调用端口函数 */
mpu6050_port_receive(dev, addr, &val);
mpu6050_port_transmit(dev, addr, val);
mpu6050_port_multi_receive(dev, addr, buf, len);
```

**Regmap 配置标准**：
```c
static const struct regmap_config mpu6050_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x75,
    .volatile_table = &mpu6050_volatile_table,
    .cache_type = REGCACHE_NONE,
};
```

### 3. 命名习惯

#### 3.1 函数命名
- **格式**：`模块名_功能描述`
- **示例**：
  - `mpu6050_init()` - MPU6050 初始化
  - `ds3231_get_time()` - DS3231 获取时间
  - `ak8975_read_mag()` - AK8975 读取磁场

#### 3.2 变量命名
- **局部变量**：简洁的小写 + 下划线
  ```c
  u8 tmp;
  u16 val;
  u32 reg;
  ```
- **全局变量**：`g_` 前缀
  ```c
  static struct mpu6050_device *g_mpu6050_dev;
  static mpu6050_config_t g_mpu6050_config;
  static u8 g_mpu6050_initialized;
  ```

#### 3.3 结构体命名
- **格式**：`小写_下划线 + _device 后缀`
  ```c
  struct mpu6050_device { ... };
  struct ds3231_device { ... };
  ```

#### 3.4 常量和宏命名
- **格式**：`大写 + 下划线`
  ```c
  #define MPU6050_REG_PWR_MGMT_1  0x6B
  #define MPU6050_ADDR_DEFAULT     0x68
  ```

### 4. 数据类型使用习惯

**优先使用的类型**（而非标准类型）：
```c
u8   // 无符号 8 位
u16  // 无符号 16 位
u32  // 无符号 32 位
s8   // 有符号 8 位
s16  // 有符号 16 位
s32  // 有符号 32 位
```

**避免使用**：
```c
uint8_t, uint16_t, uint32_t
int8_t, int16_t, int32_t
```

### 5. 注释风格习惯

#### 5.1 Doxygen 格式
所有公开 API 必须有 Doxygen 注释：
```c
/**
 * @brief Initialize MPU6050 sensor
 * @param dev: Device pointer
 * @param bus_type: Bus type (REGMAP_BUS_IIC_SOFT, REGMAP_BUS_SPI_SOFT)
 * @param bus_master: Bus master pointer
 * @return Device pointer on success, ERR_PTR on failure
 * @note This function initializes MPU6050 with default settings
 */
struct mpu6050_device *mpu6050_init(...);
```

#### 5.2 代码内注释
- 简洁扼要
- 解释"为什么"而非"是什么"
- 行尾注释对齐
```c
/* Configure PWR_MGMT_1 to wake up device */
regmap_write(dev->regmap, MPU6050_REG_PWR_MGMT_1, 0x00);

mdelay(100);  // Wait for device to stabilize
```

### 6. 错误处理习惯

#### 6.1 内存分配检查
```c
dev = kzalloc(sizeof(*dev), GFP_KERNEL);
if (!dev)
    return ERR_PTR(-ENOMEM);
```

#### 6.2 Regmap 操作检查
```c
ret = regmap_read(dev->regmap, reg, &val);
if (ret < 0)
    return ret;
```

#### 6.3 错误指针处理
```c
if (IS_ERR(dev)) {
    pr_err("Failed to allocate device: %ld\n", PTR_ERR(dev));
    return PTR_ERR(dev);
}
```

#### 6.4 日志输出
```c
pr_info("MPU6050 initialized successfully\n");
pr_err("Failed to read register 0x%02x\n", reg);
pr_warn("Sensor data overflow detected\n");
pr_debug("Debug information: val=0x%02x\n", val);
```

### 7. 代码组织习惯

#### 7.1 头文件包含顺序
```c
/* 系统头文件 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

/* 项目头文件 */
#include <kinetis/mpu6050.h>
#include <kinetis/iic_soft.h>
```

#### 7.2 函数顺序
1. 结构体定义
2. Regmap 配置
3. 内部辅助函数
4. 设备初始化函数
5. 公开 API 函数
6. 向后兼容包装函数

#### 7.3 分隔符注释
```c
/*********************************************************************
 * MPU6050 Device Structure
 *********************************************************************/

/*********************************************************************
 * Regmap Configuration
 *********************************************************************/

/*********************************************************************
 * Initialization
 *********************************************************************/

/*********************************************************************
 * Public API
 *********************************************************************/
```

### 8. 全局变量管理习惯

**使用全局变量的原则**：
1. 必须使用 `g_` 前缀
2. 尽量使用 `static` 限制作用域
3. 仅用于向后兼容的包装函数
4. 用于设备全局状态（单一设备实例）

**示例**：
```c
static struct mpu6050_device *g_mpu6050_dev;
static mpu6050_config_t g_mpu6050_config = {0};
static u8 g_mpu6050_initialized = 0;
```

### 9. 常见代码模式

#### 9.1 设备初始化模式
```c
struct xxx_device *xxx_init(enum regmap_user_bus_type bus_type, void *bus_master) {
    struct xxx_device *dev;
    
    /* 1. 分配设备 */
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return ERR_PTR(-ENOMEM);
    
    /* 2. 初始化 regmap */
    dev->regmap = regmap_init_user_bus(bus_type, bus_master, &xxx_regmap_config);
    if (IS_ERR(dev->regmap)) {
        kfree(dev);
        return ERR_CAST(dev->regmap);
    }
    
    /* 3. 配置设备 */
    regmap_write(dev->regmap, XXX_REG_CONFIG, default_value);
    
    return dev;
}
```

#### 9.2 读取传感器数据模式
```c
int xxx_read_data(struct xxx_device *dev, u8 *buf, u16 len) {
    u8 val[6];
    int ret;
    
    ret = regmap_bulk_read(dev->regmap, XXX_REG_DATA, val, len);
    if (ret < 0)
        return ret;
    
    /* 转换数据 */
    buf[0] = (val[0] << 8) | val[1];
    // ...
    
    return 0;
}
```

#### 9.3 错误清理模式（goto 清理）
```c
int xxx_function(struct xxx_device *dev) {
    int ret;
    void *buf = NULL;
    
    buf = kmalloc(SIZE, GFP_KERNEL);
    if (!buf) {
        ret = -ENOMEM;
        goto err_out;
    }
    
    ret = regmap_read(dev->regmap, REG, &val);
    if (ret < 0)
        goto err_free;
    
    /* 成功 */
    kfree(buf);
    return 0;

err_free:
    kfree(buf);
err_out:
    return ret;
}
```

### 10. 特定领域的编码习惯

#### 10.1 I2C/SPI 总线驱动
- 由 A03 (sensor Agent) 管理
- 使用 bit-bang 方式实现
- 时序优化：根据 MCU 频率调整延时

#### 10.2 传感器初始化序列
1. 复位设备
2. 延时等待
3. 唤醒设备
4. 配置采样率
5. 配置量程
6. 启动测量

**示例（MPU6050）**：
```c
/* 1. 复位 */
regmap_write(dev->regmap, MPU6050_REG_PWR_MGMT_1, 0x80);
mdelay(100);

/* 2. 唤醒 */
regmap_write(dev->regmap, MPU6050_REG_PWR_MGMT_1, 0x00);

/* 3. 配置采样率 */
regmap_write(dev->regmap, MPU6050_REG_SMPLRT_DIV, 0x07);

/* 4. 配置量程 */
regmap_write(dev->regmap, MPU6050_REG_GYRO_CONFIG, 0x18);
regmap_write(dev->regmap, MPU6050_REG_ACCEL_CONFIG, 0x18);
```

#### 10.3 传感器校准
- MPU6050：6 面静止校准法
- AK8975：硬铁/软铁补偿
- 校准参数存储到文件系统或 EEPROM

### 11. 代码审查检查清单

在提交代码前，检查以下项目：

**命名规范**：
- [ ] 所有函数使用小写 + 下划线命名
- [ ] 全局变量使用 `g_` 前缀
- [ ] 结构体使用 `_device` 后缀
- [ ] 常量和宏使用大写 + 下划线

**架构规范**：
- [ ] 新 API 函数第一个参数是 `struct xxx_device *dev`
- [ ] 使用 regmap API 而非端口函数
- [ ] 设备结构体包含 `struct regmap *regmap` 字段
- [ ] 保留向后兼容包装函数

**数据类型**：
- [ ] 使用 `u8/u16/u32` 而非 `uint8_t/uint16_t/uint32_t`
- [ ] 返回值正确（成功返回 0，失败返回负数）
- [ ] 检查所有分配的返回值

**注释和文档**：
- [ ] 所有公开 API 有 Doxygen 注释
- [ ] 代码内注释简洁，解释"为什么"而非"是什么"
- [ ] 有明显的分隔符注释

**错误处理**：
- [ ] 使用 `pr_info/pr_err/pr_warn` 输出日志
- [ ] 正确使用 `IS_ERR/PTR_ERR/ERR_PTR`
- [ ] 内存分配有错误检查

**代码组织**：
- [ ] 头文件包含顺序正确
- [ ] 函数顺序合理
- [ ] 有清晰的分隔符注释

---

## 📚 相关文档

- **编码风格规则文件**：`.codebuddy/rules/kinetis_coding_style.mdc`
- **Agent 架构设计**：`.codebuddy/agents/ARCHITECTURE.md`
- **各 Agent 配置**：`.codebuddy/agents/*.yaml`

---

## 🔄 更新记录

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-03-08 | v1.0 | 初始版本，基于 mpu6050.c, ds3231.c, max77752.c 分析编码习惯 |

---

**文档结束**
