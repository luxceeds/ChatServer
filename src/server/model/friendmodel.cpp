#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
bool FriendModel::insert(int userId, int friendId)
{
    // 组织sql语句
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "insert into friend values(%d, %d)", userId, friendId);

    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }

    return false;
}

// 返回用户好友列表
std::vector<User> FriendModel::query(int userId)
{
    std::vector<User> friends;

    char sql[1024] = {0};
    sprintf(sql, 
        "SELECT a.userid, a.username, a.status, a.gender, a.chathead "
        "FROM user a INNER JOIN friend b ON (b.userid = %d AND a.userid = b.friendid) "
        "OR (b.friendid = %d AND a.userid = b.userid)", 
        userId, userId);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setStatus(row[2]);
                user.setGender(row[3]);
                user.setChathead(row[4]);
                friends.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return friends;
}
