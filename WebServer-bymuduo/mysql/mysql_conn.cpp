#include "mysql_conn.h"

int mysql_conn::addRecord(const std::string & q)
{
    return mysql_query(_conn_ , q.data());
}

int mysql_conn::delRecord(const std::string & q)
{
    return mysql_query(_conn_ , q.data());
}

void mysql_conn::getresult(std::vector<std::vector<std::string>> & res)
{
    _RES_ = mysql_store_result(_conn_);
    int rows = mysql_num_rows(_RES_);
    int col = mysql_num_fields(_RES_);
    res.reserve(rows);
    while(MYSQL_ROW ROW = mysql_fetch_row(_RES_))
    {
        std::vector<std::string>element(col);
        for(int i = 0; i < col; ++i)
        {
            element.emplace_back(std::string(ROW[i]));
        }
        res.emplace_back(element);
    }
    free_result();
}

int mysql_conn::seleRecord(const std::string & q)
{ 
    assert(_conn_ != nullptr);
    return mysql_query(_conn_ , q.data());
}

int mysql_conn::updateRecord(const std::string & q)
{
    return mysql_query(_conn_ , q.data());
}

void mysql_conn::clearResult()
{
    _RES_ = mysql_store_result(_conn_);
    free_result();
}

bool mysql_conn::isempty()
{
    _RES_ = mysql_store_result(_conn_);
    size_t rows = mysql_num_rows(_RES_);
    free_result();
    return rows == 0;
}
