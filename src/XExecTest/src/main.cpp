#include <iostream>
#include <string>
#include <fstream>
#include <future>
#include <iomanip>
#include <memory>
#include <sstream>
#include <iomanip>
#include <queue>

#ifndef _WIN32
#define _popen  popen
#define _pclose pclose
#endif


class XExec
{
public:
    bool start(const char *cmd)
    {
        std::cout << "Start Cmd: " << cmd << std::endl;
        auto fp = _popen(cmd, "r");
        if (!fp)
            return false;

        isRunning_ = true;

        fut_ = std::async(
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
                                std::lock_guard<std::mutex> lg{ mtx_ };
                                outList_.emplace(tmp);
                            }
                            tmp = "";
                            continue;
                        }

                        tmp += c;
                    }

                    isRunning_ = false;
                    _pclose(fp);
                    return true;
                });
        return true;
    }

    bool isRunning() const
    {
        return isRunning_;
    }

    bool getOutput(std::string &ret)
    {
        std::lock_guard<std::mutex> lg{ mtx_ };

        if (outList_.empty())
            return false;

        ret = std::move(outList_.front());
        outList_.pop();
        return true;
    }

    bool wait()
    {
        return fut_.get();
    }

private:
    bool                    isRunning_ = false;
    std::queue<std::string> outList_; /// 命令结果缓存队列
    std::future<bool>       fut_;
    std::mutex              mtx_;
};


int main(int argc, char *argv[])
{
    XExec       exec;
    std::string cmdRet;
    // exec.start("cd");
    exec.start("ping 127.0.0.1 -t");
    while (exec.isRunning())
    {
        exec.getOutput(cmdRet);
        std::cout << cmdRet << std::endl;
    }
    exec.wait();
    getchar();
    return 0;
}
