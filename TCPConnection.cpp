#include "TCPConnection.hpp"
#include <kitten_logger/logger.h>

#include <chrono>
#include <thread>

namespace KittensTransport {

constexpr uint64_t g_chunkSize = 1 * 1024 * 1024;

TCPConnection::TCPConnection( boost::asio::ip::tcp::socket && s)
    : _socket(std::move(s))
{
}

void TCPConnection::initKeepAlive() {
    boost::asio::socket_base::keep_alive option(true);
    _socket.set_option(option);
}

TCPConnection::~TCPConnection()
{
    close();
}

void TCPConnection::close()
{
    boost::system::error_code ec;
   _socket.close(ec);
}

void TCPConnection::startRead(MessageCallback dataCb, ConnectionErrorCallback errorCb)
{
    _callback = dataCb;
    _errorCallback = errorCb;
    boost::asio::async_read(_socket,
        boost::asio::buffer(&_dataSize, sizeof(_dataSize)),
        boost::asio::transfer_all(),
        std::bind(&TCPConnection::onReadSize,
            this,
            std::placeholders::_1,
            std::placeholders::_2
        )
    );
}

void TCPConnection::write(const Message & _data)
{
    _written = false;
    writeImpl(_data);

    //kostil for boost 1.65
    while(_written == false) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    }
}

void TCPConnection::onReadSize(boost::system::error_code ec, std::size_t sz)
{
    if(ec) {
        auto eofEC = boost::asio::error::eof;
        auto abortedEC = boost::asio::error::operation_aborted;
        bool disconnected = (ec == eofEC || ec == abortedEC);
        LOGE << "error on read: " << (disconnected ? "disconnected" : ec.message());
    } else {
        Message().swap(_data);
        _data.resize(_dataSize);
        boost::asio::async_read(_socket,
            boost::asio::buffer(_data),
            boost::asio::transfer_all(),
            std::bind(&TCPConnection::onReadData,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}

void TCPConnection::onReadData(boost::system::error_code ec, std::size_t sz)
{
    if(ec) {
        LOGE << "error on read: " << ec.message();
    } else {
        if(_callback) {
            _callback(_data);
        } else {
            LOGE << "connection: on read: callback on set";
        }
        _data.clear();
        startRead(_callback, _errorCallback);
    }
}

void TCPConnection::writeImpl(const Message& data)
{
    std::vector<boost::asio::const_buffer> buffers;
    std::uint64_t dataSize = data.size();

    buffers.push_back(boost::asio::buffer(&dataSize, sizeof(dataSize)));
    buffers.push_back(boost::asio::buffer(data));

    boost::asio::async_write(_socket,
        buffers,
        boost::asio::transfer_all(),
        std::bind(&TCPConnection::onDataWritten,
            this,
            std::placeholders::_1,
            std::placeholders::_2
        )
    );
}

void TCPConnection::onDataWritten(boost::system::error_code ec, std::size_t sz)
{
    if(ec) {
        LOGE << "write failed: " << ec << " : " << ec.message();
    }
    _written = true;
}

boost::asio::ip::tcp::socket& TCPConnection::getSocket() {
    return _socket;
}

} // namespace KittensTransport
