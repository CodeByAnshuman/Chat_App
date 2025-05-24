# Secure C++ Chat Application

This project implements a **secure chat application** in C++ using ASIO and OpenSSL for encrypted communication, with both a console client and an optional Qt-based GUI client.

---

## Features

- Multi-client server with SSL/TLS encryption  
- Console client for quick text chatting  
- Optional GUI client built with Qt for a graphical chat interface  
- Thread-safe handling of multiple clients on the server  
- Uses OpenSSL for security and certificate-based encryption  

---

## Requirements

- C++ compiler supporting C++11 or newer (e.g., `g++`, `clang++`)  
- Standalone ASIO library or Boost.ASIO  
- OpenSSL development libraries (e.g., `libssl-dev`)  
- Qt5 (for the GUI client)  
- `pthreads` (for multi-threading)  

---

## Setup and Build Instructions

### 1. Generate SSL Certificates (for the server)
`openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.crt`

### 2. Build the Server
`g++ secure_server.cpp -o secure_server -lssl -lcrypto -lpthread`

### 3. Build the Console Client
`g++ secure_client.cpp -o secure_client -lssl -lcrypto -lpthread`

### 4. (Optional) Build the Qt GUI Client
Make sure Qt5 is installed on your system. Example compile command (adjust for your system):

`g++ chat_gui.cpp -o chat_gui -lssl -lcrypto -lpthread -fPIC -lQt5Widgets -lQt5Core -std=c++17`

