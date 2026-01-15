#pragma once

#include "XConst.h"

class XExec
{
public:
    XExec();
    ~XExec();

    /// 允许移动
    XExec(XExec &&) noexcept;
    XExec &operator=(XExec &&) noexcept;

public:
    auto start(const char *cmd) -> bool;

    auto isRunning() const -> bool;

    auto getOutput(std::string &ret) const -> bool;

    auto wait() -> bool;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
