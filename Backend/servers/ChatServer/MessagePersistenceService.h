#ifndef MESSAGEPERSISTENCESERVICE_H_
#define MESSAGEPERSISTENCESERVICE_H_

#include <boost/asio.hpp>
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

class MessagePersistenceService {
public:
    explicit MessagePersistenceService(boost::asio::io_context& io_context, int interval_seconds = 5);
    ~MessagePersistenceService();

    void Start();
    void Stop();
    void ForcePersistOnce();
private:
    void PersistMessages();
    void DoPersistMessages();

    boost::asio::steady_timer _timer;
    boost::asio::io_context& _context;

    int _interval_seconds;
    std::atomic<bool> _is_running;

    // 每次批量处理的消息数量
    static const int BATCH_SIZE = 10;
};


#endif // MESSAGEPERSISTENCESERVICE_H_
