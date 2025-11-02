#include <iostream>
#include <cstdint>
#include <optional>
#include <vector>
#include <cstring>
#include <functional>
#include <format>

// 定义订阅者最大数量
#define SUBSCRIBER_MAX_NUM 60 
// 定义每个主题的位图大小
#define TOPIC_BITMAP_SIZE 64
// 定义POWER_SIZE，用于位图索引计算
#define POWER_SIZE 1

// 错误码枚举
enum Error
{
    ERROR_OK = 0,        // 操作成功
    ERROR_NO_SPACE = 1,  // 空间不足
};

// 主题枚举定义
enum Topic
{
    TOPIC_REMOTE = 0,    // 远程控制主题
    TOPIC_POWER_ON,      // 开机主题
    TOPIC_POWER_OFF,     // 关机主题
    TOPIC_SCREEN_ON,     // 屏幕开启主题
    TOPIC_SCREEN_OFF,    // 屏幕关闭主题
    TOPIC_22 = 22,       // 第22号主题
    TOPIC_23 = 23,       // 第23号主题
    // ...
};

// 主题字符串数组，用于调试输出
static const char *topicStr[] = {
    "TOPIC_REMOTE",
    "TOPIC_POWER_ON",
    "TOPIC_POWER_OFF",
    "TOPIC_SCREEN_ON",
    "TOPIC_SCREEN_OFF"
};

// 定义主题通知回调函数指针类型
using TopicNotify = void(*)(const void *);

// 订阅者信息结构体
struct SubscriberInfo {
    uint32_t id;          // 订阅者ID
    const void *arg;      // 传递给回调函数的参数
    TopicNotify notify;   // 通知回调函数指针
};

// 订阅者表结构体（使用柔性数组）
struct SubScriberTable
{
    uint32_t capacity;      // 订阅者表容量
    uint32_t curSize;       // 当前订阅者数量
    SubscriberInfo infos[0]; // 柔性数组，存储订阅者信息
};

// 位图表结构体（使用柔性数组）
struct BitTable
{
    uint16_t xxxxx : 13;    // 保留字段（13位）
    uint16_t yyyyy : 3;     // 保留字段（3位）
    uint32_t capacity;      // 位图容量
    uint32_t bitmaps[0];    // 柔性数组，存储位图数据
};

// 发布订阅管理器类
class PubSubManager
{
public:
    // 初始化管理器
    bool Init(uint8_t *buffer, size_t bufferLen);
    
    // 订阅主题
    bool Subscribe(uint32_t topic, const SubscriberInfo &info);
    
    // 取消订阅主题
    void UnSubscribe(uint32_t topic, const SubscriberInfo &info);
    
    // 发布主题消息
    void Publish(uint32_t topic);

private:
    // 获取订阅者信息索引
    std::optional<uint32_t> GetSubScriberInfoIndex(const SubscriberInfo &info);
    
    // 注销订阅者注册
    void UnSubScriberRegister(uint32_t topic, const SubscriberInfo &info);
    
    // 检查主题是否有注册
    bool IsTopicHasRegister(uint32_t topic);
    
    // 获取位值
    uint32_t GetBit(uint32_t topic, uint32_t subscriberInfoIndex);
    
    // 设置位值
    void SetBit(uint32_t topic, uint32_t subscriberInfoIndex);
    
    // 清除位值
    void ClrBit(uint32_t topic, uint32_t subscriberInfoIndex);
    
    // 获取订阅者信息
    SubscriberInfo *GetSubScriberInfo(uint32_t index);

private:
    SubScriberTable *subScriberTable_; // 订阅者表指针
    BitTable *bitTable_;               // 位图表指针
};

// 初始化发布订阅管理器
bool PubSubManager::Init(uint8_t *buffer, size_t bufferLen)
{
    // 检查缓冲区指针是否为空
    if (buffer == nullptr) {
        return false;
    }
    
    // 清空缓冲区
    memset(buffer, 0, bufferLen);
    
    // 初始化订阅者表
    subScriberTable_ = (SubScriberTable *)buffer;
    subScriberTable_->capacity = SUBSCRIBER_MAX_NUM;
    subScriberTable_->curSize = 0;
    
    // 初始化位图表（位于订阅者表之后）
    bitTable_ = (BitTable *)(buffer + sizeof(SubScriberTable) + sizeof(SubscriberInfo) * SUBSCRIBER_MAX_NUM);
    bitTable_->capacity = (bufferLen - sizeof(SubScriberTable) - 
        sizeof(SubscriberInfo) * SUBSCRIBER_MAX_NUM - sizeof(BitTable)) / 4;

    return true;
}

// 订阅指定主题
bool PubSubManager::Subscribe(uint32_t topic, const SubscriberInfo &info)
{
    // 获取订阅者信息索引
    std::optional<uint32_t> index = GetSubScriberInfoIndex(info);
    if (!index.has_value()) {
        return false;
    }

    // 计算位图起始索引（topic * 2^POWER_SIZE）
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    
    // 计算位图中的元素索引（index / 32）
    uint32_t bitmapIndex = index.value() >> 5;
    
    // 计算位偏移（1 << (index % 32)）
    uint8_t bitmapOffset = 1 << index.value();
    
    // 在位图中设置对应位
    bitTable_->bitmaps[bitmapBeginIndex + bitmapIndex] |= bitmapOffset;
    return true;
}

// 取消订阅指定主题
void PubSubManager::UnSubscribe(uint32_t topic, const SubscriberInfo &info)
{
    // 检查主题是否有注册
    if (!IsTopicHasRegister(topic)) {
        return;
    }

    std::vector<uint32_t> subscriberInfoIndex;
    
    // 计算位图起始索引
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    
    // 计算下一个主题的位图起始索引
    uint32_t nextBitmapBeginIndex = (topic + 1) << POWER_SIZE;
    
    // 遍历位图
    for (uint32_t i = 0; i < nextBitmapBeginIndex; i++) {
        uint32_t value = bitTable_->bitmaps[bitmapBeginIndex];
        // TODO: 这里代码未完成
    }
}

/**
 * @brief 汉明重量算法计算订阅者的索引
 * 也可以使用编译器提供的函数快速计算最低位的位置：uint32_t tmpIndex = __builtin_ctz(bitmap);
 * @param topic 
 */
void PubSubManager::Publish(uint32_t topic)
{
    // 计算位图起始索引
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    
    // 遍历位图中的每个元素
    for (uint32_t i = 0; i < POWER_SIZE; i++) {
        // 获取当前位图元素
        uint32_t bitmap = bitTable_->bitmaps[bitmapBeginIndex + i];
        
        // 使用Brian Kernighan算法遍历所有设置的位
        while (bitmap != 0) {
            // 将最低位右侧的所有bit位置1，其他位置置0，结果中1的个数即原值中最低位1的位置
            uint32_t tmpIndex = (bitmap - 1) & ~bitmap;
            // 计算最低位的位置（汉明重量算法的一部分）
            tmpIndex = (tmpIndex & 0x55555555) + ((tmpIndex >> 1) & 0x55555555);
            tmpIndex = (tmpIndex & 0x33333333) + ((tmpIndex >> 2) & 0x33333333);
            tmpIndex = (tmpIndex & 0x0F0F0F0F) + ((tmpIndex >> 4) & 0x0F0F0F0F);
            tmpIndex = (tmpIndex * 0x01010101) >> 24;
            tmpIndex += i * 32; // 加上偏移量

            // 获取订阅者信息并调用通知回调
            if (SubscriberInfo *info = GetSubScriberInfo(tmpIndex); info != nullptr) {
                if (info->notify != nullptr) {
                    info->notify(info->arg);
                }
            }

            // 清除最低位的1，继续处理下一个设置位
            bitmap &= (bitmap - 1);
        }
    }
}

// 获取订阅者信息索引
std::optional<uint32_t> PubSubManager::GetSubScriberInfoIndex(const SubscriberInfo &info)
{
    uint32_t i = 0;
    
    // 查找是否已存在相同的订阅者信息
    for (i = 0; i < subScriberTable_->curSize; i++) {
        // 检查ID是否匹配
        if (subScriberTable_->infos[i].id != info.id) {
            continue;
        }

        // 检查通知回调是否匹配
        if ((subScriberTable_->infos[i]).notify != info.notify) {
            continue;
        }

        // 找到匹配项，返回索引
        return i;
    }

    // 如果没找到且还有空间，则添加新订阅者
    if (i < subScriberTable_->capacity) {
        subScriberTable_->infos[i] = info;
        subScriberTable_->curSize++;
        return i;
    }

    // 没有空间，返回空
    return std::nullopt;
}

// 注销订阅者注册
void PubSubManager::UnSubScriberRegister(uint32_t topic, const SubscriberInfo &info)
{
    // 获取订阅者信息索引
    std::optional<uint32_t> index = GetSubScriberInfoIndex(info);
    if (!index.has_value()) {
        return;
    }

    // 清除位图中对应的位
    ClrBit(topic, index.value());
}

// 检查主题是否有注册
bool PubSubManager::IsTopicHasRegister(uint32_t topic)
{
    // 计算位图起始索引
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    
    // 计算下一个主题的位图起始索引
    uint32_t nextTopicBeginIndex = (topic + 1) << POWER_SIZE;
    
    uint64_t total = 0;
    
    // 统计位图中所有位的总和
    for (uint32_t i = bitmapBeginIndex; i < nextTopicBeginIndex; i++) {
        total += bitTable_->bitmaps[i]; 
    }

    // 如果总和为0，表示没有注册
    return total != 0;
}

// 获取位值
uint32_t PubSubManager::GetBit(uint32_t topic, uint32_t subscriberInfoIndex)
{
    // 计算位图起始索引
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    
    // 计算位图元素索引
    uint32_t bitmapIndex = subscriberInfoIndex >> 5;
    
    // 计算位偏移
    uint32_t bitmapOffset = subscriberInfoIndex & 31;
    
    // 返回对应位的值
    return bitTable_->bitmaps[bitmapBeginIndex + bitmapIndex] & (1 << bitmapOffset);
}

// 设置位值
void PubSubManager::SetBit(uint32_t topic, uint32_t subscriberInfoIndex)
{
    uint32_t bitmapBeginIndex = topic << POWER_SIZE; // 计算topic起始索引，POWER_SIZE=1为topic*2即一个topic占用2个uint32_t（64bit）
    uint32_t bitmapIndex = subscriberInfoIndex >> 5; // index.value()/32，计算从bitmapBeginIndex开始第几个uint32_t偏移
    /**
     * 当取模运算的除数是 2 的整数次幂 时，可以通过位运算的 与（&）操作 来优化，
     * 因为此时取模结果等价于该数二进制表示的低 n 位（n 为 2 的幂次的指数）。
     * 对于 45 % 32：
     * 32 是 2 的 5 次方，符合 “除数为 2 的整数次幂” 的条件。
     * 此时取模运算等价于 数值 & (2ⁿ - 1)，其中 n=5，因此 
     * 2^5 - 1 = 31 。
     * 因此，45 % 32 可优化为位运算：int a = 45 & 31;
     */
    uint8_t bitmapOffset = subscriberInfoIndex & 31; // x % n => x & (n - 1)，计算半字(32bit)内bit偏移
    
    // 设置对应位为1
    bitTable_->bitmaps[bitmapBeginIndex + bitmapIndex] |= (1 << bitmapOffset);
}

// 清除位值
void PubSubManager::ClrBit(uint32_t topic, uint32_t subscriberInfoIndex)
{
    // 计算位图起始索引
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    
    // 计算位图元素索引
    uint32_t bitmapIndex = subscriberInfoIndex >> 5;
    
    // 计算位偏移
    uint32_t bitmapOffset = subscriberInfoIndex & (32 - 1);
    
    // 清除对应位（设置为0）
    bitTable_->bitmaps[bitmapBeginIndex + bitmapIndex] &= ~(1 << bitmapOffset);
}

// 获取订阅者信息
SubscriberInfo *PubSubManager::GetSubScriberInfo(uint32_t index)
{
    // 检查索引是否超出范围
    if (index >= SUBSCRIBER_MAX_NUM) {
        return nullptr;
    }

    // 返回对应索引的订阅者信息
    return &subScriberTable_->infos[index];
}

// 主函数
int main()
{
    constexpr size_t BUFFER_LEN = 4096;
    uint8_t buffer[BUFFER_LEN] = {0};
    PubSubManager pbManager;
    pbManager.Init(buffer, sizeof(buffer));

    constexpr uint32_t id1 = 1;
    constexpr uint32_t id2 = 2;
    constexpr uint32_t id22 = 22;
    constexpr uint32_t id23 = 23;

    // 创建针对TOPIC_22的订阅者
    auto topicNotify22 = [](const void *arg) {
        std::cout << std::format("receive pub {}\n", (uint32_t)TOPIC_22);
    };
    SubscriberInfo info22 = {id22, "info22", topicNotify22};
    pbManager.Subscribe(TOPIC_22, info22);

    // 创建针对TOPIC_23的订阅者
    auto topicNotify23 = [](const void *arg) {
        std::cout << std::format("receive pub {}\n", (uint32_t)TOPIC_23);
    };
    SubscriberInfo info23 = {id23, "info23", topicNotify23};
    pbManager.Subscribe(TOPIC_23, info23);

    // 创建针对TOPIC_POWER_ON的订阅者
    auto topicNotify1 = [](const void *arg) {
        std::cout << std::format("receive pub {}\n", topicStr[TOPIC_POWER_ON]);
    };
    SubscriberInfo info1 = {id1, "info1", topicNotify1};
    pbManager.Subscribe(TOPIC_POWER_ON, info1);

    // 创建针对TOPIC_POWER_OFF的订阅者
    auto topicNotify2 = [](const void *arg) {
        std::cout << std::format("receive pub {}\n", topicStr[TOPIC_POWER_OFF]);
    };
    SubscriberInfo info2 = {id2, "info2", topicNotify2};
    pbManager.Subscribe(TOPIC_POWER_OFF, info2);

    // 发布消息
    pbManager.Publish(TOPIC_POWER_ON);
    pbManager.Publish(TOPIC_POWER_OFF);

    return 0;
}