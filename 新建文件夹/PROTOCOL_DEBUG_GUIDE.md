# 通讯协议实现调试指南

## 📋 问题分析

你遇到的"第一次通信成功，第二次失败"问题的根本原因是：

### ❌ 原始代码的问题

1. **`Protocol_ParseFrame()` 函数未实现** → 直接调用会返回undefined
2. **接收缓冲区管理不当** → 第一帧后残留数据污染第二帧
3. **没有完整的状态机** → 帧同步丢失，CRC校验失败
4. **ASCII转十六进制逻辑缺失** → 无法正确解析接收到的数据

---

## ✅ 新方案的改进

### 1. **完整的状态机解析（Protocol.c）**
```c
typedef enum {
    PARSE_STATE_IDLE,           // 等待帧头
    PARSE_STATE_HEADER_BYTE2,   // 解析帧头第二字节
    PARSE_STATE_DEVICE_ID_BYTE1,// 解析设备ID
    ...
} ParseState_t;
```

**特点：**
- ✅ 逐字节严格解析
- ✅ 每步都有错误检查
- ✅ 帧完整后自动验证CRC
- ✅ 失败时正确重置状态，不会卡死

### 2. **ASCII字符串正确处理**
```c
/* 将接收到的ASCII字符 "A5B6" 转为 0xA5, 0xB6 */
static uint8_t Protocol_Ascii2BytesToByte(const char *ascii)
{
    uint8_t high = Protocol_AsciiToHex(ascii[0]);  // 'A' -> 10
    uint8_t low = Protocol_AsciiToHex(ascii[1]);   // '5' -> 5
    return (high << 4) | low;  // 结果: 0xA5
}
```

### 3. **发送端也转为ASCII格式**
```c
/* 将二进制帧转为ASCII字符串发送 */
static void Protocol_SendFrameASCII(uint8_t *frame_bin, uint16_t frame_len)
{
    char ascii_buf[512];
    for (i = 0; i < frame_len; i++) {
        sprintf(&ascii_buf[i*2], "%02X", frame_bin[i]);
    }
    RS485_SendData((uint8_t*)ascii_buf, frame_len * 2);
}
```

### 4. **环形缓冲区管理**
```c
/* 接收时自动存入环形缓冲区 */
Usart1Ring_PutChar(ch);  // 中断中调用

/* 解析后清理已处理数据 */
Usart1Ring_Skip(ascii_count);  // 清理缓冲区
```

---

## 🔧 集成步骤

### 步骤1：更新项目文件

**替换以下文件到你的工程：**

1. **Protocol.h** 和 **Protocol.c** → 新的协议层实现
2. **Function_v2.c** → 改进的主循环（可重命名为Function.c）

### 步骤2：更新Function.h

在 `Function.h` 中添加：
```c
#include "Protocol.h"

/* 如果还没有这些结构定义，需要从你的项目中查找 */
typedef struct {
    uint8_t updateFlag;
    uint8_t updateStatus;
    uint32_t appSize;
} BootParam_t;

typedef struct {
    /* 定义你项目中的参数结构 */
} UpdateLog_t;

typedef struct {
    /* 定义你项目中的配置结构 */
} UserConfig_t;

typedef struct {
    /* 定义你项目中的校准数据结构 */
} CalibData_t;

#define APP_START_ADDR  0x08011000
#define BOOT_CONFIG_ADDR 0x08010000
#define BOOT_PARAM_SIZE 1024
#define UPDATE_REQUEST_FLAG    0x01
#define UPDATE_STATUS_WAIT_BOOTLOADER 0x02
```

### 步骤3：编译检查

```bash
# 在你的IDE中编译，确保没有错误
# 如果有undefined reference，确保：
# 1. 所有源文件都被编译
# 2. Protocol.c 被添加到工程
# 3. USART1.c 中的环形缓冲函数已实现
```

---

## 🧪 测试方法

### 测试1：单次通信验证

**上位机发送：** 查询固件版本
```
A5B60001010104000214BBB6A5
```

**期望单片机回复：**
```
A5B60001020104040200010004972 1B6A5
```

### 测试2：连续通信验证

**上位机发送3次相同命令，验证每次都有正确应答**

```c
/* 上位机伪代码 */
for (int i = 0; i < 3; i++) {
    send("A5B60001010104000214BBB6A5");
    wait(100ms);
    receive();  // 应该每次都收到应答
}
```

### 测试3：通过串口调试助手验证

**设置：**
- 波特率：19200
- 数据位：8
- 停止位：1
- 校验：None
- 显示格式：**HEX字符显示**（重要！）

**发送测试帧：**
```
A5B60001010104000214BBB6A5
```

### 测试4：查看调试信息

在 `Protocol.c` 中添加调试打印（可选）：

```c
#ifdef DEBUG_PROTOCOL
    printf("Parse state: %d\r\n", g_parse_state);
    printf("Frame CRC: 0x%04X, Calc: 0x%04X\r\n", crc_calc, g_frame_buffer.crc16);
#endif
```

---

## 📊 故障排查

| 症状 | 原因 | 解决方案 |
|------|------|--------|
| 第一次成功，第二次无反应 | 接收缓冲区未清理 | 确保 `Protocol_ParseFrame()` 中调用 `Usart1Ring_Skip()` |
| 收到错误应答 `A5B60001FFEEEE...` | CRC校验失败 | 检查 `Protocol_CalcCRC16()` 实现 |
| 收不到任何应答 | 帧未成功解析 | 打开DEBUG，观察解析过程 |
| 波特率错误导致乱码 | UART初始化问题 | 检查 `RS485_Init()` 中的波特率设置是否为19200 |
| ASCII转换错误 | `Protocol_Ascii2BytesToByte()` 有bug | 验证高低字节顺序 |

---

## 💡 关键设计要点

### 1. **帧头同步机制**
```c
/* 在空闲状态下查找"A5B6"（ASCII: A5B6） */
while (i + 3 < count) {
    if (peek_buf[i] == 'A' && peek_buf[i+1] == '5' &&
        peek_buf[i+2] == 'B' && peek_buf[i+3] == '6') {
        break;  // 找到帧头
    }
    i++;
}
```

### 2. **完整帧检测**
```c
/* 必须先找到帧尾"B6A5"才能认为帧完整 */
for (i = 0; i + 3 < count; i += 2) {
    if (peek_buf[i] == 'B' && peek_buf[i+1] == '6' &&
        peek_buf[i+2] == 'A' && peek_buf[i+3] == '5') {
        frame_end = i + 4;
        break;
    }
}
if (frame_end == 0) return 0;  // 帧未完整，继续等待
```

### 3. **错误恢复**
```c
/* 解析失败时跳过一对ASCII字符（代表一个字节）并重试 */
Usart1Ring_Skip(2);
g_parse_state = PARSE_STATE_IDLE;
```

---

## 📝 代码集成清单

- [ ] 复制 `Protocol.h` 到项目
- [ ] 复制 `Protocol.c` 到项目  
- [ ] 更新 `Function.h`，添加必要定义
- [ ] 将 `Function_v2.c` 的内容合并到 `Function.c`，或直接替换
- [ ] 在项目配置中添加 `Protocol.c` 编译
- [ ] 编译验证无错误
- [ ] 烧录测试

---

## 🎯 预期结果

**修复后：**
- ✅ 第一次通信成功
- ✅ 第二次通信也成功
- ✅ 可以连续发送多条命令
- ✅ 错误帧会正确应答错误
- ✅ 帧头丢失时能正确同步

---

## 📞 常见问题

**Q: 为什么还要转成ASCII字符串发送？**
A: 根据赛题要求 2.2 节"所有协议帧按十六进制结构组帧后，以**组帧后的ASCII字符串形式**在串口收发"

**Q: CRC-16-Modbus与其他CRC有什么区别？**
A: 
- 初始值：0xFFFF
- 多项式：0xA001（反向）
- 最终异或：0（无）
- 反向输入/输出

**Q: 如果多次通信仍然失败，怎么办？**
A: 
1. 用逻辑分析仪抓取实际的UART波形
2. 检查RS485收发切换时序是否正确
3. 验证是否有半双工冲突

---

## 📚 文件对照表

| 原文件 | 新文件 | 说明 |
|--------|--------|------|
| - | Protocol.h | 新增：协议头文件 |
| - | Protocol.c | 新增：协议实现 |
| Function.c | Function_v2.c | 改进版本 |
| USART1.c | 无改动 | 保持不变 |
| USART1.h | 无改动 | 保持不变 |

---

## ⚡ 快速开始

**最快集成方法（3步）：**

1. 复制 `Protocol.h` 和 `Protocol.c` 到项目
2. 在 `Function.c` 开头添加 `#include "Protocol.h"`
3. 将 `Function_v2.c` 的 `UsrFunction()` 替换原来的版本

Done! 编译→烧录→测试

---

