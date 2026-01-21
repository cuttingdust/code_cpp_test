#include "ProgressBarConfig.h"

auto ProgressBarConfig::clone() const -> ProgressBarConfig::Ptr
{
    return std::make_shared<ProgressBarConfig>(*this);
}

IMPLEMENT_CREATE(ProgressBarConfig)
