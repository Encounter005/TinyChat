#ifndef MESSSAGE_H_
#define MESSSAGE_H_

#include <string>
#include <cstdint>

// [ uint32_t body_len ][ uint16_t msg_id ][ body ]
constexpr static int HEADER_LEN = 4;
constexpr static int HEADER_ID = 2;
static constexpr uint32_t MAX_BODY_LEN = 64 * 1024;
struct Message {
    uint16_t msg_id;
    std::string body;
};

#endif // MESSSAGE_H_
