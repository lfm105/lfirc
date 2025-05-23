#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include <optional>
#include <cstdint>
#include <iostream>
#include <expected>

struct Error {
    std::string message;
};

template<typename T>
std::unexpected<T> Err(T&& t) {
    return std::unexpected<T>{std::forward<T>(t)};
}

std::ostream& operator<<(std::ostream& os, const Error& error) {
    os << "Error: " << error.message;
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::nullopt_t&) {
    os << "null";
    return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::expected<T, Error>& result) {
    if (result.has_value()) {
        os << result.value();
    } else {
        os << result.error();
    }
    return os;
}

template<typename T>
struct std::formatter<std::expected<T,Error>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const std::expected<T, Error>& expected, std::format_context& ctx) {
        auto out = ctx.out();
        if (expected.has_value()) {
            out = std::format_to(out, "{}", expected.value());
        } else {
            out = std::format_to(out, "{}", expected.error());
        }
        return out;
    }
};

class ServerConnection {
    public:
    ServerConnection(): sock_{-1} {}

    ~ServerConnection() {
        if (sock_ != -1) {
            close(sock_);
        }
    }

    std::expected<std::nullopt_t, Error> Connect(std::string ip_address, std::uint16_t port) {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ == -1) {
            return Err(Error{"socket"});
        }

        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip_address.c_str());

        if (connect(sock_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            return Err(Error{"connect"});
        }

        return std::nullopt;
    }

    std::expected<std::uint32_t, Error> Send(std::string message) {
        auto bytes_sent = send(sock_, message.c_str(), message.size(), 0);
        if (bytes_sent == -1) {
            return Err(Error{"send"});
        }
        return bytes_sent;
    }

    private:
        int sock_;
};

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        perror("usage <program> <nick> <username> <password>");
        return 1;
    }

    const char* nick = argv[1];
    const char* username = argv[2];
    const char* password = argv[3];

    std::ignore = nick;
    std::ignore = username;
    std::ignore = password;

    auto server_conn = ServerConnection{};

    std::ignore = server_conn.Connect("127.0.0.1", 7777).value();
    std::print(std::cout, "send: {0}\n", server_conn.Send("CAP LS 302").value());
}
