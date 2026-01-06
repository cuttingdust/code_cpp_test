#include <iostream>
#include <map>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace std
{
    class mutex;
}

struct Msg
{
    std::string key;
    std::string data;
};

class MsgServer
{
public:
    using MsgCall = std::function<void(const Msg &)>;

    /// \brief 根据key关联函数对象 注册消息处理回调
    /// \param key 消息的关键字，用于查找函数
    /// \param call 回调函数
    void Reg(const std::string &key, MsgCall call)
    {
        calls_[key] = std::move(call);
    }

    /// \brief 转发消息
    /// \param msg
    void Distribute(const Msg &msg)
    {
        auto ptr = calls_.find(msg.key);
        if (ptr == calls_.end())
            return;
        ptr->second(msg);
        //calls_[msg.key](msg);
    }

    /// \brief 存储用户发送的消息
    /// \param msg 缓存msg
    void Send(const Msg &msg)
    {
        msgs_.push(msg);
    }

    void Start()
    {
        th_ = std::thread(&MsgServer::Run, this);
    }

    void Wait()
    {
        if (th_.joinable())
            th_.join();
    }

    void Stop()
    {
        is_exit_ = true;
    }

private:
    void Run()
    {
        std::cout << __FUNCSIG__ << std::endl;
        while (!is_exit_) /// 消息处理主循环
        {
            /// 读取消息队列数据
            /// 读取消息队列数据
            if (msgs_.empty())
            {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                continue;
            }
            /// 取第一个数据
            Msg msg = msgs_.front();
            {
                std::lock_guard<std::mutex> lock(mux_);
                /// 取第一个数据
                msg = msgs_.front();
                msgs_.pop(); /// 清掉第一个数据
            }
            Distribute(msg);
        }
    }

private:
    std::map<std::string, MsgCall> calls_;            ///< 根据消息的key调用函数对象
    std::queue<Msg>                msgs_;             ///< 消息缓存队列
    std::thread                    th_;               ///< 处理缓存消息和转发的现场
    bool                           is_exit_{ false }; ///< 线程是否退出
    std::mutex                     mux_;              ///< 消息队列互斥访问
};

class HttpServer : public MsgServer
{
public:
    void Init()
    {
        Reg("post", [this](const Msg &msg) { Post(msg); });
        Reg("get", [this](const Msg &msg) { Get(msg); });
        Reg("head", [this](const Msg &msg) { Head(msg); });
    }

private:
    void Post(const Msg &msg)
    {
        std::cout << __FUNCSIG__ << msg.key << ":" << msg.data << std::endl;
    }
    void Get(const Msg &msg)
    {
        std::cout << __FUNCSIG__ << msg.key << ":" << msg.data << std::endl;
    }
    void Head(const Msg &msg)
    {
        std::cout << __FUNCSIG__ << msg.key << ":" << msg.data << std::endl;
    }
};

int main(int argc, char *argv[])
{
    HttpServer server;
    server.Init();
    server.Start();
    server.Send({ .key = "post", .data = "test post data" });
    server.Send({ .key = "get", .data = "test get data" });
    server.Send({ .key = "head", .data = "test head data" });
    std::this_thread::sleep_for(std::chrono::seconds(3));
    server.Stop();
    server.Wait();


    // MsgServer server;
    // server.Reg("post", [](const Msg &msg)
    //            { std::cout << "reg post test post msg " << msg.key << ":" << msg.data << std::endl; });
    //
    // Msg msg{ .key = "post", .data = "test post data" };
    // server.Send(msg);
    // server.Send(msg);
    // server.Start();
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    // server.Stop();
    // server.Wait();

    // server.Distribute({ .key = "post", .data = "test post data" });

    return 0;
}
