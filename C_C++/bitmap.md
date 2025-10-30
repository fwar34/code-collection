### 不使用 `ctzll` 等库函数获取 uint64_t 二进制中 1 的位置（索引从 0 开始）

#### 核心思路
当无法使用编译器内置函数（如 `__builtin_ctzll`）时，可通过 **De Bruijn 序列法** 或 **位操作二分查找法** 高效获取最低位 1 的位置。两种方法均满足：
- **循环次数 = 设置位数量**（非 64 次）
- **无编译器依赖**
- **常数时间定位最低位 1**

---

#### 方法 1：De Bruijn 序列法（推荐）
**原理**：利用 De Bruijn 序列的数学特性，通过 **1 次乘法 + 1 次移位 + 1 次查表** 定位最低位 1。

##### 实现代码
```cpp
#include <vector>
#include <cstdint>

std::vector<int> getSetBitPositions(uint64_t value) {
    // 预定义 De Bruijn 序列映射表（64 位标准序列）
    static const int debruijn64[64] = {
        0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    const uint64_t debruijn_const = 0x03F79D71B4CB0A89ULL; // 64 位 De Bruijn 常量

    std::vector<int> positions;
    while (value != 0) {
        // 1. 提取最低位 1: value & -value
        uint64_t lowest_bit = value & -static_cast<int64_t>(value);
        // 2. De Bruijn 计算位置
        int pos = debruijn64[(lowest_bit * debruijn_const) >> 58];
        positions.push_back(pos);
        // 3. 清除最低位 1
        value &= value - 1;
    }
    return positions;
}
```

##### 优势
- **极致高效**：每次循环仅 3 次操作（`&`, `*`, `>>` + 查表）
- **无分支预测失败**：纯位运算，CPU 流水线友好
- **循环次数 = 1 的数量**（如 `0b11010` 仅循环 3 次）

##### 示例
- **输入**：`value = 0b11010`（十进制 26）
- **执行**：
  1. `lowest_bit = 0b10` → `(2 * debruijn_const) >> 58 = 1` → `pos = debruijn64[1] = 1`
  2. `value = 0b11000` → `lowest_bit = 0b1000` → `pos = 3`
  3. `value = 0b10000` → `lowest_bit = 0b10000` → `pos = 4`
- **输出**：`[1, 3, 4]`

---

#### 方法 2：位操作二分查找法
**原理**：通过 **6 次位掩码检查** 逐步缩小范围，定位最低位 1。

##### 实现代码
```cpp
#include <vector>
#include <cstdint>

std::vector<int> getSetBitPositions(uint64_t value) {
    std::vector<int> positions;
    while (value != 0) {
        int pos = 0;
        uint64_t x = value & -value; // 提取最低位 1

        // 二分查找最低位 1 的位置
        if ((x & 0xFFFFFFFF) == 0) { pos += 32; x >>= 32; }
        if ((x & 0xFFFF) == 0) { pos += 16; x >>= 16; }
        if ((x & 0xFF) == 0) { pos += 8; x >>= 8; }
        if ((x & 0xF) == 0) { pos += 4; x >>= 4; }
        if ((x & 0x3) == 0) { pos += 2; x >>= 2; }
        if ((x & 0x1) == 0) { pos += 1; }

        positions.push_back(pos);
        value &= value - 1; // 清除最低位 1
    }
    return positions;
}
```

##### 优势
- **零数据依赖**：无需预定义表，纯位操作
- **确定性性能**：每次循环固定 6 次检查（优于逐位扫描）
- **嵌入式友好**：无乘法/移位开销，适合资源受限环境

##### 示例
- **输入**：`value = 0b11010`
- **定位最低位 `0b10`**：
  1. `x = 0b10` → `0xFFFFFFFF` 掩码：`x & 0xFFFFFFFF != 0` → 不加 32
  2. `0xFFFF` 掩码：`x & 0xFFFF != 0` → 不加 16
  3. `0xFF` 掩码：`x & 0xFF != 0` → 不加 8
  4. `0xF` 掩码：`x & 0xF != 0` → 不加 4
  5. `0x3` 掩码：`x & 0x3 = 0b10 != 0` → 不加 2
  6. `0x1` 掩码：`x & 0x1 = 0` → `pos += 1` → **位置 = 1**

---

### 方法对比
| **特性**               | **De Bruijn 法**                     | **二分查找法**               |
|------------------------|--------------------------------------|-----------------------------|
| **循环内操作次数**     | 3 次（乘法+移位+查表）               | 6 次（位掩码检查）          |
| **额外存储**           | 64 字节查表                          | 无                          |
| **适用场景**           | 通用环境（x86/ARM 服务器/PC）        | 嵌入式/无乘法单元设备       |
| **代码可读性**         | 中等（需理解 De Bruijn 原理）        | 高（逻辑直观）              |
| **性能**               | ⭐⭐⭐⭐ (更快)                         | ⭐⭐⭐ (稍慢但稳定)           |

> **推荐选择**：  
> - 在您的 [bitmap.cpp](file://d:\gcc-projects\code-collection\C_C++\bitmap.cpp) 上下文中（`gcc-projects`），**De Bruijn 法更优**（x86 架构下乘法指令高效，且循环次数极少）。  
> - 若目标平台无 64 位乘法支持（如某些 MCU），改用二分查找法。