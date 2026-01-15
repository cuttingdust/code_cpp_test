#include "XExec.h"

#include <iostream>
#include <future>
#include <iomanip>
#include <queue>

#ifndef _WIN32
#define _popen  popen
#define _pclose pclose
#endif

class XExec::PImpl
{
public:
    PImpl(XExec *owenr);
    ~PImpl() = default;

public:
    XExec                  *owenr_     = nullptr;
    bool                    isRunning_ = false;
    std::queue<std::string> outList_; /// 命令结果缓存队列
    std::future<bool>       fut_;
    std::mutex              mtx_;
};

XExec::PImpl::PImpl(XExec *owenr) : owenr_(owenr)
{
}


XExec::XExec()
{
}

XExec::~XExec()                            = default;
XExec::XExec(XExec &&) noexcept            = default;
XExec &XExec::operator=(XExec &&) noexcept = default;

auto XExec::start(const char *cmd) -> bool
{
    std::cout << "Start Cmd: " << cmd << std::endl;
    auto fp = _popen(cmd, "r");
    if (!fp)
        return false;

    impl_->isRunning_ = true;

    impl_->fut_ = std::async(
            [fp, this]()
            {
                std::string tmp;
                char        c = 0;
                while (c = fgetc(fp))
                {
                    if (c == EOF)
                        break;

                    /// \r 回到当前行开头 /// ffmpeg的进度条 就是 当前行的开头
                    /// \n 回到下一行的开头
                    if (c == '\r' || c == '\n')
                    {
                        if (tmp.empty())
                            continue;

                        // std::cout << tmp << std::endl;
                        {
                            std::lock_guard<std::mutex> lg{ impl_->mtx_ };
                            impl_->outList_.emplace(tmp);
                        }
                        tmp = "";
                        continue;
                    }

                    tmp += c;
                }

                impl_->isRunning_ = false;
                _pclose(fp);
                return true;
            });
    return true;
}

auto XExec::isRunning() const -> bool
{
    return impl_->isRunning_;
}

auto XExec::getOutput(std::string &ret) const -> bool
{
    std::lock_guard<std::mutex> lg{ impl_->mtx_ };

    if (impl_->outList_.empty())
        return false;

    ret = std::move(impl_->outList_.front());
    impl_->outList_.pop();
    return true;
}

auto XExec::wait() -> bool
{
    return impl_->fut_.get();
}
