QT += core gui widgets network

TARGET = client_gui
TEMPLATE = app

# Set C++17 standard
CONFIG += c++17

# Configure for MOC compilation
CONFIG += qt

# Sources and headers
SOURCES += \
    client_gui.cpp

HEADERS += \
    client_gui.h

# Enable OpenSSL support
DEFINES += ASIO_HAS_SSL
DEFINES += ASIO_STANDALONE
DEFINES += ASIO_HAS_STD_CHRONO

# Include paths for ASIO
# Note: You may need to adjust these paths for your specific installation
INCLUDEPATH += /opt/homebrew/include  # For Apple Silicon Macs

# OpenSSL for macOS - using Homebrew's OpenSSL installation
macx {
    # For Apple Silicon Macs
    INCLUDEPATH += /opt/homebrew/opt/openssl/include
    LIBS += -L/opt/homebrew/opt/openssl/lib -lssl -lcrypto
}

# Linux OpenSSL
linux {
    LIBS += -lssl -lcrypto
}

# Windows OpenSSL (if you're cross-compiling or planning to use on Windows)
win32 {
    # Adjust these paths to match your OpenSSL installation on Windows
    INCLUDEPATH += C:/OpenSSL-Win64/include
    LIBS += -LC:/OpenSSL-Win64/lib -llibssl -llibcrypto
}

# Additional compilation flags
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

