#include "XExec.h"
#include <iostream>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define WIN32_LEAN_AND_MEAN
#else
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#endif

class XExec::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        cleanup();
    }

    bool start(const std::string& cmd, bool redirectStderr, OutputCallback callback);
    int  wait();
    bool terminate();
    bool isRunning() const;

    std::string getStdout() const;
    std::string getStderr() const;
    std::string getAllOutput() const;

public:
    void cleanup();
    void readOutput(bool isStderr);

#ifdef _WIN32
    void closeAllHandles();
    bool checkProcessExited();
#else
    void closeAllFds();
    bool checkProcessExited();
#endif

#ifdef _WIN32
    struct ProcessHandles
    {
        HANDLE hProcess  = INVALID_HANDLE_VALUE;
        HANDLE hStdoutRd = INVALID_HANDLE_VALUE;
        HANDLE hStderrRd = INVALID_HANDLE_VALUE;
        HANDLE hStdoutWr = INVALID_HANDLE_VALUE;
        HANDLE hStderrWr = INVALID_HANDLE_VALUE;
        HANDLE hStdinRd  = INVALID_HANDLE_VALUE;
        HANDLE hStdinWr  = INVALID_HANDLE_VALUE;
    };

    ProcessHandles handles_;
#else
    struct ProcessHandles
    {
        pid_t pid      = -1;
        int   stdoutFd = -1;
        int   stderrFd = -1;
        int   stdinFd  = -1;
    };

    ProcessHandles handles_;
#endif

    mutable std::mutex mutex_;
    std::string        stdout_;
    std::string        stderr_;
    std::atomic<bool>  running_{ false };
    std::atomic<bool>  terminated_{ false };
    std::atomic<int>   exitCode_{ -1 };
    OutputCallback     outputCallback_;
    std::thread        stdoutThread_;
    std::thread        stderrThread_;
};

// XExec 公共接口实现
XExec::XExec() : impl_(std::make_unique<Impl>())
{
}
XExec::~XExec()                           = default;
XExec::XExec(XExec&&) noexcept            = default;
XExec& XExec::operator=(XExec&&) noexcept = default;

void XExec::setOutputCallback(OutputCallback callback)
{
    impl_->outputCallback_ = std::move(callback);
}

bool XExec::start(const std::string& cmd, bool redirectStderr)
{
    return impl_->start(cmd, redirectStderr, impl_->outputCallback_);
}

std::string XExec::getStdout() const
{
    return impl_->getStdout();
}
std::string XExec::getStderr() const
{
    return impl_->getStderr();
}
std::string XExec::getAllOutput() const
{
    return impl_->getAllOutput();
}
int XExec::wait()
{
    return impl_->wait();
}
bool XExec::isRunning() const
{
    return impl_->isRunning();
}
bool XExec::terminate()
{
    return impl_->terminate();
}

// 静态方法实现
XExec::XResult XExec::execute(const std::string& command, bool redirectStderr, int timeoutMs)
{
    XExec   exec;
    XResult result;

    // 启动命令
    if (!exec.start(command, redirectStderr))
    {
        result.exitCode     = -1;
        result.stderrOutput = "启动命令失败";
        return result;
    }

    // 设置超时
    if (timeoutMs > 0)
    {
        auto startTime = std::chrono::steady_clock::now();
        while (exec.isRunning())
        {
            auto elapsed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);

            if (elapsed.count() >= timeoutMs)
            {
                exec.terminate();
                result.exitCode = -2; // 超时
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // 等待完成
    result.exitCode     = exec.wait();
    result.stdoutOutput = exec.getStdout();
    result.stderrOutput = exec.getStderr();

    return result;
}

#ifdef _WIN32
// ==================== Windows 实现 ====================

void XExec::Impl::closeAllHandles()
{
    auto closeHandle = [](HANDLE& h)
    {
        if (h != INVALID_HANDLE_VALUE && h != nullptr)
        {
            ::CloseHandle(h);
            h = INVALID_HANDLE_VALUE;
        }
    };

    closeHandle(handles_.hStdoutRd);
    closeHandle(handles_.hStderrRd);
    closeHandle(handles_.hStdoutWr);
    closeHandle(handles_.hStderrWr);
    closeHandle(handles_.hStdinRd);
    closeHandle(handles_.hStdinWr);
    // 注意：hProcess 在 wait() 中单独处理
}

bool XExec::Impl::checkProcessExited()
{
    if (handles_.hProcess == INVALID_HANDLE_VALUE)
        return true;

    DWORD exitCode = STILL_ACTIVE;
    if (GetExitCodeProcess(handles_.hProcess, &exitCode))
    {
        return exitCode != STILL_ACTIVE;
    }
    return true; // 获取失败也认为已退出
}

bool XExec::Impl::start(const std::string& cmd, bool redirectStderr, OutputCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_)
    {
        std::cerr << "已有命令正在执行" << std::endl;
        return false;
    }

    outputCallback_ = std::move(callback);
    exitCode_       = -1;
    stdout_.clear();
    stderr_.clear();
    terminated_ = false;
    // 删除：reading_ = false;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength              = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle       = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    // 创建stdout管道
    if (!CreatePipe(&handles_.hStdoutRd, &handles_.hStdoutWr, &saAttr, 0))
    {
        std::cerr << "创建stdout管道失败" << std::endl;
        cleanup();
        return false;
    }

    if (!SetHandleInformation(handles_.hStdoutRd, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "设置stdout管道信息失败" << std::endl;
        cleanup();
        return false;
    }

    // 创建stderr管道（如果不重定向到stdout）
    if (!redirectStderr)
    {
        if (!CreatePipe(&handles_.hStderrRd, &handles_.hStderrWr, &saAttr, 0))
        {
            std::cerr << "创建stderr管道失败" << std::endl;
            cleanup();
            return false;
        }

        if (!SetHandleInformation(handles_.hStderrRd, HANDLE_FLAG_INHERIT, 0))
        {
            std::cerr << "设置stderr管道信息失败" << std::endl;
            cleanup();
            return false;
        }
    }

    // 创建stdin管道
    if (!CreatePipe(&handles_.hStdinRd, &handles_.hStdinWr, &saAttr, 0))
    {
        std::cerr << "创建stdin管道失败" << std::endl;
        cleanup();
        return false;
    }

    if (!SetHandleInformation(handles_.hStdinWr, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "设置stdin管道信息失败" << std::endl;
        cleanup();
        return false;
    }

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOA        siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));

    siStartInfo.cb         = sizeof(STARTUPINFOA);
    siStartInfo.hStdInput  = handles_.hStdinRd;
    siStartInfo.hStdOutput = handles_.hStdoutWr;
    siStartInfo.hStdError  = redirectStderr ? handles_.hStdoutWr : handles_.hStderrWr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmdCopy = cmd; // 创建副本，CreateProcessA可能修改字符串
    BOOL bSuccess = CreateProcessA(nullptr, cmdCopy.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr,
                                   &siStartInfo, &piProcInfo);

    // 关闭子进程不用的句柄（重要！）
    CloseHandle(handles_.hStdoutWr);
    handles_.hStdoutWr = INVALID_HANDLE_VALUE;
    CloseHandle(handles_.hStdinRd);
    handles_.hStdinRd = INVALID_HANDLE_VALUE;

    if (!redirectStderr)
    {
        CloseHandle(handles_.hStderrWr);
        handles_.hStderrWr = INVALID_HANDLE_VALUE;
    }

    if (!bSuccess)
    {
        DWORD error = GetLastError();
        std::cerr << "创建进程失败，错误码: " << error << std::endl;
        cleanup();
        return false;
    }

    handles_.hProcess = piProcInfo.hProcess;
    CloseHandle(piProcInfo.hThread); // 线程句柄不需要

    // 关键：先设置 running_，然后立即启动线程
    running_.store(true, std::memory_order_release);

    // 直接启动线程，不需要检查
    stdoutThread_ = std::thread(
            [this]()
            {
                readOutput(false); // 直接调用
            });

    if (!redirectStderr)
    {
        stderrThread_ = std::thread(
                [this]()
                {
                    readOutput(true); // 直接调用
                });
    }

    return true;
}

void XExec::Impl::readOutput(bool isStderr)
{
    HANDLE hPipe = isStderr ? handles_.hStderrRd : handles_.hStdoutRd;
    char   buffer[4096];
    DWORD  bytesRead;

    // 使用 running_ 作为条件，而不是 reading_
    while (running_.load(std::memory_order_acquire))
    {
        BOOL readResult = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);

        if (readResult && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            std::string output(buffer);

            std::lock_guard<std::mutex> lock(mutex_);
            if (isStderr)
            {
                stderr_ += output;
            }
            else
            {
                stdout_ += output;
            }

            if (outputCallback_)
            {
                // 按行回调
                std::istringstream stream(output);
                std::string        line;
                while (std::getline(stream, line))
                {
                    if (!line.empty() && line.back() == '\r')
                    {
                        line.pop_back();
                    }
                    if (!line.empty())
                    {
                        outputCallback_(line, isStderr);
                    }
                }
            }
        }
        else
        {
            // 检查是否应该退出
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA || bytesRead == 0)
            {
                // 管道已关闭，正常退出
                running_.store(false, std::memory_order_release);
                break;
            }

            // 短暂休眠避免忙等待
            Sleep(10);
        }
    }
}

int XExec::Impl::wait()
{
    if (!running_ && exitCode_ != -1)
    {
        return exitCode_;
    }

    // 步骤1：等待进程退出
    if (handles_.hProcess != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(handles_.hProcess, INFINITE);

        DWORD dwExitCode;
        if (GetExitCodeProcess(handles_.hProcess, &dwExitCode))
        {
            exitCode_ = static_cast<int>(dwExitCode);
        }
        else
        {
            exitCode_ = -1;
        }
    }

    // 步骤2：关闭管道，让读取线程自然退出
    closeAllHandles();

    // 步骤3：等待线程结束
    if (stdoutThread_.joinable())
    {
        stdoutThread_.join();
    }

    if (stderrThread_.joinable())
    {
        stderrThread_.join();
    }

    // 步骤4：关闭进程句柄
    if (handles_.hProcess != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handles_.hProcess);
        handles_.hProcess = INVALID_HANDLE_VALUE;
    }

    // 步骤5：最后更新状态
    running_.store(false, std::memory_order_release);

    return exitCode_;
}

bool XExec::Impl::terminate()
{
    if (!running_ || handles_.hProcess == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    terminated_ = true;

    if (!TerminateProcess(handles_.hProcess, 1))
    {
        std::cerr << "终止进程失败" << std::endl;
        return false;
    }

    wait(); // 等待清理
    return true;
}

#else
// ==================== Linux/macOS 实现 ====================

void XExec::Impl::closeAllFds()
{
    auto closeFd = [](int& fd)
    {
        if (fd != -1)
        {
            close(fd);
            fd = -1;
        }
    };

    closeFd(handles_.stdoutFd);
    closeFd(handles_.stderrFd);
    closeFd(handles_.stdinFd);
}

bool XExec::Impl::checkProcessExited()
{
    if (handles_.pid <= 0)
        return true;

    int   status;
    pid_t result = waitpid(handles_.pid, &status, WNOHANG);
    return result > 0;
}

bool XExec::Impl::start(const std::string& cmd, bool redirectStderr, OutputCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_)
    {
        std::cerr << "已有命令正在执行" << std::endl;
        return false;
    }

    outputCallback_ = std::move(callback);
    exitCode_       = -1;
    stdout_.clear();
    stderr_.clear();
    terminated_ = false;
    // 删除：reading_ = false;

    int stdoutPipe[2] = { -1, -1 };
    int stderrPipe[2] = { -1, -1 };
    int stdinPipe[2]  = { -1, -1 };

    // 创建管道
    if (pipe(stdoutPipe) == -1)
    {
        std::cerr << "创建stdout管道失败" << std::endl;
        return false;
    }

    if (pipe(stdinPipe) == -1)
    {
        std::cerr << "创建stdin管道失败" << std::endl;
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return false;
    }

    if (!redirectStderr && pipe(stderrPipe) == -1)
    {
        std::cerr << "创建stderr管道失败" << std::endl;
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return false;
    }

    handles_.pid = fork();

    if (handles_.pid == -1)
    {
        std::cerr << "fork失败" << std::endl;
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        if (!redirectStderr)
        {
            close(stderrPipe[0]);
            close(stderrPipe[1]);
        }
        return false;
    }

    if (handles_.pid == 0) // 子进程
    {
        // 关闭父进程用的读端
        close(stdoutPipe[0]);
        close(stdinPipe[1]);

        // 重定向stdin
        if (dup2(stdinPipe[0], STDIN_FILENO) == -1)
        {
            exit(127);
        }
        close(stdinPipe[0]);

        // 重定向stdout
        if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1)
        {
            exit(127);
        }
        close(stdoutPipe[1]);

        // 重定向stderr
        if (redirectStderr)
        {
            if (dup2(stdoutPipe[1], STDERR_FILENO) == -1)
            {
                exit(127);
            }
        }
        else
        {
            close(stderrPipe[0]);
            if (dup2(stderrPipe[1], STDERR_FILENO) == -1)
            {
                exit(127);
            }
            close(stderrPipe[1]);
        }

        // 执行命令
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(127); // exec失败
    }
    else // 父进程
    {
        // 关闭子进程用的写端
        close(stdoutPipe[1]);
        handles_.stdoutFd = stdoutPipe[0];
        close(stdinPipe[0]);
        handles_.stdinFd = stdinPipe[1];

        if (!redirectStderr)
        {
            close(stderrPipe[1]);
            handles_.stderrFd = stderrPipe[0];
        }

        // 设置管道非阻塞
        fcntl(handles_.stdoutFd, F_SETFL, O_NONBLOCK);
        if (!redirectStderr)
        {
            fcntl(handles_.stderrFd, F_SETFL, O_NONBLOCK);
        }

        // 关键：先设置 running_，然后立即启动线程
        running_.store(true, std::memory_order_release);

        // 直接启动线程
        stdoutThread_ = std::thread([this]() { readOutput(false); });

        if (!redirectStderr)
        {
            stderrThread_ = std::thread([this]() { readOutput(true); });
        }
    }

    return true;
}

void XExec::Impl::readOutput(bool isStderr)
{
    int  fd = isStderr ? handles_.stderrFd : handles_.stdoutFd;
    char buffer[4096];

    // 使用 running_ 作为条件
    while (running_.load(std::memory_order_acquire))
    {
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            std::string output(buffer);

            std::lock_guard<std::mutex> lock(mutex_);
            if (isStderr)
            {
                stderr_ += output;
            }
            else
            {
                stdout_ += output;
            }

            if (outputCallback_)
            {
                std::istringstream stream(output);
                std::string        line;
                while (std::getline(stream, line))
                {
                    if (!line.empty())
                    {
                        outputCallback_(line, isStderr);
                    }
                }
            }
        }
        else if (bytesRead == 0)
        {
            // EOF，管道已关闭
            break;
        }
        else if (bytesRead == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 非阻塞，没有数据
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            else
            {
                // 其他错误
                break;
            }
        }
    }
}

int XExec::Impl::wait()
{
    if (!running_ && exitCode_ != -1)
    {
        return exitCode_;
    }

    // 步骤1：等待进程退出
    if (handles_.pid > 0)
    {
        int status;
        waitpid(handles_.pid, &status, 0);

        if (WIFEXITED(status))
        {
            exitCode_ = WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            exitCode_ = 128 + WTERMSIG(status); // 按照bash惯例
        }
        else
        {
            exitCode_ = -1;
        }
    }

    // 步骤2：关闭文件描述符，让读取线程自然退出
    closeAllFds();

    // 步骤3：等待线程结束
    if (stdoutThread_.joinable())
    {
        stdoutThread_.join();
    }

    if (stderrThread_.joinable())
    {
        stderrThread_.join();
    }

    // 步骤4：最后更新状态
    running_.store(false, std::memory_order_release);

    // 步骤5：清理
    cleanup();

    return exitCode_;
}

bool XExec::Impl::terminate()
{
    if (!running_ || handles_.pid <= 0)
    {
        return false;
    }

    terminated_ = true;

    if (kill(handles_.pid, SIGTERM) == -1)
    {
        std::cerr << "发送SIGTERM失败" << std::endl;
        return false;
    }

    // 等待一段时间
    for (int i = 0; i < 50; ++i) // 最多等待5秒
    {
        if (!isRunning())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 强制终止
    if (kill(handles_.pid, SIGKILL) == -1)
    {
        std::cerr << "发送SIGKILL失败" << std::endl;
        return false;
    }

    return true;
}

#endif

// ==================== 跨平台通用实现 ====================

bool XExec::Impl::isRunning() const
{
    return running_;
}

std::string XExec::Impl::getStdout() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stdout_;
}

std::string XExec::Impl::getStderr() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stderr_;
}

std::string XExec::Impl::getAllOutput() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stdout_ + stderr_;
}

void XExec::Impl::cleanup()
{
#ifdef _WIN32
    auto closeHandle = [](HANDLE& h)
    {
        if (h != INVALID_HANDLE_VALUE && h != nullptr)
        {
            ::CloseHandle(h);
            h = INVALID_HANDLE_VALUE;
        }
    };

    closeHandle(handles_.hProcess);
    closeHandle(handles_.hStdoutRd);
    closeHandle(handles_.hStderrRd);
    closeHandle(handles_.hStdoutWr);
    closeHandle(handles_.hStderrWr);
    closeHandle(handles_.hStdinRd);
    closeHandle(handles_.hStdinWr);
#else
    auto closeFd = [](int& fd)
    {
        if (fd != -1)
        {
            close(fd);
            fd = -1;
        }
    };

    closeFd(handles_.stdoutFd);
    closeFd(handles_.stderrFd);
    closeFd(handles_.stdinFd);
    handles_.pid = -1;
#endif

    running_.store(false, std::memory_order_release);
    // 删除：reading_ = false;
}
