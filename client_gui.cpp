#include "client_gui.h"

#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaType>
#include <QLabel>
#include <iostream>

// Register std::string as a meta-type
Q_DECLARE_METATYPE(std::string)

// ChatClient implementation
ChatClient::ChatClient(QTextEdit* display, QLineEdit* input)
    : display(display), input(input), running(false), io_work(nullptr) {
    
    // Register std::string meta-type for use in signals/slots
    qRegisterMetaType<std::string>();
    
    moveToThread(&networkThread);
    QObject::connect(&networkThread, &QThread::finished, this, &QObject::deleteLater);
}

ChatClient::~ChatClient() {
    stopClient();
    if (networkThread.isRunning()) {
        networkThread.quit();
        networkThread.wait();
    }
}

bool ChatClient::connect(const std::string& host, const std::string& port) {
    try {
        // Start the network thread
        if (!networkThread.isRunning()) {
            networkThread.start();
        }
        
        // Use QMetaObject::invokeMethod to safely call startClient from the UI thread
        bool result = false;
        QMetaObject::invokeMethod(
            this, 
            "startClient", 
            Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(std::string, host),
            Q_ARG(std::string, port)
        );
        return result;
    } catch (const std::exception& e) {
        emit errorOccurred(QString("Connection error: %1").arg(e.what()));
        return false;
    }
}

bool ChatClient::startClient(const std::string& host, const std::string& port) {
    if (running) return true;
    
    try {
        io_context = std::make_unique<asio::io_context>();
        io_work = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
            io_context->get_executor());
        
        ctx = std::make_unique<asio::ssl::context>(asio::ssl::context::sslv23);
        ctx->set_default_verify_paths();
        
        ssl_stream = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(*io_context, *ctx);

        asio::ip::tcp::resolver resolver(*io_context);
        auto endpoints = resolver.resolve(host, port);

        asio::connect(ssl_stream->lowest_layer(), endpoints);
        ssl_stream->handshake(asio::ssl::stream_base::client);
        
        running = true;
        
        // Start the message reading loop
        std::thread([this]() {
            try {
                readMessages();
                io_context->run();
            } catch (const std::exception& e) {
                emit errorOccurred(QString("Network error: %1").arg(e.what()));
            }
        }).detach();
        
        return true;
    } catch (const std::exception& e) {
        emit errorOccurred(QString("Connection error: %1").arg(e.what()));
        return false;
    }
}

void ChatClient::sendMessage(const QString& message) {
    if (!running || !ssl_stream) {
        emit errorOccurred("Not connected to server");
        return;
    }
    
    try {
        std::string msg = message.toStdString() + "\n";
        asio::write(*ssl_stream, asio::buffer(msg));
        emit messageSent(message);
    } catch (const std::exception& e) {
        emit errorOccurred(QString("Send error: %1").arg(e.what()));
    }
}

void ChatClient::stopClient() {
    std::lock_guard<std::mutex> lock(mutex);
    if (running) {
        running = false;
        if (io_work) {
            io_work.reset(); // Allow io_context to exit
        }
        if (ssl_stream) {
            asio::error_code ec;
            ssl_stream->lowest_layer().close(ec);
            ssl_stream.reset();
        }
        if (io_context) {
            io_context->stop();
            io_context.reset();
        }
    }
}

void ChatClient::readMessages() {
    while (running) {
        try {
            char data[1024] = {0};
            asio::error_code ec;
            size_t len = ssl_stream->read_some(asio::buffer(data), ec);
            
            if (ec) {
                if (running) {
                    emit errorOccurred(QString("Read error: %1").arg(ec.message().c_str()));
                }
                break;
            }
            
            if (len > 0) {
                QString message = QString::fromStdString(std::string(data, len));
                emit messageReceived(message);
            }
        } catch (const std::exception& e) {
            if (running) {
                emit errorOccurred(QString("Read error: %1").arg(e.what()));
            }
            break;
        }
    }
}

// ChatWindow implementation
ChatWindow::ChatWindow(QWidget* parent, const std::string& host, const std::string& port) 
    : QWidget(parent), serverHost(host), serverPort(port) {
    
    setupUI();
    
    // Create client
    client = new ChatClient(chatBox, inputLine);
    
    // Connect signals and slots for chat functionality
    QObject::connect(sendButton, &QPushButton::clicked, this, &ChatWindow::onSendClicked);
    QObject::connect(inputLine, &QLineEdit::returnPressed, this, &ChatWindow::onSendClicked);
    
    QObject::connect(client, &ChatClient::messageReceived, this, &ChatWindow::onMessageReceived);
    QObject::connect(client, &ChatClient::messageSent, this, &ChatWindow::onMessageSent);
    QObject::connect(client, &ChatClient::errorOccurred, this, &ChatWindow::onErrorOccurred);
    
    // Set initial values in connection fields
    hostInput->setText(QString::fromStdString(serverHost));
    portInput->setText(QString::fromStdString(serverPort));
    
    // Connect to server
    reconnect(serverHost, serverPort);
    
    setWindowTitle("Chat Client");
    resize(450, 600);
}

void ChatWindow::setupUI() {
    // Create main layout
    layout = new QVBoxLayout(this);
    
    // Create connection configuration section
    QHBoxLayout* connectionLayout = new QHBoxLayout();
    
    QLabel* hostLabel = new QLabel("Host:", this);
    hostInput = new QLineEdit(this);
    hostInput->setPlaceholderText("127.0.0.1");
    
    QLabel* portLabel = new QLabel("Port:", this);
    portInput = new QLineEdit(this);
    portInput->setPlaceholderText("12345");
    portInput->setMaximumWidth(80);
    
    connectButton = new QPushButton("Connect", this);
    
    connectionLayout->addWidget(hostLabel);
    connectionLayout->addWidget(hostInput);
    connectionLayout->addWidget(portLabel);
    connectionLayout->addWidget(portInput);
    connectionLayout->addWidget(connectButton);
    
    // Chat area
    chatBox = new QTextEdit(this);
    chatBox->setReadOnly(true);
    
    // Message input area
    QHBoxLayout* messageLayout = new QHBoxLayout();
    inputLine = new QLineEdit(this);
    sendButton = new QPushButton("Send", this);
    
    messageLayout->addWidget(inputLine);
    messageLayout->addWidget(sendButton);
    
    // Add all components to main layout
    layout->addLayout(connectionLayout);
    layout->addWidget(chatBox, 1);
    layout->addLayout(messageLayout);
    
    // Connect connection button
    QObject::connect(connectButton, &QPushButton::clicked, this, &ChatWindow::onConnectClicked);
    QObject::connect(hostInput, &QLineEdit::returnPressed, this, &ChatWindow::onConnectClicked);
    QObject::connect(portInput, &QLineEdit::returnPressed, this, &ChatWindow::onConnectClicked);
}

bool ChatWindow::reconnect(const std::string& host, const std::string& port) {
    // Update stored values
    serverHost = host;
    serverPort = port;
    
    // Update UI fields
    hostInput->setText(QString::fromStdString(host));
    portInput->setText(QString::fromStdString(port));
    
    chatBox->append(QString("Connecting to %1:%2...")
                    .arg(QString::fromStdString(host))
                    .arg(QString::fromStdString(port)));
    
    // Try to connect
    if (!client->connect(host, port)) {
        QMessageBox::warning(
            this, 
            "Connection Error", 
            QString("Could not connect to server at %1:%2. Chat functionality may be limited.")
                .arg(QString::fromStdString(host))
                .arg(QString::fromStdString(port))
        );
        return false;
    }
    
    chatBox->append("Connected successfully.");
    return true;
}

void ChatWindow::onConnectClicked() {
    std::string host = hostInput->text().trimmed().toStdString();
    std::string port = portInput->text().trimmed().toStdString();
    
    // Use defaults if fields are empty
    if (host.empty()) {
        host = "127.0.0.1";
        hostInput->setText(QString::fromStdString(host));
    }
    
    if (port.empty()) {
        port = "12345";
        portInput->setText(QString::fromStdString(port));
    }
    
    // Disconnect current connection first (if any)
    client->stopClient();
    
    // Try to connect with new parameters
    reconnect(host, port);
}

ChatWindow::~ChatWindow() {
    delete client;
}

void ChatWindow::onSendClicked() {
    QString text = inputLine->text().trimmed();
    if (!text.isEmpty()) {
        client->sendMessage(text);
        inputLine->clear();
    }
}

void ChatWindow::onMessageReceived(const QString& message) {
    chatBox->append("Server: " + message);
}

void ChatWindow::onMessageSent(const QString& message) {
    chatBox->append("You: " + message);
}

void ChatWindow::onErrorOccurred(const QString& error) {
    chatBox->append("Error: " + error);
    QMessageBox::warning(this, "Chat Error", error);
}

// Main function
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Get command-line arguments for host/port if provided
    std::string host = "127.0.0.1";
    std::string port = "12345";
    
    if (argc > 1) {
        host = argv[1];
    }
    
    if (argc > 2) {
        port = argv[2];
    }
    
    ChatWindow window(nullptr, host, port);
    window.show();
    
    return app.exec();
}
