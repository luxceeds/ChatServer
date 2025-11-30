#include "db.h"
#include <muduo/base/Logging.h>

// 数据库配置信息

// 初始化数据库连接
MySQL::MySQL()
{
    m_conn = mysql_init(nullptr);
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (m_conn != nullptr) {
        mysql_close(m_conn);
    }
}

// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(m_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        // C和C++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文显示？
        mysql_query(m_conn, "set names utf8mb4");
    }
    else
    {
        LOG_INFO << "connect mysql failed!";
    }

    return p;
}

// 更新操作
bool MySQL::update(std::string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!" << "错误原因: " << mysql_error(m_conn);       
        return false;
    }

    return true;
}

// 查询操作
MYSQL_RES *MySQL::query(std::string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!" << "错误原因: " << mysql_error(m_conn);
        return nullptr;
    }
    
    return mysql_use_result(m_conn);
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return m_conn;
}