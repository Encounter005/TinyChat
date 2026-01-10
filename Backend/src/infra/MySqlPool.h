#ifndef MYSQLPOOL_H_
#define MYSQLPOOL_H_

#include <atomic>
#include <condition_variable>
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>
#include <memory>
#include <mutex>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <queue>



class MySqlPool {
public:
    MySqlPool(
        const std::string& url, const std::string& user,
        const std::string& passwd, const std::string& _schema, std::size_t size)
        : _url(url)
        , _user(user)
        , _passwd(passwd)
        , _schema(_schema)
        , _poolSize(size)
        , _stop(false) {


        for (std::size_t i = 0; i < _poolSize; ++i) {
            sql::mysql::MySQL_Driver* driver
                = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> connection(
                driver->connect(_url, _user, _passwd));
            connection->setSchema(_schema);
            _pool.push(std::move(connection));
        }
    }
    std::unique_ptr<sql::Connection> getConnection() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this]() {
            if (_stop) return true;
            return !_pool.empty();
        });

        if (_stop) {
            return nullptr;
        }

        std::unique_ptr<sql::Connection> connection(std::move(_pool.front()));
        _pool.pop();
        return connection;
    }

    void returnConnection(std::unique_ptr<sql::Connection> connection) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_stop) return;
        _pool.push(std::move(connection));
        _cond.notify_one();
    }
    void Close() {
        _stop.store(true);
        _cond.notify_all();
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_pool.empty()) {
            _pool.pop();
        }
    }
    ~MySqlPool() {
        if (!_stop.load()) Close();
    }

private:
    std::string                                  _url;
    std::string                                  _user;
    std::string                                  _passwd;
    std::string                                  _schema;
    std::size_t                                  _poolSize;
    std::queue<std::unique_ptr<sql::Connection>> _pool;
    std::mutex                                   _mutex;
    std::condition_variable                      _cond;
    std::atomic<bool>                            _stop;
};

#endif   // MYSQLPOOL_H_
