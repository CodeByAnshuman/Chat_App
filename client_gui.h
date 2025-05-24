#ifndef CLIENT_GUI_H
#define CLIENT_GUI_H

// Qt includes
#include <QObject>
#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QThread>
#include <QMetaMethod>

// Standard library includes
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// ASIO includes
#include <asio.hpp>
#include <asio/ssl.hpp>

class ChatClient : public QObject {
    Q_OBJECT
public:
    ChatClient(QTextEdit* display, QLineEdit* input);
    ~ChatClient();

    bool connect(const std::string& host = "127.0.0.1", const std::string& port = "12345");

public slots:
    bool startClient(const std::string& host, const std::string& port);
    void sendMessage(const QString& message);
    void stopClient();

signals:
    void messageReceived(const QString& message);
    void messageSent(const QString& message);
    void errorOccurred(const QString& error);

private:
    void readMessages();

    QTextEdit* display;
    QLineEdit* input;
    QThread networkThread;
    bool running;
    std::mutex mutex;
    
    std::unique_ptr<asio::io_context> io_context;
    std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> io_work;
    std::unique_ptr<asio::ssl::context> ctx;
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> ssl_stream;
};

class ChatWindow : public QWidget {
    Q_OBJECT
public:
    // Constructor with configurable connection parameters
    ChatWindow(QWidget* parent = nullptr, 
               const std::string& host = "127.0.0.1", 
               const std::string& port = "12345");
    ~ChatWindow();
    
    // Method to reconnect with new parameters
    bool reconnect(const std::string& host, const std::string& port);

private slots:
    void onSendClicked();
    void onMessageReceived(const QString& message);
    void onMessageSent(const QString& message);
    void onErrorOccurred(const QString& error);
    void onConnectClicked();

private:
    void setupUI();
    
    // Connection parameters
    std::string serverHost;
    std::string serverPort;
    
    // UI components
    QVBoxLayout* layout;
    QTextEdit* chatBox;
    QLineEdit* inputLine;
    QPushButton* sendButton;
    
    // Connection configuration UI
    QLineEdit* hostInput;
    QLineEdit* portInput;
    QPushButton* connectButton;
    
    // Client instance
    ChatClient* client;
};

#endif // CLIENT_GUI_H

