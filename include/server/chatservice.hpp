#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "group_model.hpp"
#include "redis.hpp"
#include "friendRequestModel.hpp"
#include "messagemodel.hpp"

using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;

// 回调函数类型
using MsgHandler = std::function<void(const TcpConnectionPtr&, json&, Timestamp)>;

// 聊天服务器业务类
class ChatService
{
public:
    // ChatService 单例模式
    static ChatService* instance() {
        static ChatService service;
        return &service;
    }

    // 获取对应消息的处理器
    MsgHandler getHandler(int msgId);
    // 登录业务
    void loginHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 注册业务
    void registerHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 搜索联系人业务
    void searchContactHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 请求头像业务
    void avatarHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 请求通知列表业务
    void noticesHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 好友验证业务
    void friendVerify(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 请求联系人列表业务
    void contactsHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 请求消息列表业务
    void messageListHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChatHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 请求历史消息
    void historyHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 请求下载文件
    void downloadFile(const TcpConnectionPtr &conn, json &js, Timestamp time);


    // 添加好友业务
    void addFriendHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);


    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseExceptionHandler(const TcpConnectionPtr &conn);
    // 服务端异常终止之后的操作
    void reset();
    //redis订阅消息触发的回调函数
    void redis_subscribe_message_handler(int channel, string message);


    //发送仅含有Json的数据
    void sendJson(const TcpConnectionPtr &conn, const json &response);
    //发送带文件的Json数据
    void sendJsonWithFile(const TcpConnectionPtr &conn, json &response);
    //base64编码
    std::string encodeBase64(const std::string &filePath);

private:
    ChatService();
    // 禁止拷贝构造函数
    ChatService(const ChatService&) = delete;
    // 禁止赋值操作符
    ChatService& operator=(const ChatService&) = delete;

    // 存储消息id和其对应的业务处理方法
    std::unordered_map<int, MsgHandler> m_msgHandlerMap;   
    // 存储在线用户的通信连接
    std::unordered_map<int, TcpConnectionPtr> m_userConnMap;

    // 定义互斥锁
    std::mutex m_connMutex;
    //redis操作对象
    Redis m_redis;

    // 数据操作类对象
    UserModel m_userModel;
    OfflineMsgModel m_offlineMsgModel;
    FriendModel m_friendModel;
    GroupModel m_groupModel;
    FriendRequestModel m_friendRequestModel;
    MessageModel m_messageModel;
};

#endif // CHATSERVICE_H