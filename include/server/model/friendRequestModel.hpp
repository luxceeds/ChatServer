#ifndef FRIENDREQUESTMODEL_H
#define FRIENDREQUESTMODEL_H

#include "db.h"
#include <vector>
#include <string>

struct FriendRequest {
    int id;                     // 请求 ID
    int fromId;                 // 发起者 ID
    int toId;                   // 接收者 ID
    std::string status;         // 请求状态
    std::string requestTime;    // 请求时间
    std::string fromName;       // 发起者用户名
    std::string fromAvatar;     // 发起者头像路径
};

class FriendRequestModel {
public:
    // 插入好友请求
    bool insertRequest(int fromId, int toId);

    // 查询接收者的所有好友请求
    std::vector<FriendRequest> queryRequests(int toId);

    // 更新好友请求状态
    bool updateRequestStatus(int requestId, const std::string &status);

    // 取刚插入的好友请求的ID
    int getLastInsertId();

    // 根据请求id查询
    FriendRequest queryRequestById(int requestId);
};

#endif // FRIENDREQUESTMODEL_H