#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>
#include <thread>

using asio::ip::tcp;

int main() {
    try {
        asio::io_context io_context;

        // SSL context: uses TLS/SSL
        asio::ssl::context ctx(asio::ssl::context::sslv23);
        ctx.set_default_verify_paths();  // Optional: Trust system CA certificates

        // SSL socket
        asio::ssl::stream<tcp::socket> ssl_socket(io_context, ctx);

        // Connect to server
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "12345");
        asio::connect(ssl_socket.lowest_layer(), endpoints);

        // Perform handshake
        ssl_socket.handshake(asio::ssl::stream_base::client);

        std::cout << "Connected securely to server!\n";

        std::thread reader([&]() {
            while (true) {
                char reply[1024];
                asio::error_code ec;
                size_t len = ssl_socket.read_some(asio::buffer(reply), ec);
                if (ec) break;
                std::cout << "Server: " << std::string(reply, len) << std::endl;
            }
        });

        while (true) {
            std::string msg;
            std::getline(std::cin, msg);
            if (msg.empty()) continue;
            asio::write(ssl_socket, asio::buffer(msg));
        }

        reader.join();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
