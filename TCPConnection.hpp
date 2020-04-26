#pragma once

#include "IConnection.hpp"
#include <boost/asio.hpp>

namespace KittensTransport {

class TCPConnection : public IConnection {
public:
    TCPConnection( boost::asio::ip::tcp::socket && _socket );
    
    ~TCPConnection();

    void write(const Message & data) override;
    void startRead(MessageCallback dataCb, ConnectionErrorCallback errorCb) override;

    void close();

    void initKeepAlive();

private:
    friend class TCPClientServer;
    //boost requres to know socket on connect step
    boost::asio::ip::tcp::socket& getSocket();

    void onReadSize(boost::system::error_code ec, std::size_t sz);
    void onReadData(boost::system::error_code ec, std::size_t sz);
    
    void writeImpl(const Message& _data);
    void onDataWritten(boost::system::error_code ec, std::size_t sz);
    
    boost::asio::ip::tcp::socket _socket;
    
    MessageCallback _callback;
    ConnectionErrorCallback _errorCallback;
    uint64_t _dataSize = 0;
    uint64_t _chunksCount = 0;
    Message _data;

    // kostil
    // without this everyhing is bad
    // need to remove after migrate to new boost version
    std::atomic<bool> _written = false;

};

} // namespace KittensTransport
