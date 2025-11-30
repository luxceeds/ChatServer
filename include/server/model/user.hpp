#ifndef USER_H
#define USER_H

#include <string>

class User
{
public:
    User(int id = -1, std::string name = "", 
        std::string pwd = "", std::string status = "offline")
        : m_userid(id),
          m_username(name),
          m_password(pwd),
          m_status(status)
    {}

    // 设置器
    void setId(const int &id) { m_userid = id; }
    void setName(const std::string &name) { m_username = name; }
    void setPassword(const std::string &pwd) { m_password = pwd; }
    void setStatus(const std::string &status) { m_status = status; }
    void setGender(const std::string &gender) { m_gender = gender; }
    void setChathead(const std::string &chathead) { m_chathead = chathead; }
    void setQuestion(const std::string &question) { m_question = question; }
    void setAnswer(const std::string &answer) { m_answer = answer; }

    // 获取器
    int getId() const { return m_userid; }
    std::string getName() const { return m_username; } 
    std::string getPassword() const { return m_password; } 
    std::string getStatus() const { return m_status; } 
    std::string getGender() const { return m_gender; }
    std::string getChathead() const { return m_chathead; }
    std::string getQuestion() const { return m_question; }
    std::string getAnswer() const { return m_answer; }

private:
    int m_userid;                  // 用户ID
    std::string m_username;        // 用户名
    std::string m_password;        // 用户密码
    std::string m_gender;          // 性别
    std::string m_chathead;        // 头像路径
    std::string m_question;        // 密保问题
    std::string m_answer;          // 密保答案
    std::string m_status;          // 用户状态（在线/离线）
};

#endif // USER_H