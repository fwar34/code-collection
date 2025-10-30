#include <iostream>
#include <cstdint>
#include <optional>
#include <vector>

#define SUBSCRIBER_MAX_NUM 64
#define TOPIC_BITMAP_SIZE 64
#define POWER_SIZE 1

enum Error
{
    ERROR_OK = 0,
    ERROR_NO_SPACE = 1,
};

enum Topic
{
    TOPIC_REMOTE = 0,
    TOPIC_POWER_ON,
    TOPIC_POWER_OFF,
    TOPIC_SCREEN_ON,
    TOPIC_SCREEN_OFF,
    // ...
};

using NotifyCallback = void(void *arg);
// pub-sub 使用位图
struct SubscriberInfo {
    uint32_t id;
    void *arg;
    NotifyCallback notify;
};

struct SubScriberTable
{
    uint32_t capacity;
    uint32_t curSize;
    SubscriberInfo infos[0];
};

struct BitTable
{
uint16_t xxxxx: 13;
uint16_t yyyyy: 3;
uint32_t capacity;
uint32_t bitmaps[0];
};

class PubSubManager
{
public:
    bool Init(uint8_t *buffer, size_t bufferLen);
    bool Subscribe(uint32_t topic, const SubscriberInfo &info);
    void UnSubscribe(uint32_t topic, const SubscriberInfo &info);
    void Publish(uint32_t topic);

private:
    std::optional<uint32_t> SubscriberRegister(const SubscriberInfo &info);
    void UnSubScriberRegister(uint32_t topic);
    bool IsTopicHasRegister(uint32_t topic);
    void GetBit(uint32_t topic, uint32_t subscriberInfoIndex);
    void SetBit(uint32_t topic, uint32_t subscriberInfoIndex);
    void ClrBit(uint32_t topic, uint32_t subscriberInfoIndex);
    void CollectSubscriberInfoIndex(std::vector &subScriberInfoIndexVec);
    SubscriberInfo *GetSubScriberInfo(uint32_t index);

private:
    SubScriberTable *subScriberTable_;
    BitTable *bitTable_;
};

bool PubSubManager::Init(uint8_t *buffer, size_t bufferLen)
{
    memset(buffer, 0, bufferLen);
    subScriberTable_ = (SubScriberTable *)buffer;
    subScriberTable_->capacity = SUBSCRIBER_MAX_NUM;
    subScriberTable_->curSize = 0;
    bitTable_ = (BitTable *)(buffer + sizeof(SubScriberTable) + sizeof(SubscriberInfo) * SUBSCRIBER_MAX_NUM);
    bitTable_->capacity = (bufferLen - sizeof(SubScriberTable) - 
        sizeof(SubscriberInfo) * SUBSCRIBER_MAX_NUM - sizeof(BitTable)) / (TOPIC_BITMAP_SIZE >> POWER_SIZE);
}

bool PubSubManager::Subscribe(uint32_t topic, const SubscriberInfo &info)
{
    std::optional<uint32_t> index = SubscriberRegister(info);
    if (!index.has_value()) {
        return false;
    }

    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    uint32_t bitmapIndex = index.value() >> 5;
    uint8_t bitmapOffset = 1 << index.value();
    subScriberTable_->infos[bitmapBeginIndex + bitmapIndex] |= bitmapOffset;
    return true;
}

void PubSubManager::UnSubscribe(uint32_t topic, const SubscriberInfo &info)
{
    if (!IsTopicHasRegister(topic)) {
        return;
    }

    std::vector<uint32_t> subscriberInfoIndex;
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    uint32_t nextBitmapBeginIndex = (topic + 1) << POWER_SIZE;
    for (uint32_t i = 0; i < nextBitmapBeginIndex; i++) {
        uint32_t value = bitTable_->bitmaps[bitmapBeginIndex];

    }

}

void PubSubManager::Publish(uint32_t topic)
{
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    for (uint32_t i = 0; i < POWER_SIZE; i++) {
        uint32_t bitmap = bitTable_->bitmaps[bitmapBeginIndex + i];
        while (bitmap != 0) {
            uint32_t tmpIndex = (bitmap - 1) & ~bitmap;
            tmpIndex = tmpIndex & 0x55555555 + (tmpIndex >> 1) & 0x55555555;
            tmpIndex = tmpIndex & 0x33333333 + (tmpIndex >> 2) & 0x33333333;
            tmpIndex = tmpIndex & 0x0F0F0F0F + (tmpIndex >> 4) & 0x0F0F0F0F;
            tmpIndex = (tmpIndex * 0x01010101) >> 24;
            tmpIndex += i * 32;

            if (SubscriberInfo *info = GetSubScriberInfo(tmpIndex), info != nullptr) {
                if (info->notify != nullptr) {
                    info->notify(info->arg);
                }
            }

            bitmap &= (bitmap - 1);
        }
    }
}

std::optional<uint32_t> PubSubManager::SubscriberRegister(const SubscriberInfo &info)
{
    uint32_t i = 0;
    for (i = 0; i < subScriberTable_->curSize; i++) {
        if (subScriberTable_->infos[i].id != info.id) {
            continue;
        }

        if (subScriberTable_->infos[i].notify != info.notify) {
            continue;
        }

        return i;
    }

    if (i < subScriberTable_->capacity) {
        subScriberTable_->infos[i] = info;
        subScriberTable_->curSize++;
        return i;
    }

    return std::nullopt;
}

void PubSubManager::UnSubScriberRegister(uint32_t topic)
{
}

bool PubSubManager::IsTopicHasRegister(uint32_t topic)
{
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    uint32_t nextTopicBeginIndex = (topic + 1) << POWER_SIZE;
    uint64_t total = 0;
    for (uint32_t i = bitmapBeginIndex; i < nextTopicBeginIndex; i++) {
        total += bitTable_->bitmaps[i]; 
    }

    return total != 0;
}

uint32_t PubSubManager::GetBit(uint32_t topic, uint32_t subscriberInfoIndex)
{
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    uint32_t bitmapIndex = subscriberInfoIndex >> 5;
    uint32_t bitmapOffset = subscriberInfoIndex & 31;
    return bitTable_->infos[bitmapBeginIndex + bitmapIndex] & bitmapOffset;
}

void PubSubManager::SetBit(uint32_t topic, uint32_t subscriberInfoIndex)
{
    uint32_t bitmapBeginIndex = topic << POWER_SIZE; // 计算topic起始索引，POWER_SIZE=1为topic*2即一个topic占用2个uint32_t（64bit）
    uint32_t bitmapIndex = index.value() >> 5; // index.value()/32，计算从bitmapBeginIndex开始第几个uint32_t偏移
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
    bitTable_->infos[bitmapBeginIndex + bitmapIndex] |= (1 << bitmapOffset);
}

void PubSubManager::ClrBit(uint32_t topic, uint32_t subscriberInfoIndex)
{
    uint32_t bitmapBeginIndex = topic << POWER_SIZE;
    uint32_t bitmapIndex = subscriberInfoIndex >> 5;
    uint32_t bitmapOffset = subscriberInfoIndex & (32 - 1);
    bitTable_->bitmaps[bitmapBeginIndex + bitmapIndex] &= ~(1 << bitmapOffset);
}

void PubSubManager::CollectSubscriberInfoIndex(std::vector &subScriberInfoIndexVec)
{
    
}

SubscriberInfo *PubSubManager::GetSubScriberInfo(uint32_t index)
{
    if (index >= SUBSCRIBER_MAX_NUM) {
        return nullptr;
    }

    return &subScriberTable_[index];
}

int main()
{

}