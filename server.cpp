// secure_server.cpp
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>
#include <thread>
#include <memory>

using asio::ip::tcp;

void handle_client(std::shared_ptr<asio::ssl::stream<tcp::socket>> ssl_stream) {
    try {
        ssl_stream->handshake(asio::ssl::stream_base::server);

        char data[1024];
        asio::error_code ec;

        while (true) {
            size_t length = ssl_stream->read_some(asio::buffer(data), ec);
            if (ec) break;
            std::string message(data, length);
            std::cout << "Received: " << message << std::endl;

            asio::write(*ssl_stream, asio::buffer("Echo: " + message), ec);
        }
    } catch (...) {}
}

int main() {
    asio::io_context io_context;
    asio::ssl::context ssl_context(asio::ssl::context::sslv23);
    ssl_context.set_options(asio::ssl::context::default_workarounds);
    ssl_context.use_certificate_file("server.crt", asio::ssl::context::pem);
    ssl_context.use_private_key_file("server.key", asio::ssl::context::pem);

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
    std::cout << "Secure server running...\n";

    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        auto ssl_stream = std::make_shared<asio::ssl::stream<tcp::socket>>(std::move(socket), ssl_context);
        std::thread(handle_client, ssl_stream).detach();
    }
}
