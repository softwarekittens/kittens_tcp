#include <string>
#include "IConnection.hpp"

namespace KittensTransport {

using ConnectionCallback = std::function<void(std::unique_ptr<IConnection>&&)>;
using ErrorCallback = std::function<void(const std::string&)>;

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual void connect(const std::string & url,
        uint16_t port,
        ConnectionCallback callback,
        ErrorCallback errorCallback) = 0;

    virtual void listen(const std::string & ip,
        uint16_t port,
        ConnectionCallback callback,
        ErrorCallback errorCallback) = 0;
};

} // namespace KittensTransport