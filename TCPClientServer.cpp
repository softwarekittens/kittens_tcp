#include "TCPClientServer.hpp"
#include "TCPConnection.hpp"
#include <iostream>

#include <kitten_logger/logger.h>

namespace KittensTransport {

bool TCPClientServer::AcceptInfo::operator < (const AcceptInfo& other) const {
    return address + std::to_string(port) < other.address + std::to_string(other.port);
}

TCPClientServer::TCPClientServer(uint32_t threadsCount)
    : _resolver(_io_context)
    , _listenSocket(_io_context)
{
    for(uint32_t i = 0; i < threadsCount; ++i) {
        _workerThreads.emplace_back([this]() {
            run();
        }); 
    }
}

TCPClientServer::~TCPClientServer() {
    terminate();
    for(auto &t : _workerThreads) {
        if(t.joinable()) {
            t.join();
        }
    }
}

void TCPClientServer::terminate() {
    _canRun = false;
    _io_context.stop();
    while(_isTerminated == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void TCPClientServer::connect(const std::string & url,
        uint16_t port, 
        ConnectionCallback callback,
        ErrorCallback errorCallback)
{
    resolve(url, std::to_string(port), callback, errorCallback);
    if(_io_context.stopped()) {
        _io_context.reset();
    }
}

void TCPClientServer::listen(const std::string & ip,
        uint16_t port,
        ConnectionCallback callback,
        ErrorCallback errorCallback)
{
    accept({ip, port}, callback, errorCallback);
    if(_io_context.stopped()) {
        _io_context.reset();
    }
}

void TCPClientServer::run()
{
    _isTerminated = false;
    _canRun = true;
    while(_canRun)
    {
        try {
            if(_io_context.stopped()) {
                _io_context.reset();
            }
            _io_context.run();
        } catch (const std::exception& e) {
            std::cout << "error while running io_context: " << e.what() << std::endl;
            _io_context.stop();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    _isTerminated = true;

}

void TCPClientServer::resolve(const std::string& ip,
    const std::string& port,
    ConnectionCallback callback,
    ErrorCallback errorCallback)
{

    boost::asio::ip::tcp::resolver::query query(ip, port);
    _resolver.async_resolve(query,
        std::bind(&TCPClientServer::onResolved,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            callback,
            errorCallback
        )
    );
}
    
void TCPClientServer::onResolved(boost::system::error_code ec,
    boost::asio::ip::tcp::resolver::iterator endpoint,
    ConnectionCallback callback,
    ErrorCallback errorCallback)
{
    if(ec) {
        LOGE << "failed to resolve " << ec << " : " << ec.message();
        errorCallback(ec.message());
    } else {
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(_io_context);
        socket->async_connect(*endpoint,
            std::bind(&TCPClientServer::onConnected,
                this,
                std::placeholders::_1,
                callback,
                errorCallback,
                socket
            )
        );
    }
}

void TCPClientServer::onConnected(boost::system::error_code ec,
        ConnectionCallback callback,
        ErrorCallback errorCallback,
        std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    if(ec) {
        LOGE << "failed to connect: " << ec << " : " <<  ec.message();
        errorCallback(ec.message());
    } else {
        if(callback) {
            auto connection = std::make_unique<TCPConnection>(std::move(*socket.get()));
            callback(std::move(connection));
        } else {
            errorCallback("connection callback not set");
        }
    }
}

void TCPClientServer::accept(const AcceptInfo& acceptInfo,
    ConnectionCallback callback,
    ErrorCallback errorCallback)
{
    if(_acceptors.find(acceptInfo) == _acceptors.end()) {
        boost::asio::ip::address address = boost::asio::ip::address::from_string(acceptInfo.address);

        _acceptors[acceptInfo] = std::unique_ptr<boost::asio::ip::tcp::acceptor>(new boost::asio::ip::tcp::acceptor(_io_context, {address, acceptInfo.port}));
    }
    _acceptors[acceptInfo]->async_accept(
        _listenSocket,
        std::bind(&TCPClientServer::onAccepted,
            this,
            std::placeholders::_1,
            acceptInfo,
            callback,
            errorCallback
        )
    );
}

void TCPClientServer::onAccepted(boost::system::error_code ec,
        const AcceptInfo& acceptInfo,
        ConnectionCallback callback,
        ErrorCallback errorCallback)
{
    if(ec) {
        LOGE << "failed to accept: " << ec << " : " << ec.message();
        errorCallback(ec.message());
    } else {
        auto connection = std::make_unique<TCPConnection>(std::move(_listenSocket));
        callback(std::move(connection));
        _listenSocket = boost::asio::ip::tcp::socket(_io_context);
        accept(acceptInfo, callback, errorCallback);
    }
}

} // namespace KittensTransport
