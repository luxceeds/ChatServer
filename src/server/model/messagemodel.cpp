#include "messagemodel.hpp"
#include "db.h"



bool MessageModel::insertMessage(int senderId, int receiverId, const std::string &type, const std::string &content, bool isRead)
{
    char sql[1024] = {0};
    sprintf(sql, 
        "INSERT INTO messages (sender_id, receiver_id, type, content, is_read) VALUES (%d, %d, '%s', '%s', %d)",
        senderId, receiverId, type.c_str(), content.c_str(), isRead ? 1 : 0);

    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}

std::vector<json> MessageModel::queryOfflineMessages(int userId)
{
    std::vector<json> messages;
    char sql[1024] = {0};
    sprintf(sql, 
        "SELECT id, sender_id, receiver_id, type, content, timestamp FROM messages "
        "WHERE receiver_id = %d AND is_read = FALSE", 
        userId);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                json message;
                message["id"] = atoi(row[0]);
                message["sender_id"] = atoi(row[1]);
                message["receiver_id"] = atoi(row[2]);
                message["type"] = row[3];
                message["content"] = row[4];
                message["timestamp"] = row[5];
                messages.push_back(message);
            }
            mysql_free_result(res);
        }
    }
    return messages;
}

std::vector<json> MessageModel::queryChatHistory(int userId, int friendId, int offset, int limit)
{
    std::vector<json> messages;
    char sql[1024] = {0};
    sprintf(sql, 
        "SELECT id, sender_id, receiver_id, type, content, timestamp FROM messages "
        "WHERE (sender_id = %d AND receiver_id = %d) OR (sender_id = %d AND receiver_id = %d) "
        "ORDER BY timestamp DESC LIMIT %d OFFSET %d", 
        userId, friendId, friendId, userId, limit, offset);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                json message;
                message["id"] = atoi(row[0]);
                message["sender_id"] = atoi(row[1]);
                message["receiver_id"] = atoi(row[2]);
                message["type"] = row[3];
                message["content"] = row[4];
                message["timestamp"] = row[5];
                messages.push_back(message);
            }
            mysql_free_result(res);
        }
    }
    return messages;
}

void MessageModel::removeOfflineMessages(int userId)
{
    char sql[1024] = {0};
    sprintf(sql, "DELETE FROM messages WHERE receiver_id = %d AND is_read = FALSE", userId);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

void MessageModel::markMessagesAsRead(const std::vector<int> &messageIds)
{
    if (messageIds.empty())
    {
        return;
    }

    // 构造 SQL 语句
    std::string sql = "UPDATE messages SET is_read = TRUE WHERE id IN (";
    for (size_t i = 0; i < messageIds.size(); ++i)
    {
        sql += std::to_string(messageIds[i]);
        if (i != messageIds.size() - 1)
        {
            sql += ",";
        }
    }
    sql += ")";

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql.c_str());
    }
}