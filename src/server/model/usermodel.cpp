#include "usermodel.hpp"
#include "db.h"


// 插入用户信息
bool UserModel::insert(User &user)
{
    // 组装 SQL 语句
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), 
        "insert into user(username, password, status, gender, chathead, question, answer) "
        "values('%s', '%s', '%s', '%s', '%s', '%s', '%s')",
        user.getName().c_str(), 
        user.getPassword().c_str(), 
        user.getStatus().c_str(),
        user.getGender().c_str(),
        user.getChathead().c_str(),
        user.getQuestion().c_str(),
        user.getAnswer().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            user.setId(mysql_insert_id(mysql.getConnection())); // 获取插入的用户ID
            return true;
        }
    }

    return false;
}

// 根据用户号码查询用户信息
User UserModel::query(int id)
{
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "select * from user where userid = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                // 生成一个 User 对象，填入信息
                User user;
                user.setId(atoi(row[0]));          // 用户ID
                user.setName(row[1]);              // 用户名
                user.setPassword(row[2]);          // 密码
                user.setGender(row[3]);            // 性别
                user.setChathead(row[4]);          // 头像路径
                user.setQuestion(row[5]);          // 密保问题
                user.setAnswer(row[6]);            // 密保答案
                user.setStatus(row[7]);            // 状态

                mysql_free_result(res);
                return user;
            }
        }
    }
    // 返回空 User
    return User();
}

// 更新用户的状态信息
bool UserModel::updateStatus(User user)
{
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), 
        "update user set status = '%s' where userid = %d", 
        user.getStatus().c_str(), 
        user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 重置用户的状态信息
void UserModel::resetStatus()
{
    // 组装 SQL 语句
    char sql[1024] = "update user set status = 'offline' where status = 'online'";

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 更新用户的其他信息（如头像、密保问题等）
bool UserModel::updateUserInfo(User user)
{
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), 
        "update user set gender = '%s', chathead = '%s', question = '%s', answer = '%s' "
        "where userid = %d",
        user.getGender().c_str(),
        user.getChathead().c_str(),
        user.getQuestion().c_str(),
        user.getAnswer().c_str(),
        user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 根据用户名模糊查询联系人
std::vector<User> UserModel::searchUsersByName(const std::string &username)
{
    std::vector<User> users;
    char sql[1024] = {0};
    sprintf(sql, "SELECT userid, username, chathead FROM user WHERE username LIKE '%%%s%%'", username.c_str());

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
                user.setChathead(row[2]);
                users.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return users;
}