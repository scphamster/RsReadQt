#include "arinc_label_item.hpp"
#include "child_container.hpp"
#include <QDateTime>
#include "arinc_label.hpp"
#include "proxy_arinc_label_item.hpp"

using ArincRole = ProxyArincLabelItem<1>::ArincRole;

ArincData::ArincData(ArincQModel::ChannelNumT belongs_to_channel,
                     const _ArincLabel       &lbl,
                     const QDateTime         &firstTime,
                     const QDateTime         &lastTime,
                     HitCounterT              counter,
                     Qt::CheckState           ifBeep,
                     Qt::CheckState           ifVisible)
  : label{ lbl }
  , channel{ belongs_to_channel }
  , firstOccurrence{ firstTime }
  , lastOccurrence{ lastTime }
  , hitCount{ counter }
{ }

ArincData::ArincData(const ArincData &other)            = default;
ArincData &ArincData::operator=(const ArincData &other) = default;
ArincData::ArincData(ArincData &&other)                 = default;
ArincData &ArincData::operator=(ArincData &&other)      = default;
ArincData::~ArincData()                                 = default;

void
ArincData::SetFirstOccurrence(const QDateTime &when)
{
    firstOccurrence = when;
}
void
ArincData::SetLastOccurrence(const QDateTime &when)
{
    lastOccurrence = when;
}
void
ArincData::SetHitCount(HitCounterT quantity) noexcept
{
    hitCount = quantity;
}
void
ArincData::IncrementHitCount(HitCounterT increment_value /*= 1*/) noexcept
{
    hitCount += increment_value;
}
void
ArincData::AppendMessage(std::shared_ptr<ArincMsg> msg)
{
    messages.push_back(msg);
    IncrementHitCount();
}

[[nodiscard]] ArincQModel::ChannelNumT
ArincData::GetChannelAffinity() const noexcept
{
    return channel;
}
[[nodiscard]] QDateTime
ArincData::GetFirstOccurrence() const
{
    return firstOccurrence;
}
[[nodiscard]] QDateTime
ArincData::GetLastOccurrence() const
{
    return lastOccurrence;
}
[[nodiscard]] HitCounterT
ArincData::GetHitCount() const noexcept
{
    return hitCount;
}
[[nodiscard]] const std::vector<std::shared_ptr<ArincMsg>> &
ArincData::GetMessages()
{
    return messages;
}

CompositeArincItem::CompositeArincItem()
  : arincData{ std::make_shared<ArincData>() }
  , treeData{ new ArincTreeData{} }
{
    SetupTreeDataFromArincData();
}
