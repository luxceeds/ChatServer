#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"
#include <vector>
#include <string>

class UserModel
{
public:
    // User表的插入方法
    bool insert(User &user);

    // 根据用户号码查询用户信息
    User query(int id);

    // 更新用户的状态信息
    bool updateStatus(User user);

    // 重置用户的状态信息
    void resetStatus();

    // 更新用户的其他信息（如头像、密保问题等）
    bool updateUserInfo(User user);

    // 根据用户名模糊查询联系人
    std::vector<User> searchUsersByName(const std::string &username);
};

#endif // USERMODEL_H