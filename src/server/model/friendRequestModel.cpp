#include "friendRequestModel.hpp"
#include <iostream>

// 插入好友请求
bool FriendRequestModel::insertRequest(int fromId, int toId)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO friend_request (from_id, to_id) VALUES (%d, %d)", fromId, toId);

    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            return true;
        }
    }
    return false;
}

// 查询接收者的所有好友请求
std::vector<FriendRequest> FriendRequestModel::queryRequests(int toId)
{
    std::vector<FriendRequest> requests;
    char sql[1024] = {0};
    sprintf(sql, 
        "SELECT fr.id, fr.from_id, fr.to_id, fr.status, fr.request_time, u.username, u.chathead "
        "FROM friend_request AS fr "
        "JOIN user AS u ON fr.from_id = u.userid "
        "WHERE fr.to_id = %d AND fr.status = 'pending'", 
        toId);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                FriendRequest request;
                request.id = atoi(row[0]);               // 请求 ID
                request.fromId = atoi(row[1]);           // 发起者 ID
                request.toId = atoi(row[2]);             // 接收者 ID
                request.status = row[3];                 // 请求状态
                request.requestTime = row[4];            // 请求时间
                request.fromName = row[5];               // 发起者用户名
                request.fromAvatar = row[6];             // 发起者头像路径
                requests.push_back(request);
            }
            mysql_free_result(res);
        }
    }
    return requests;
}

// 更新好友请求状态
bool FriendRequestModel::updateRequestStatus(int requestId, const std::string &status)
{
    char sql[1024] = {0};
    sprintf(sql, "UPDATE friend_request SET status = '%s' WHERE id = %d", status.c_str(), requestId);

    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            return true;
        }
    }
    return false;
}

int FriendRequestModel::getLastInsertId()
{
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query("SELECT LAST_INSERT_ID()");
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                int lastInsertId = atoi(row[0]);
                mysql_free_result(res);
                return lastInsertId;
            }
            mysql_free_result(res);
        }
    }
    return -1; // 如果获取失败，返回 -1
}

FriendRequest FriendRequestModel::queryRequestById(int requestId)
{
    FriendRequest request;
    request.id = -1; // 默认值，表示请求不存在

    char sql[1024] = {0};
    sprintf(sql, 
        "SELECT id, from_id, to_id, status, request_time "
        "FROM friend_request WHERE id = %d", 
        requestId);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                request.id = atoi(row[0]);
                request.fromId = atoi(row[1]);
                request.toId = atoi(row[2]);
                request.status = row[3];
                request.requestTime = row[4];
            }
            mysql_free_result(res);
        }
    }
    return request;
}