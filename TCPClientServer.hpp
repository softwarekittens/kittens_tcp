#pragma once

#include <thread>

#include <boost/asio.hpp>

#include "ITransport.hpp"

namespace KittensTransport {

class TCPClientServer : public ITransport {
public:
    TCPClientServer(uint32_t threadsCount);
    ~TCPClientServer();

    TCPClientServer(const TCPClientServer&) = delete;
    TCPClientServer& operator = (const TCPClientServer&) = delete;

    void connect(const std::string & url,
        uint16_t port,
        ConnectionCallback callback,
        ErrorCallback errorCallback);

    void listen(const std::string & ip,
        uint16_t port,
        ConnectionCallback callback,
        ErrorCallback errorCallback);

private:
    struct AcceptInfo {
        std::string address;
        uint16_t port;
        
        bool operator < (const AcceptInfo& other) const;
    };

    void run();

    void terminate();

    void resolve(const std::string& url,
        const std::string& port,
        ConnectionCallback callback,
        ErrorCallback errorCallback);
    
    void onResolved(boost::system::error_code ec,
        boost::asio::ip::tcp::resolver::iterator endpoint,
        ConnectionCallback callback,
        ErrorCallback errorCallback);

    void accept(const AcceptInfo& acceptInfo,
        ConnectionCallback callback,
        ErrorCallback errorCallback);
    
    void onAccepted(boost::system::error_code ec,
        const AcceptInfo& acceptInfo,
        ConnectionCallback callback,
        ErrorCallback errorCallback);

    void onConnected(boost::system::error_code ec,
        ConnectionCallback callback,
        ErrorCallback errorCallback,
        std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    std::vector<std::thread> _workerThreads;

    boost::asio::io_service _io_context;
    boost::asio::ip::tcp::resolver _resolver;
    std::map<AcceptInfo, std::unique_ptr<boost::asio::ip::tcp::acceptor>> _acceptors;

    std::atomic<bool> _canRun = true;
    std::atomic<bool> _isTerminated = false;

    boost::asio::ip::tcp::socket _listenSocket;
};

} // namespace KittensTransport
