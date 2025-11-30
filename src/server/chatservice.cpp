#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <boost/beast/core/detail/base64.hpp>
#include <set>

using namespace muduo;

ChatService::ChatService()
{
    // 对各类消息处理方法的注册
    m_msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::loginHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({REGISTER_MSG, std::bind(&ChatService::registerHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChatHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriendHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({SEARCH_CONTACT, std::bind(&ChatService::searchContactHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({AVATAR_MSG, std::bind(&ChatService::avatarHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({NOTICES_MSG, std::bind(&ChatService::noticesHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({FRIEND_VERITY, std::bind(&ChatService::friendVerify, this, _1, _2, _3)});
    m_msgHandlerMap.insert({CONTACTS_MSG, std::bind(&ChatService::contactsHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({MESSAGELIST_MSG,std::bind(&ChatService::messageListHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({HISTORY_MSG,std::bind(&ChatService::historyHandler, this, _1, _2, _3)});
    m_msgHandlerMap.insert({DOWNLOADFILE_MSG,std::bind(&ChatService::downloadFile, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    m_msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    m_msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    m_msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    if (m_redis.connect())
    {
        m_redis.init_notify_handler(std::bind(&ChatService::redis_subscribe_message_handler, this, _1, _2));
    }
}

//处理请求消息列表的业务
void ChatService::messageListHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = std::stoi(js["userid"].get<std::string>());

    // 查询该用户的所有离线消息
    std::vector<json> offlineMessages = m_messageModel.queryOfflineMessages(userId);

    // 用于存储每个用户的最新消息和离线消息总数
    std::unordered_map<int, std::pair<json, int>> userMessageMap;

    for (const auto &message : offlineMessages)
    {
        int senderId = message["sender_id"].get<int>();

        // 如果该用户的消息尚未记录，初始化
        if (userMessageMap.find(senderId) == userMessageMap.end())
        {
            userMessageMap[senderId] = {message, 1};
        }
        else
        {
            // 更新离线消息总数
            userMessageMap[senderId].second++;

            // 如果当前消息的时间戳比记录的最新消息更晚，则更新最新消息
            if (message["timestamp"].get<std::string>() > userMessageMap[senderId].first["timestamp"].get<std::string>())
            {
                userMessageMap[senderId].first = message;
            }
        }
    }

    // 构造返回的 JSON 数据
    json response;
    response["msgid"] = MESSAGELIST_MSG_ACK;

    if (!userMessageMap.empty())
    {
        response["errno"] = 0;
        std::vector<json> messageList;

        for (const auto &[senderId, messageInfo] : userMessageMap)
        {
            json messageData;
            messageData["userid"] = senderId;
            messageData["latest_message"] = messageInfo.first; // 最新消息
            messageData["unread_count"] = messageInfo.second;  // 离线消息总数
            messageList.push_back(messageData);
        }

        response["message_list"] = messageList;
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "没有离线消息";
    }

    // 发送 JSON 数据到客户端
    sendJson(conn, response);
}

// 处理请求历史消息
void ChatService::historyHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = std::stoi(js["userid"].get<std::string>());
    int friendId = std::stoi(js["friendId"].get<std::string>());
    int offset = js["offset"].get<int>(); // 分页偏移量
    int limit = 5; // 每页最多返回 5 条记录

    // 查询聊天记录
    std::vector<json> chatHistory = m_messageModel.queryChatHistory(userId, friendId, offset, limit);

    // 构造返回的 JSON 数据
    json response;
    response["msgid"] = HISTORY_MSG_ACK;

    if (!chatHistory.empty())
    {
        response["errno"] = 0;

        // 遍历聊天记录，处理图片类型的消息
        for (auto &message : chatHistory)
        {
            if (message["type"] == "image")
            {
                // 将图片路径的内容编码为 Base64
                std::string filePath = "../cache/" + message["content"].get<std::string>();
                message["content"] = encodeBase64(filePath);
            }
        }

        response["history"] = chatHistory;
        response["count"] = chatHistory.size(); // 返回实际记录数

        // 将这些消息标记为已读
        std::vector<int> messageIds;
        for (const auto &message : chatHistory)
        {
            messageIds.push_back(message["id"].get<int>());
        }
        m_messageModel.markMessagesAsRead(messageIds);
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "没有更多聊天记录";
        response["count"] = 0; // 没有记录时返回 0
    }

    // 发送 JSON 数据到客户端
    sendJson(conn, response);
}

// 处理下载文件请求
void ChatService::downloadFile(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    std::string fileName = js["fileName"].get<std::string>();

    json fileInfoJson;
    fileInfoJson["filename"] = fileName;
    json response;
    response["msgid"] = 100;
    response["fileinfo"] = fileInfoJson;

    sendJsonWithFile(conn, response);
}

// redis订阅消息触发的回调函数,这里channel其实就是id
void ChatService::redis_subscribe_message_handler(int channel, string message)
{
    //用户在线
    lock_guard<mutex> lock(m_connMutex);
    auto it = m_userConnMap.find(channel);
    if (it != m_userConnMap.end())
    {
        it->second->send(message);
        return;
    }

    //转储离线
    m_offlineMsgModel.insert(channel, message);
}

MsgHandler ChatService::getHandler(int msgId)
{
    // 找不到对应处理器的情况
    auto it = m_msgHandlerMap.find(msgId);
    if (it == m_msgHandlerMap.end())
    {
        // 返回一个默认的处理器(lambda匿名函数，仅仅用作提示)
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgId: " << msgId << " can not find handler!";
        }; 
    }
  
    return m_msgHandlerMap[msgId];
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 将所有online状态的用户，设置成offline
    m_userModel.resetStatus();
}


/**
 * 处理客户端异常退出
 */
void ChatService::clientCloseExceptionHandler(const TcpConnectionPtr &conn)
{
    User user;
    // 互斥锁保护
    {
        lock_guard<mutex> lock(m_connMutex);
        for (auto it = m_userConnMap.begin(); it != m_userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                m_userConnMap.erase(it);
                break;
            }
        }
    }


    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setStatus("offline");
        m_userModel.updateStatus(user);
    }
}

// 处理请求头像业务
void ChatService::avatarHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = std::stoi(js["userid"].get<std::string>());
    User user = m_userModel.query(userid);

    json response;
    response["msgid"] = AVATAR_MSG_ACK;

    if (user.getId() == -1) // 用户不存在
    {
        response["errno"] = 1;
        response["errmsg"] = "用户不存在";
        sendJson(conn, response);
        return;
    }

    // 将头像文件编码为 Base64
    std::string avatarPath = "../cache/" + user.getChathead();
    response["avatar"] = encodeBase64(avatarPath);
    response["username"] = user.getName();
    response["errno"] = 0;

    sendJson(conn, response);
}

// 处理请求通知列表业务
void ChatService::noticesHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = std::stoi(js["userid"].get<std::string>());

    // 查询好友请求通知列表
    std::vector<FriendRequest> requests = m_friendRequestModel.queryRequests(userId);

    // 构造返回的 JSON 数据
    json response;
    response["msgid"] = NOTICES_MSG_ACK;

    if (!requests.empty())
    {
        response["errno"] = 0;
        std::vector<json> notices;
        for (const auto &request : requests)
        {
            json notice;
            notice["requestId"] = request.id;
            notice["fromId"] = request.fromId;
            notice["fromName"] = request.fromName;
            notice["avatar"] = encodeBase64("../cache/" + request.fromAvatar); // 将头像文件编码为 Base64
            notice["timestamp"] = request.requestTime;
            notices.push_back(notice);
        }
        response["notices"] = notices;
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "没有好友请求通知";
    }

    // 发送 JSON 数据到客户端
    sendJson(conn, response);
}

// 处理请求联系人列表业务
void ChatService::contactsHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = std::stoi(js["userid"].get<std::string>());

    // 查询好友列表
    std::vector<User> friends = m_friendModel.query(userId);

    // 构造返回的 JSON 数据
    json response;
    response["msgid"] = CONTACTS_MSG_ACK;

    if (!friends.empty())
    {
        response["errno"] = 0;
        std::vector<json> contacts;
        for (const auto &friendUser : friends)
        {
            json contact;
            contact["userid"] = friendUser.getId();
            contact["username"] = friendUser.getName();
            contact["status"] = friendUser.getStatus();
            contact["gender"] = friendUser.getGender();

            // 将头像文件编码为 Base64
            std::string avatarPath = "../cache/" + friendUser.getChathead();
            contact["avatar"] = encodeBase64(avatarPath);

            contacts.push_back(contact);
        }
        response["contacts"] = contacts;
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "好友列表为空";
    }

    // 发送 JSON 数据到客户端
    sendJson(conn, response);
}

// 添加好友业务
void ChatService::addFriendHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int fromId = std::stoi(js["fromId"].get<std::string>());
    int toId = std::stoi(js["toId"].get<std::string>());

    // 插入好友请求
    bool success = m_friendRequestModel.insertRequest(fromId, toId);

    if (success)
    {
        // 检查对方是否在线
        {
            lock_guard<mutex> lock(m_connMutex);
            auto it = m_userConnMap.find(toId);
            if (it != m_userConnMap.end())
            {
                User user = m_userModel.query(fromId);
                // 将头像文件编码为 Base64
                std::string avatarPath = "../cache/" + user.getChathead();
                // 对方在线，发送通知
                json notify;
                notify["msgid"] = FRIEND_REQUEST_NOTIFY; // 通知消息类型
                notify["fromId"] = fromId;
                notify["requestId"] = m_friendRequestModel.getLastInsertId(); // 获取刚插入的请求 ID
                notify["fromName"] = user.getName(); //发起者的用户名
                notify["avatar"] = encodeBase64(avatarPath);//发起者的头像
                notify["timestamp"] = time.toFormattedString(); //请求时间

                sendJson(it->second, notify);  
            }
        }
    }
    else
    {
        LOG_ERROR << "好友请求插入数据库失败: fromId=" << fromId << ", toId=" << toId;
    }
}

// 好友验证业务
void ChatService::friendVerify(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int requestId = js["requestId"].get<int>();
    bool opinion = js["opinion"].get<bool>();

    LOG_INFO << requestId << opinion ;

    // 查询好友请求信息
    FriendRequest request = m_friendRequestModel.queryRequestById(requestId);

    if (opinion)
    {
        // 同意好友请求，确认双方好友关系
        bool success = m_friendModel.insert(request.fromId, request.toId);
        if (success)
        {
            // 更新好友请求表状态为 accepted
            m_friendRequestModel.updateRequestStatus(requestId, "accepted");
        }

    }
    else
    {
        // 拒绝好友请求，更新状态为 rejected
        m_friendRequestModel.updateRequestStatus(requestId, "rejected");

    }

}

// 一对一聊天业务
void ChatService::oneChatHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int fromId = std::stoi(js["fromid"].get<std::string>());
    int toId = std::stoi(js["toid"].get<std::string>());
    std::string type = js["type"].get<std::string>();
    std::string content = js["content"].get<std::string>();

    bool isRead = false; // 默认消息为未读

    {
        lock_guard<mutex> lock(m_connMutex);
        auto it = m_userConnMap.find(toId);
        if (it != m_userConnMap.end())
        {
            //如果是图片则需发送base64编码
            if(type == "image")
            {
                json response = js;
                
                // 将头像文件编码为 Base64
                std::string filePath = "../cache/" + content;
                response["content"] = encodeBase64(filePath);
                sendJson(it->second, response);
            }
            else if(type == "text")
            {            
                // 接收方在线，直接发送消息
                sendJson(it->second, js);
            }
            else if(type == "file")
            {
                json response;
                response["msgid"] = ONE_CHAT_MSG;
                response["fromid"] = fromId;
                response["toid"] = toId;
                response["type"] = type;
                response["content"] = content;

                sendJson(it->second, response);
            }
            // 消息状态设置为已读
            isRead = true;
        }
    }

    // 存储消息到数据库，设置 is_read 状态
    m_messageModel.insertMessage(fromId, toId, type, content, isRead);

}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupesc"];

    // 存储新创建的群组消息
    Group group(-1, name, desc);
    if (m_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        m_groupModel.addGroup(userId, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();
    m_groupModel.addGroup(userId, groupId, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();
    std::vector<int> userIdVec = m_groupModel.queryGroupUsers(userId, groupId);

    lock_guard<mutex> lock(m_connMutex);
    for (int id : userIdVec)
    {
        auto it = m_userConnMap.find(id);
        if (it != m_userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线
            User user = m_userModel.query(id);
            if (user.getStatus() == "online")
            {
                // 向群组成员publish信息
                m_redis.publish(id, js.dump());
            }
            else
            {
                //转储离线消息
                m_offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

/**
 * 登录业务
 * 从json得到用户id
 * 从数据中获取此id的用户，判断此用户的密码是否等于json获取到的密码
 * 判断用户是否重复登录
 * {"msgid":1,"id":13,"password":"123456"}
 * {"errmsg":"this account is using, input another!","errno":2,"msgid":2}
 */
void ChatService::loginHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_DEBUG << "do login service!";

    int id = std::stoi(js["userid"].get<std::string>());
    std::string password = js["password"].get<std::string>();

    User user = m_userModel.query(id);
    if (user.getId() == id && user.getPassword() == password)
    {
        if (user.getStatus() == "online")
        {
            // 该用户已经登录，不能重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            sendJson(conn, response);
        }
        else
        {
            // 登录成功，记录用户连接信息
            // 需要考虑线程安全问题 onMessage会在不同线程中被调用
            {
                lock_guard<mutex> lock(m_connMutex);
                m_userConnMap.insert({id, conn});
            }

            // id用户登录成功后，向redis订阅channel(id)
            // m_redis.subscribe(id); 

            // 登录成功，更新用户状态信息 state offline => online
            user.setStatus("online");
            m_userModel.updateStatus(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["userid"] = user.getId();
            response["username"] = user.getName();

            // 查询该用户是否有离线消息
            /*std::vector<std::string> vec = m_offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，将该用户离线消息删除掉
                m_offlineMsgModel.remove(id);
            }
            else
            {
                LOG_INFO << "无离线消息";
            }*/

            /*std::vector<User> userVec = m_friendModel.query(id);
            if (!userVec.empty())
            {
                std::vector<std::string> vec;
                for (auto& user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getStatus();
                    vec.push_back(js.dump());
                }
                response["friends"] = vec;
            }*/

            sendJson(conn, response);
        }
    }else{
        // 该用户名或密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误!";
        sendJson(conn, response);
    }
    
}

// 注册业务
void ChatService::registerHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_DEBUG << "do regidster service!";

    std::string username = js["username"].get<std::string>();
    std::string password = js["password"].get<std::string>();
    std::string chathead = js["fileinfo"]["filename"].get<std::string>();
    std::string gender = js["gender"].get<std::string>();
    std::string question = js["question"].get<std::string>();
    std::string answer = js["answer"].get<std::string>();

    User user;
    user.setName(username);
    user.setPassword(password);
    user.setGender(gender);
    user.setChathead(chathead);
    user.setQuestion(question);
    user.setAnswer(answer);

    bool success = m_userModel.insert(user);

    if (success)
    {
        //文件名
        std::string fileName = user.getChathead();
        json fileInfoJson;
        fileInfoJson["filename"] = fileName;
        // 注册成功
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 0;
        response["userid"] = user.getId();
        response["chathead"] = user.getChathead();
        response["fileinfo"] = fileInfoJson;

        sendJsonWithFile(conn, response);
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 1;
        sendJson(conn, response);
    }
}

//搜索联系人
void ChatService::searchContactHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取自己的userid
    int selfId = std::stoi(js["userid"].get<std::string>());
    std::string username = js["username"].get<std::string>();

    // 查询自己的好友列表
    std::vector<User> friends = m_friendModel.query(selfId);
    std::set<int> friendIdSet;
    for (const auto& f : friends)
    {
        friendIdSet.insert(f.getId());
    }

    // 查询匹配的联系人
    std::vector<User> users = m_userModel.searchUsersByName(username);

    // 构造返回的 JSON 数据
    json response;
    response["msgid"] = SEARCH_CONTACT_ACK;

    if (!users.empty())
    {
        response["errno"] = 0;
        std::vector<json> contacts;
        for (const auto &user : users)
        {
            json contact;
            contact["userid"] = user.getId();
            contact["username"] = user.getName();

            // 头像
            std::string avatarPath = "../cache/" + user.getChathead();
            contact["avatar"] = encodeBase64(avatarPath);

            // 关系判断
            if (user.getId() == selfId)
                contact["relation"] = "self";
            else if (friendIdSet.count(user.getId()))
                contact["relation"] = "friend";
            else
                contact["relation"] = "none";

            contacts.push_back(contact);
        }
        response["contacts"] = contacts;
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "未找到匹配的联系人";
    }

    sendJson(conn, response);
}

//发送仅含有Json的数据
void ChatService::sendJson(const TcpConnectionPtr &conn, const json &response)
{
    // 序列化 JSON 数据
    std::string jsonStr = response.dump();

    // 计算消息体长度
    int len = jsonStr.size();

    // 构造消息头（4 字节，网络字节序）
    u_int32_t netLen = htonl(len); // 转换为网络字节序
    std::string header(reinterpret_cast<char *>(&netLen), sizeof(netLen));

    // 拼接消息头和消息体
    std::string data = header + jsonStr;

    // 发送完整数据包
    conn->send(data);
}

//发送带文件的Json数据
void ChatService::sendJsonWithFile(const TcpConnectionPtr &conn, json &response)
{
    std::string fileName = response["fileinfo"]["filename"].get<std::string>();
    std::string filePath = "../cache/" + fileName; 

    // 打开文件
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR << "打开文件失败: " << filePath;
        return;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    response["fileinfo"]["filesize"] = fileSize;

    // 序列化 JSON 数据
    std::string jsonStr = response.dump();

    // 构造消息头
    uint32_t jsonSize = htonl(jsonStr.size()); // 转换为网络字节序
    std::string header(reinterpret_cast<char *>(&jsonSize), sizeof(jsonSize));

    LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << jsonStr;

    // 发送消息头和 JSON 数据
    conn->send(header);
    conn->send(jsonStr);

    // 分块发送文件内容（直接利用文件流的缓冲区）
    conn->send(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));    

    file.close();
    LOG_INFO << "文件: " << fileName << " 发送成功!";
}

// base64编码
std::string ChatService::encodeBase64(const std::string &filePath)
{
    // 打开文件
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        LOG_INFO << "无法打开文件: " << filePath;
        return "";
    }

    // LOG_INFO << "打开文件成功:" << filePath;
    // 读取文件内容
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string fileContent = oss.str();

    // 使用 Boost 进行 Base64 编码
    std::string base64Encoded;
    base64Encoded.resize(boost::beast::detail::base64::encoded_size(fileContent.size()));
    boost::beast::detail::base64::encode(base64Encoded.data(), fileContent.data(), fileContent.size());

    // LOG_INFO << "文件内容大小: " << fileContent.size();
    // LOG_INFO << "Base64 编码大小: " << base64Encoded.size();

    file.close();

    return base64Encoded;
}
