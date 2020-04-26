#pragma once

#include <vector>
#include <functional>

namespace KittensTransport {

using Message = std::vector<char>;
using MessageCallback = std::function<void(const Message&)>;
using ConnectionErrorCallback = std::function<void(const std::string&)>;

class IConnection {
public:
    virtual ~IConnection() = default;

    //async write
    virtual void write(const Message & data) = 0;
    
    // will read in cycle
    virtual void startRead(MessageCallback dataCb, ConnectionErrorCallback errorCb) = 0;
};

} //namespace KittsnaTransport
