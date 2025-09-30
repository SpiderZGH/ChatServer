#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    /*
    用lambda表达式注册消息回调
    _server.setMessageCallback(
    [this](const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        this->onMessage(conn, buf, time);
    }
    );
    */

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开链接
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();

    // 测试，添加json打印代码
    cout << buf << endl;

    // 数据的反序列化
    json js = json::parse(buf);
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取=》业务handler=》conn  js  time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);

    /*
    从这里走一遍逻辑:我理一理
    1.客户端发送数据到服务器
    2.服务器收到数据，将数据存储到buffer中
    3.服务器将数据从buffer中取出来，将数据反序列化成json
    4.根据json中的msgid找到对应的业务处理函数
        这里的细节：
        getHandler通过查找_msgHandlerMap找到对应的业务处理函数
        _msgHandlerMap在ChatService的构造函数中被初始化
        _msgHandlerMap是一个unordered_map，key是int类型，value是函数指针类型
        关键就在于这个函数指针类型
        这个函数指针类型是一个std::function类型
        这个std::function类型是一个函数对象，可以存储任何可调用对象
        这个std::function类型的函数对象的类型是void(const TcpConnectionPtr &conn, json &js, Timestamp)


    5.将conn，json，time作为参数传递给业务处理函数
    6.业务处理函数根据参数执行相应的业务逻辑
    7.业务处理函数将结果序列化成json，将json发送给客户端
    8.客户端收到数据，将数据反序列化成json，将json中的数据展示给用户
    */
}