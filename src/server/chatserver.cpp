#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <fstream>
#include <functional>
#include <string>
using namespace placeholders;
using json = nlohmann::json;


// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : m_server(loop, listenAddr, nameArg), m_loop(loop)
{
    // 注册连接事件的回调函数
    m_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 注册消息事件的回调函数
    m_server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置subLoop线程数量
    m_server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    m_server.start();
}

// 连接事件相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if (!conn->connected())
    {
        // 处理客户端异常退出事件
        ChatService::instance()->clientCloseExceptionHandler(conn);
        // 半关闭
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    static int remainingSize = 0; // 用于记录当前文件剩余大小
    static std::ofstream outFile; // 用于文件写入
    static json pendingJson; // 用于存储待处理的普通 JSON 消息

    while (buffer->readableBytes() > 0)
    {
        if (remainingSize == 0) // 当前没有正在处理的文件
        {
            // 确保至少有消息头的长度
            if (buffer->readableBytes() < sizeof(int))
            {
                return; // 数据不完整，等待更多数据到来
            }
            // 读取消息头，获取 JSON 数据长度
            const int jsonLen = buffer->peekInt32();
            std::cout << "JSON 数据长度：" << jsonLen << " 字节" << std::endl;

            if (buffer->readableBytes() < sizeof(int) + jsonLen)
            {
                return; // 数据不完整，等待更多数据到来
            }
            // 移除消息头
            buffer->retrieve(sizeof(int));
            // 读取 JSON 数据
            std::string jsonStr = buffer->retrieveAsString(jsonLen);
            try
            {
                // 解析 JSON 数据
                json js = json::parse(jsonStr);
                std::cout << "Received JSON: " << js.dump() << std::endl;

                // 检查是否包含文件信息
                if (js.contains("fileinfo"))
                {
                    std::cout << "有文件传来" << std::endl;

                    // 处理文件传输
                    std::string fileName = js["fileinfo"]["filename"].get<std::string>();
                    int fileSize = js["fileinfo"]["filesize"].get<int>();

                    // 文件存储路径
                    std::string filePath = "../cache/" + fileName;

                    std::cout << "开始接收文件：" << fileName << "，大小：" << fileSize << " 字节" << std::endl;

                    // 打开文件以写入
                    outFile.open(filePath, std::ios::binary);
                    if (!outFile.is_open())
                    {
                        std::cerr << "无法打开文件进行写入：" << fileName << std::endl;
                        return;
                    }

                    // 初始化剩余大小
                    remainingSize = fileSize;

                    // 保存待处理的普通 JSON 消息
                    pendingJson = js;
                }
                else
                {
                    // 处理普通 JSON 消息
                    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
                    msgHandler(conn, js, time);
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "JSON 解析错误：" << e.what() << std::endl;
                outFile.close();
            }
        }
        else // 如果有文件需要接收
        {
            if (buffer->readableBytes() > 0)
            {
                int chunkSize = std::min(remainingSize, static_cast<int>(buffer->readableBytes()));
                std::string fileData = buffer->retrieveAsString(chunkSize);
                outFile.write(fileData.data(), chunkSize);
                remainingSize -= chunkSize;

                std::cout << "已接收：" << chunkSize << " 字节, 剩余：" << remainingSize << " 字节" << std::endl;

                if (remainingSize == 0)
                {
                    outFile.close();
                    std::cout << "文件接收完成" << std::endl;

                    // 文件接收完成后，处理之前保存的普通 JSON 消息
                    if (!pendingJson.empty())
                    {
                        auto msgHandler = ChatService::instance()->getHandler(pendingJson["msgid"].get<int>());
                        msgHandler(conn, pendingJson, time);
                        pendingJson.clear(); // 清空待处理的 JSON 消息
                    }
                }
            }
        }
    }
    
}