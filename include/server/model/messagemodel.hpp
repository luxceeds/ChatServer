#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include <vector>
#include "json.hpp"
using json = nlohmann::json;

class MessageModel
{
public:
    // 存储消息
    bool insertMessage(int senderId, int receiverId, const std::string &type, const std::string &content, bool isRead);

    // 查询离线消息
    std::vector<json> queryOfflineMessages(int userId);

    // 查询聊天记录（分页）
    std::vector<json> queryChatHistory(int userId, int friendId, int offset, int limit);

    // 删除离线消息
    void removeOfflineMessages(int userId);

    // 标记消息为已读
    void markMessagesAsRead(const std::vector<int> &messageIds);
};

#endif