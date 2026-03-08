# Kinetis Agent 团队使用指南

## 📁 文件结构

```
.codebuddy/agents/
├── ARCHITECTURE.md              # 架构设计文档
├── README.md                    # 本文档（使用指南）
├── coordinator-agent.yaml       # 主协调 Agent
├── A01-kernel-agent.yaml        # 内核核心 Agent
├── A02-base-agent.yaml          # 基础服务 Agent
├── A03-sensor-agent.yaml        # 传感器管理 Agent（含 I2C/SPI）
├── A04-peripheral-agent.yaml    # 外设管理 Agent
├── A05-storage-agent.yaml       # 存储系统 Agent
├── A06-protocol-agent.yaml      # 通信协议 Agent
└── A07-application-agent.yaml   # 应用层 Agent
```

---

## 🎯 Agent 快速参考

| Agent ID | 名称 | 职责 | 关键代码 |
|---------|------|------|---------|
| **A01** | kernel | 内核核心（所有 Agent 共享） | `kernel/*`, `mm/*`, `lib/*` |
| **A02** | base | 基础服务（GPIO、Regmap） | `drivers/base/*`, `drivers/gpio/*` |
| **A03** | sensor | 传感器 + I2C/SPI 总线 | `mpu6050`, `ak8975`, `iic_soft`, `spi_soft` |
| **A04** | peripheral | 外设（LED、按键、开关） | `button`, `switch`, `led` |
| **A05** | storage | 存储系统（Flash、文件系统） | `fatfs-intf`, `w25qxxx`, `at24cxx` |
| **A06** | protocol | 通信协议（蓝牙、NB-IoT、GPS） | `hc-05`, `nb_bc95`, `nema0183` |
| **A07** | application | 应用层（飞行控制、水文） | `flight/*`, `hydrology*`, `tim-task` |

---

## 🚀 快速开始

### 方式 1：直接指定 Agent（精准控制）

```bash
# 语法：@Agent_ID <任务描述>

# 示例：
@A03 优化 MPU6050 的 I2C 读取性能
@A06 添加 MQTT 协议支持到 NB-IoT
@A07 实现飞行模式切换逻辑
@A05 修复 Flash 写入错误
```

**适用场景**：
- ✅ 明确知道要修改哪个模块
- ✅ 性能敏感操作
- ✅ 快速迭代和调试

---

### 方式 2：通过 Coordinator 协调（智能分配）

```bash
# 语法：直接描述任务，让 Coordinator 自动分配

# 示例：
实现飞行数据上传到云端
降低系统整体功耗
添加数据加密功能
优化传感器数据读取延迟
```

**适用场景**：
- ✅ 不清楚涉及哪些 Agent
- ✅ 跨多个模块的功能
- ✅ 系统级优化

---

### 方式 3：Agent 自主协作（自动调用）

```bash
# 语法：@Application_Agent <高层任务>

# 示例：
@A07 实现飞行数据记录功能
@A07 实现水文数据自动上报
@A07 实现 LED 状态指示
```

**适用场景**：
- ✅ 应用层任务
- ✅ 需要多个底层 Agent 协作
- ✅ Agent 之间已有协作经验

---

## 📋 典型使用场景

### 场景 1：优化传感器读取

```bash
@A03 优化 MPU6050 的 I2C 读取性能
```

**预期流程**：
1. A03 查询记忆（I2C 时序优化经验）
2. 分析当前代码
3. 实现优化（burst read、减少 I2C start/stop）
4. 测试验证
5. 更新记忆

---

### 场景 2：添加新传感器

```bash
@A03 添加 BMP280 气压传感器驱动
```

**预期流程**：
1. A03 检索记忆（类似 MAX30205 的经验）
2. 应用现有模式（I2C 初始化、校准）
3. 实现 BMP280 驱动
4. 更新记忆（BMP280 特殊配置）

---

### 场景 3：实现新功能（跨 Agent）

```bash
实现飞行数据上传到云端
```

**预期流程**：
1. Coordinator 分解任务
2. A07 收集飞行数据
3. A03 读取传感器
4. A06 通过 NB-IoT 发送
5. A05 本地存储备份
6. A04 LED 指示状态

---

### 场景 4：修复 Bug

```bash
@A06 修复 NB-IoT 发送数据时的内存泄漏
```

**预期流程**：
1. A06 查询记忆（内存泄漏修复经验）
2. 分析代码（nb_send_at() 未释放缓冲区）
3. 修复（添加 kfree()）
4. 更新记忆（强化规则）

---

## 💡 最佳实践

### 1. 何时使用直接指定 Agent

```bash
✅ 推荐场景：
- 明确知道要修改哪个模块
- 性能敏感操作
- 快速迭代和调试
- 单一功能点修改

示例：
  @A03 优化 I2C 时序
  @A05 添加文件加密功能
  @A06 修复串口死锁
```

### 2. 何时使用 Coordinator

```bash
✅ 推荐场景：
- 不清楚涉及哪些 Agent
- 跨多个模块的功能
- 系统级优化
- 探索性任务

示例：
  实现数据备份功能
  降低系统延迟
  如何提高通信可靠性
```

---

## 📊 Agent 依赖关系

```
A07 (application)
├── A03 (sensor)
│   └── A02 (base)
│       └── A01 (kernel)
├── A05 (storage)
│   └── A03 (sensor) → A02 (base) → A01 (kernel)
├── A06 (protocol)
│   └── A02 (base) → A01 (kernel)
└── A04 (peripheral)
    └── A02 (base) → A01 (kernel)
```

---

## 🔄 Agent 协作示例

### 飞行控制姿态解算

```
用户: "解算飞行姿态"
  │
  ▼
Coordinator
  │
  ├─→ A07 (application): 飞控逻辑
  │       │
  │       ├─→ A03 (sensor): 读取 MPU6050 + AK8975
  │       │       │
  │       │       └─→ A02 (base): 使用 Regmap
  │       │
  │       └─→ A05 (storage): 读取 PID 参数
  │
  └─→ A01 (kernel): 提供时间戳
```

### 水文数据上报

```
用户: "上报水文数据"
  │
  ▼
Coordinator
  │
  ├─→ A07 (application): 水文数据处理
  │       │
  │       ├─→ A05 (storage): 读取历史数据
  │       │
  │       └─→ A06 (protocol): 通过 NB-IoT 上报
  │               │
  │               └─→ A01 (kernel): 串口驱动
  │
  └─→ A04 (peripheral): LED 指示
```

---

## 🗂️ 代码文件映射

| 功能域 | 代码文件 | 归属 Agent |
|--------|---------|-----------|
| I2C 驱动 | `drivers/kinetis/iic_soft.c/h` | A03 (sensor) |
| SPI 驱动 | `drivers/kinetis/spi_soft.c/h` | A03 (sensor) |
| MPU6050 | `drivers/kinetis/mpu6050.c/h` | A03 (sensor) |
| AK8975 | `drivers/kinetis/ak8975.c/h` | A03 (sensor) |
| 按键 | `drivers/kinetis/button.c/h` | A04 (peripheral) |
| LED | `drivers/kinetis/led.c/h` | A04 (peripheral) |
| Flash | `drivers/kinetis/w25qxxx.c/h` | A05 (storage) |
| FATFS | `fs/fatfs/*` | A05 (storage) |
| 串口 | `drivers/kinetis/serial-port.c/h` | A06 (protocol) |
| NB-IoT | `drivers/kinetis/nb_*.c/h` | A06 (protocol) |
| 飞行控制 | `drivers/kinetis/flight/*` | A07 (application) |
| 水文监测 | `drivers/kinetis/hydrology*` | A07 (application) |
| 内核核心 | `kernel/*`, `mm/*`, `lib/*` | A01 (kernel) |
| GPIO | `drivers/gpio/*` | A02 (base) |
| Regmap | `drivers/base/regmap/*` | A02 (base) |

---

## 📝 常见问题

### Q1: I2C 和 SPI 为什么在 sensor Agent？

**A**: 因为项目中 80% 的 I2C/SPI 使用者是传感器。将总线驱动与传感器驱动放在同一个 Agent，便于整体优化时序和性能，减少跨 Agent 调用。详见 `ARCHITECTURE.md`。

### Q2: 如何添加新 Agent？

**A**: 参考 `ARCHITECTURE.md` 的扩展性章节，创建新的 YAML 配置文件，并在 coordinator-agent.yaml 中注册。

### Q3: Agent 会自主学习吗？

**A**: 会。每个 Agent 都有记忆系统，记录问题-解决方案对、用户偏好、最佳实践等。随着交互次数增加，Agent 会不断进化。

### Q4: 如何查看 Agent 的记忆？

**A**: Agent 的记忆通过 `update_memory` 工具持久化存储，可以通过查看记忆记录了解 Agent 学到了什么。

### Q5: Agent 之间如何协作？

**A**: Agent 之间通过 Coordinator 协调，或者通过预先定义的协作关系自动调用。每个 Agent 的配置文件中都定义了依赖关系和可调用的其他 Agent。

---

## 🔧 进阶使用

### 自定义 Agent 配置

如果需要修改 Agent 的配置，编辑对应的 YAML 文件：

```yaml
# A03-sensor-agent.yaml
agent:
  id: "A03"
  name: "sensor Agent"
  
  # 添加新的服务
  services:
    - name: "new_sensor"
      description: "新传感器操作"
  
  # 添加新的知识
  domain_knowledge:
    - "新传感器的特性"
```

### 创建新 Agent

1. 创建 YAML 配置文件
2. 定义职责、代码文件、依赖关系
3. 在 coordinator-agent.yaml 中注册
4. 测试验证

---

## 📚 相关文档

- `ARCHITECTURE.md` - 架构设计详细文档
- `coordinator-agent.yaml` - 主协调 Agent 配置
- `A01-A07-*-agent.yaml` - 各个 Agent 的详细配置

---

## 📞 支持

如有问题或建议，请直接提出，系统会持续优化和进化。

---

**版本**: v1.0
**创建日期**: 2026-03-06
**最后更新**: 2026-03-06
