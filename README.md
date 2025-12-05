![](https://raw.githubusercontent.com/zhengchenliang/h2o-plus-plus/main/_a5472deplmoni0v2.png)

# http_a Server Library - Professional HTTP Server Framework

## Overview

`http_a` is a high-performance, asynchronous HTTP server library built on h2o and libuv, designed for enterprise-grade applications requiring robust HTTP request handling, command execution, and real-time processing capabilities.

## Key Features

- **High-Performance Async I/O**: Built on h2o HTTP server and libuv event loop
- **Thread-Safe Request Processing**: Dedicated thread pool for business logic
- **Built-in Command Execution**: Secure process execution with timeout and capture controls
- **Automatic Compression**: gzip, deflate, and brotli support
- **JSON Response Handling**: Native JSON serialization and parsing
- **SSL/TLS Support**: Full HTTPS capability with custom certificates
- **Timeout Management**: Configurable request and execution timeouts
- **Security**: wordexp-based argument parsing prevents shell injection

## Quick Start

### Server Initialization

```cpp

int main() {
    http_a app;

    // Optional: Enable SSL
    // app.ssl_("/path/to/cert.pem", "/path/to/key.pem");

    // Bind to address and port
    app.listen_("127.0.0.1", 8080);

    // Optional: Signal handling for graceful shutdown
    app.signal_();

    // Register endpoints (see below)
    // ...

    // Start server (blocking)
    app.serve_();

    // Alternative: Start in background thread
    // app.start_();
    // then do anything to block until shutdown

    return 0;
}
```

### Endpoint Registration

#### Synchronous Endpoints

```cpp
app.get_("/api/health", [](const http_q& q, http_s& s) {
    s.status_(200);
    s.send_json_(nlohmann::json{
        {"status", "healthy"},
        {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
    });
}, false); // false = sync
```

#### Asynchronous Endpoints

```cpp
app.post_("/api/process", [](const http_q& q, http_s& s) {
    // Access request data
    std::string user_id = q.query_("user_id");
    std::string data = q.body_();

    // Perform async business logic
    // Processing happens in thread pool automatically

    // Send response
    s.status_(200);
    s.send_json_(nlohmann::json{
        {"result", "processed"},
        {"user_id", user_id}
    });
}, true); // true = async (default)
```

## Request Handling

### Request Object (`http_q`)

```cpp
app.post_("/api/data", [](const http_q& q, http_s& s) {
    // URL and path information
    std::string full_url = q.url_();
    std::string method = q.url_prefix_();
    std::string rest_path = q.url_rest_();
    std::string query = q.url_query_();
    std::string rest_raw = q.rest_raw_();

    // Query parameters
    std::string param = q.query_("key");
    bool has_param = q.query_has_("key");

    // Headers
    std::string auth = q.header_("authorization");
    bool has_auth = q.header_has_("authorization");

    // Request body
    std::string body = q.body_(); // GET the dat_t either malloc or on memory pool

    // Environment expansion (for command execution)
    std::unordered_map<std::string, std::string> env
      = q.expand_("PATH=/usr/bin|HOME=/tmp");

    // EXE
    // DAT
    // DSK
    // VTX
    // RAD
    // ... (const: Write out)
});
```

### Response Object (`http_s`)

```cpp
app.get_("/api/response", [](const http_q& q, http_s& s) {
    // Status codes
    s.status_(200); // or s.status_(404, "Not Found");

    // Answer status
    s.quit_(); // Close connection after response
    s.stay_(); // Keep connection alive (HTTP/1.1 default)

    // Headers
    s.header_("Content-Type", "application/json");
    s.header_("Cache-Control", "no-cache");
    s.header_json_(); // Content-Type: application/json
    s.header_text_(); // Content-Type: text/plain
    s.header_html_(); // Content-Type: text/html

    // Fill body
    s.body_(dat_t/std::string); // fill dat_t pointed

    // Response body
    s.send_(); // send dat_t pointed
    s.send_text_("Hello World");
    s.send_json_(nlohmann::json{{"message", "success"}});
    s.send_html_("<h1>Title</h1>");

    // DAT
    // ... (Read in)
});
```

## Command Execution

### Synchronous Command Execution

```cpp
app.post_("/api/execute", [](const http_q& q, http_s& s) {
    exe_r result = q.exe_load_(
        "ls",           // command
        "-la /tmp",     // arguments
        "/tmp",         // working directory
        {},             // environment variables
        5000,           // timeout (ms)
        1,              // capture mode (1=stdout only)
        0               // sampling period (0=no sampling)
    );

    if (result.status == 3 && result.exit_code == 0) {
        s.send_text_(result.stdout_info);
    } else {
        s.status_(500);
        s.send_json_(nlohmann::json{
            {"error", "Command failed"},
            {"exit_code", result.exit_code},
            {"stderr", result.stderr_info}
        });
    }
});
```

### Fire-and-Forget Execution

```cpp
app.post_("/api/fire", [](const http_q& q, http_s& s) {
    exe_r result = q.exe_fire_(
        "backup_script.sh",
        "--full",
        "/var/data"
    );

    // Always returns 202 for fire-and-forget
    s.status_(202);
    s.send_text_("Job triggered");
});
```

### Capture Modes

- `0`: No capture (high performance)
- `1`: stdout only
- `2`: stderr only
- `3`: Both stdout and stderr
- `4`: Merged (stderr redirected to stdout)

## Advanced Features

### Custom Environment Variables

```cpp
std::unordered_map<std::string, std::string> env = {
    {"PATH", "/usr/local/bin:/usr/bin"},
    {"CUSTOM_VAR", "value"}
};

exe_r result = q.exe_run_("my_command", "args", "/tmp", env);
```

### Complex Environment Parsing

```cpp
// Environment string: "KEY1=value1|KEY2=value2"
std::string env_str = "PATH=/usr/bin|HOME=/tmp|CONFIG_FILE=/etc/app.conf";
std::unordered_map<std::string, std::string> env = q.expand_(env_str);
```

### Timeout and Sampling

```cpp
exe_r result = q.exe_run_(
    "long_running_command",
    "",
    "",
    {},
    30000,  // 30 second timeout
    1,      // capture stdout
    1000    // sample every 1 second (for monitoring)
);
```

## Security Considerations

### Input Validation

- All query parameters are URL-decoded
- Command arguments use `wordexp` for secure parsing
- No shell metacharacter injection possible

### Resource Limits

- Configurable timeouts prevent hanging processes
- Memory pooling prevents excessive allocations
- Thread pool sizing prevents resource exhaustion

## Configuration Options

### Server Configuration

```cpp
// SSL/TLS
app.ssl_("/path/to/cert.pem", "/path/to/key.pem");

// Listening
app.listen_("0.0.0.0", 8080);     // All interfaces
app.listen_("127.0.0.1", 8080);   // Localhost only

// Thread pool size (default: CPU cores * 2)
pthd.rebn_(16); // 16 worker threads
```

### Endpoint Configuration

```cpp
app.post_("/api/endpoint",
    [](const http_q& q, http_s& s) {
        // Business logic
    },
    true,   // async: true/false
    30000,  // timeout_ms: 0 = no timeout
    true,   // compress: enable compression
    1024,   // min_size: minimum size for compression
    1,      // gzip_quality: 1-9
    1       // brotli_quality: 1-11
);
```

## Production Deployment

### Graceful Shutdown

```cpp
app.signal_(); // Handle SIGINT/SIGTERM
app.serve_();  // Blocks until shutdown signal
```

### Health Checks

```cpp
app.get_("/health", [](const http_q& q, http_s& s) {
    s.status_(200);
    s.send_json_(nlohmann::json{
        {"status", "healthy"},
        {"uptime", get_uptime()},
        {"version", "1.0.0"}
    });
});
```

### Monitoring Integration

```cpp
app.get_("/metrics", [](const http_q& q, http_s& s) {
    s.send_json_(collect_metrics());
});
```

## API Reference

### Core Classes

- `http_a`: Main server class
- `http_q`: Request object
- `http_s`: Response object
- `exe_r`: Command execution result

### Key Methods

#### Server Control
- `listen_(host, port)`: Bind server to address
- `ssl_(cert, key)`: Enable HTTPS
- `signal_()`: Enable signal handling
- `start_()`: Start server in background thread
- `serve_()`: Start server (blocking)
- `stop_()`: Stop server

#### Endpoint Registration
- `get_(path, handler, async, timeout, compress, ...)`: Register GET endpoint
- `post_(path, handler, async, timeout, compress, ...)`: Register POST endpoint
- `put_(path, handler, async, timeout, compress, ...)`: Register PUT endpoint
- `delete_(path, handler, async, timeout, compress, ...)`: Register DELETE endpoint

#### Request Processing
- `exe_run_(cmd, args, dir, env, timeout, capture, period)`: Execute command
- `exe_fire_(cmd, args, dir, env)`: Execute asynchronously
- `exe_load_(cmd, args, dir, env)`: Execute synchronously with output capture

#### Response Building
- `status_(code, reason)`: Set HTTP status
- `header_(name, value)`: Add HTTP header
- `send_text_(content)`: Send text response
- `send_json_(data)`: Send JSON response
- `send_html_(content)`: Send HTML response

This framework provides enterprise-grade HTTP server capabilities with security, performance, and reliability as core design principles.

