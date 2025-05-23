#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include <optional>
#include <cstdint>
#include <iostream>
#include <expected>

#include <cxxopts.hpp>

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

    std::expected<std::string, Error> Receive() {
        char buffer[1024];
        auto bytes_received = recv(sock_, buffer, sizeof(buffer), 0);
        if (bytes_received == -1) {
            return Err(Error{"recv"});
        }
        return std::string(buffer, bytes_received);
    }

    private:
        int sock_;
};

int main(int argc, char** argv)
{
    cxxopts::Options options("lfirc", "A simple IRC client");

    options.add_options()
    ("username", "Client's username", cxxopts::value<std::string>())
    ("password", "Client's password", cxxopts::value<std::string>())
    ("a,address", "Server address", cxxopts::value<std::string>())
    ("p,port", "Server port", cxxopts::value<std::uint16_t>());

    options.parse_positional({"username", "password"});

    auto args = options.parse(argc,argv);

    auto username = args["username"].as<std::string>();
    auto password = args["password"].as<std::string>();
    auto address = args["address"].as<std::string>();
    auto port = args["port"].as<std::uint16_t>();

    auto server_conn = ServerConnection{};

    std::ignore = server_conn.Connect(address.c_str(), port).value();
    std::cout << "send: " << server_conn.Send("CAP LS 302\r\n").value() << std::endl;
    std::cout << "recv: " << server_conn.Receive().value() << std::endl;
    std::cout << "send: " << server_conn.Send("PASS 1234\r\n").value() << std::endl;
    std::cout << "send: " << server_conn.Send("NICK joaj\r\n").value() << std::endl;
    std::cout << "send: " << server_conn.Send("USER joaj 0 * joaj\r\n").value() << std::endl;
    std::cout << "send: " << server_conn.Send("CAP END\r\n").value() << std::endl;
    std::cout << "recv: " << server_conn.Receive().value() << std::endl;
}
