#ifndef XEXEC_H
#define XEXEC_H

#include <string>
#include <functional>
#include <memory>

class XExec
{
public:
    using OutputCallback = std::function<void(const std::string& line, bool isStderr)>;

    struct XResult
    {
        int         exitCode = -1;
        std::string stdoutOutput;
        std::string stderrOutput;
    };

    XExec();
    ~XExec();
    XExec(XExec&&) noexcept;
    XExec& operator=(XExec&&) noexcept;

    // 删除拷贝构造和赋值
    XExec(const XExec&)            = delete;
    XExec& operator=(const XExec&) = delete;

    void setOutputCallback(OutputCallback callback);

    bool start(const std::string& cmd, bool redirectStderr = true);

    std::string getStdout() const;
    std::string getStderr() const;
    std::string getAllOutput() const;

    int  wait();
    bool isRunning() const;
    bool terminate();

    static XResult execute(const std::string& command, bool redirectStderr = true, int timeoutMs = 0);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // XEXEC_H
