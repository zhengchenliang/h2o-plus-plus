# h2o++ 🚀

*A modern, high-performance C++ HTTP/HTTPS server library built on H2O and libuv*

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![HTTP/2](https://img.shields.io/badge/HTTP-2.0-green.svg)](https://http2.github.io/)
[![TLS 1.3](https://img.shields.io/badge/TLS-1.3-orange.svg)](https://tools.ietf.org/html/rfc8446)
[![License](https://img.shields.io/badge/license-MIT-red.svg)](LICENSE)

## 🌟 Overview

h2o++ is a cutting-edge C++ HTTP/HTTPS server library that combines the power of [H2O](https://h2o.examp1e.net/) (the fastest HTTP/2 server) with the elegance of modern C++20. Designed for performance-critical applications, it provides a clean, intuitive API for building scalable web services and APIs.

## ✨ Key Features

- 🚀 **High Performance**: Built on H2O, one of the fastest HTTP servers available
- 🔒 **Full SSL/TLS Support**: TLS 1.3, TLS 1.2 with modern cipher suites
- 🏗️ **Modern C++20**: Clean, type-safe API with RAII and smart pointers
- 🌐 **HTTP/2 & HTTP/1.1**: Automatic protocol negotiation and fallback
- 🔄 **Async I/O**: Powered by libuv for efficient event-driven operations
- 📋 **RESTful Routing**: Intuitive route registration with method-specific handlers
- 📦 **JSON Integration**: Built-in JSON support with nlohmann/json
- 🛡️ **Thread-Safe**: Designed for concurrent operations and multi-threading
- 🎯 **Zero-Copy**: Efficient memory management with minimal allocations

## 🏗️ Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Application   │    │     h2o++       │    │      H2O        │
│     Layer       │◄──►│   C++ API       │◄──►│   HTTP Server   │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                ▲
                                │
                       ┌─────────────────┐
                       │     libuv       │
                       │  Event Loop     │
                       └─────────────────┘
```

## 🚀 Quick Start

### Basic HTTP Server

```cpp
#include "f200restfunc0v1.hh"

int main() {
    http_a app;  // Create HTTP application

    // Register routes
    app.get_("/hello", [](const http_q& req, http_s& res) {
        res.status_(200);
        res.send_text_("Hello, World!");
    });

    app.listen_("0.0.0.0", 8080);  // Listen on port 8080
    app.start_();                   // Start server (non-blocking)

    // Server runs until interrupted (Ctrl + C)
    while (app.state.load() == 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
```

### HTTPS Server with SSL

```cpp
#include "f200restfunc0v1.hh"

int main() {
    http_a app;

    // Setup SSL/TLS
    app.ssl_("server.crt", "server.key");

    // Register routes
    app.get_("/secure", [](const http_q& req, http_s& res) {
        res.status_(200);
        res.send_json_(R"({"message": "Secure connection!", "tls": true})");
    });

    app.listen_("0.0.0.0", 8443);
    app.start_();

    return 0;
}
```

### REST API Example

```cpp
#include "f200restfunc0v1.hh"

int main() {
    http_a app;

    // GET endpoint
    app.get_("/api/users", [](const http_q& req, http_s& res) {
        res.status_(200);
        res.send_json_(R"({"users": [{"id": 1, "name": "Alice"}]})");
    });

    // POST endpoint
    app.post_("/api/users", [](const http_q& req, http_s& res) {
        res.status_(201);
        res.send_json_(R"({"message": "User created", "id": 2})");
    });

    // PUT endpoint
    app.put_("/api/users/{id}", [](const http_q& req, http_s& res) {
        res.status_(200);
        res.send_json_(R"({"message": "User updated"})");
    });

    // DELETE endpoint
    app.delete_("/api/users/{id}", [](const http_q& req, http_s& res) {
        res.status_(204);
        res.send_("");  // No content
    });

    app.listen_("0.0.0.0", 8080);
    app.start_();

    return 0;
}
```

## 📚 API Reference

### Core Classes

#### `http_a` - HTTP Application
The main server class that manages the HTTP application lifecycle.

```cpp
class http_a {
public:
    std::atomic<uint8_t> state;  // Server state (0=finalized, 1=initialized, 2=serving, 3=stopped)

    // Lifecycle methods
    void ssl_(const std::string& cert_file, const std::string& key_file);
    void listen_(const std::string& host = "0.0.0.0", uint16_t port = 8080);
    void signal_();              // Setup signal handlers
    void start_();               // Start server in background thread
    void serve_();               // Start server in current thread
    void stop_();                // Stop server gracefully
    int64_t uptime_() const;     // Get server uptime in milliseconds

    // Route registration
    void get_(std::string prefix, http_f handler);
    void post_(std::string prefix, http_f handler);
    void put_(std::string prefix, http_f handler);
    void delete_(std::string prefix, http_f handler);
    void patch_(std::string prefix, http_f handler);
    void options_(std::string prefix, http_f handler);
    void head_(std::string prefix, http_f handler);
    void trace_(std::string prefix, http_f handler);
    void connect_(std::string prefix, http_f handler);
};
```

#### `http_q` - HTTP Request
Represents an incoming HTTP request with parsed URL components.

```cpp
struct http_q {
    h2o_req_t* h2o_request;      // Raw H2O request (advanced usage)

    // Parsed URL components
    std::string_view url;        // Full URL: "/api/v1/users?limit=10"
    std::string_view url_normal; // Normalized URL: "/api/v1/users"
    std::string_view url_prefix; // Route prefix: "/api/v1"
    std::string_view url_rest;   // Rest of path: "/users"
    std::string_view url_query;  // Query string: "?limit=10"
};
```

#### `http_s` - HTTP Response
Handles HTTP response construction and sending.

```cpp
struct http_s {
    h2o_req_t* h2o_request;      // Raw H2O request (advanced usage)

    // Response methods
    void status_(int code, const std::string& reason = "");  // Set status: 200, 404, etc.
    void head_(const std::string& name, const std::string& value);  // Add header
    void head_type_(const std::string& content_type);  // Set Content-Type
    void send_(const std::string& body);               // Send generic response
    void send_text_(const std::string& text);          // Send plain text
    void send_html_(const std::string& html);          // Send HTML
    void send_json_(const std::string& json);          // Send JSON string
    void send_json_(const nlohmann::json& json);       // Send JSON object
};
```

#### `http_m` - HTTP Methods
Type-safe HTTP method enumeration.

```cpp
struct http_m {
    enum value : uint8_t {
        NONE = 0, GET = 1, POST = 2, PUT = 3, DELETE = 4,
        PATCH = 5, OPTIONS = 6, HEAD = 7, TRACE = 8, CONNECT = 9
    };

    constexpr http_m(value v) noexcept : v(v) {}
    constexpr http_m(const char* str) noexcept;  // Parse from string
    constexpr operator value() const noexcept { return v; }
};
```

## 🧪 Testing

The library includes comprehensive tests demonstrating various features:

```bash
# Run basic HTTP test
cd /root/G1/new2 && ./A200/a200httptest0v1

# Run SSL HTTPS test
cd /root/G1/new2 && ./A200/a200httptest1v1
```

Test with curl:
```bash
# HTTP test
curl http://127.0.0.1:8080/hello

# HTTPS test (skip cert verification for self-signed)
curl -k https://127.0.0.1:8443/hello
```

## 🔧 Build System

h2o++ uses a custom build system with the following dependencies:

### Required Dependencies
- **H2O**: High-performance HTTP server library
- **libuv**: Cross-platform asynchronous I/O
- **OpenSSL**: SSL/TLS encryption support
- **nlohmann/json**: JSON parsing and serialization
- **Brotli**: Compression support (optional)
- **Zlib**: Compression support

### Build Commands

```bash
# Build REST function library
fps a3func2build.sh m200.fps:0

# Build HTTP test
fps a3func2build.sh m200.fps:5

# Build all HTTP tests
fps a3func2build.sh m200.fps:5,6
```

## 🎯 Performance

h2o++ inherits H2O's exceptional performance characteristics:

- **HTTP/2**: Multiplexed connections with header compression
- **Zero-Copy**: Minimal memory allocations and copies
- **Event-Driven**: Efficient libuv event loop
- **Connection Reuse**: Keep-alive connections
- **Async I/O**: Non-blocking operations throughout

## 🛡️ Security

- **TLS 1.3**: Latest TLS version with forward secrecy
- **Modern Ciphers**: Only secure cipher suites enabled
- **Certificate Validation**: Proper certificate chain validation
- **Secure Defaults**: Conservative security settings

## 📈 Use Cases

- **REST APIs**: High-performance JSON APIs
- **Microservices**: Lightweight service communication
- **Web Applications**: Fast web server backends
- **API Gateways**: Request routing and transformation
- **Real-time Services**: WebSocket-ready architecture
- **Edge Computing**: Low-latency edge deployments

## 🤝 Contributing

We welcome contributions! Please:

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [H2O](https://h2o.examp1e.net/) - The underlying HTTP server
- [libuv](https://libuv.org/) - Cross-platform async I/O
- [OpenSSL](https://www.openssl.org/) - SSL/TLS implementation
- [nlohmann/json](https://github.com/nlohmann/json) - JSON library

## 📞 Support

For questions, issues, or contributions, please [create an issue](https://github.com/your-repo/issues) on GitHub.

---

**h2o++** - *Where performance meets elegance* ⚡
