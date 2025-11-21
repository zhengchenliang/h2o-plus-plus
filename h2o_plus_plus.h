#ifndef F200RESTFUNC0V1_HH
#define F200RESTFUNC0V1_HH

#include <nlohmann/json.hpp>

struct http_m;
struct http_q;
struct http_s;

#include <cstdlib>
#include <cstddef>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <uv.h>
#include <h2o.h>
#include <h2o/http1.h>
#include <h2o/http2.h>

/* --------------------------------------------- */

struct http_m // method
{
  enum value : uint8_t
  {
    NONE = 0,
    GET = 1,
    POST = 2,
    PUT = 3,
    DELETE = 4,
    PATCH = 5,
    OPTIONS = 6,
    HEAD = 7,
    TRACE = 8,
    CONNECT = 9
  };
  value v;
  constexpr http_m() noexcept : v(NONE) {}
  constexpr http_m(value val) noexcept : v(val) {}
  constexpr http_m(const char* str) noexcept : v(method_(str)) {}
  http_m(const std::string& str) noexcept : v(method_(str.c_str())) {}
  constexpr operator value() const noexcept { return v; } // use as enum e.g. http_m::GET
  explicit operator bool() = delete; // ban if(http_m)
  constexpr bool operator==(http_m other) const noexcept { return v == other.v; }
  constexpr bool operator!=(http_m other) const noexcept { return v != other.v; }
  static constexpr value method_(const char* str) noexcept
  {
    if (str[0] == 'G' && str[1] == 'E' && str[2] == 'T' && str[3] == '\0') return GET;
    if (str[0] == 'P' && str[1] == 'O' && str[2] == 'S' && str[3] == 'T' && str[4] == '\0') return POST;
    if (str[0] == 'P' && str[1] == 'U' && str[2] == 'T' && str[3] == '\0') return PUT;
    if (str[0] == 'D' && str[1] == 'E' && str[2] == 'L' && str[3] == 'E' && str[4] == 'T' && str[5] == 'E' && str[6] == '\0') return DELETE;
    if (str[0] == 'P' && str[1] == 'A' && str[2] == 'T' && str[3] == 'C' && str[4] == 'H' && str[5] == '\0') return PATCH;
    if (str[0] == 'O' && str[1] == 'P' && str[2] == 'T' && str[3] == 'I' && str[4] == 'O' && str[5] == 'N' && str[6] == 'S' && str[7] == '\0') return OPTIONS;
    if (str[0] == 'H' && str[1] == 'E' && str[2] == 'A' && str[3] == 'D' && str[4] == '\0') return HEAD;
    if (str[0] == 'T' && str[1] == 'R' && str[2] == 'A' && str[3] == 'C' && str[4] == 'E' && str[5] == '\0') return TRACE;
    if (str[0] == 'C' && str[1] == 'O' && str[2] == 'N' && str[3] == 'N' && str[4] == 'E' && str[5] == 'C' && str[6] == 'T' && str[7] == '\0') return CONNECT;
    return NONE;
  }
};
namespace std
{
  template <typename T, typename = std::enable_if_t<std::is_same_v<T, http_m>>>
  inline std::string to_string(const T m)
  {
    switch (m.v)
    {
      case http_m::GET:     return "GET";
      case http_m::POST:    return "POST";
      case http_m::PUT:     return "PUT";
      case http_m::DELETE:  return "DELETE";
      case http_m::PATCH:   return "PATCH";
      case http_m::OPTIONS: return "OPTIONS";
      case http_m::HEAD:    return "HEAD";
      case http_m::TRACE:   return "TRACE";
      case http_m::CONNECT: return "CONNECT";
      default:              return "NONE";
    }
  }
  template <>
  struct hash<http_m>
  {
    std::size_t operator()(const http_m& m) const noexcept
    {
      return static_cast<std::size_t>(m.v);
    }
  };
  template <>
  struct hash<std::pair<std::string, http_m>>
  {
    std::size_t operator()(const std::pair<std::string, http_m>& p) const noexcept
    {
      std::size_t h1 = std::hash<std::string>{}(p.first);
      std::size_t h2 = static_cast<std::size_t>(p.second.v);
      return h1 ^ (h2 * 0x9e3779b97f4a7c17ULL);
    }
  };
}
template <typename T, typename = std::enable_if_t<std::is_same_v<T, http_m>>>
inline std::ostream& operator<<(std::ostream& os, const T m)
{
  return os << std::to_string(m);
}

/* --------------------------------------------- */

using http_f = std::function<void(const http_q&, http_s&)>; // functions

struct http_q // request
{
  h2o_req_t* h2o_request = NULL;
  // h2o http request fields
  std::string_view url; // "/api/v2/../v1/m5472/a5472?token=xxx"
  std::string_view url_normal; // "/api/v1/m5472/a5472"
  std::string_view url_prefix; // "/api/v1" registered
  std::string_view url_rest; // "/m5472/a5472"
  std::string_view url_query; // "?token=xxx"
  // ??? businesss logic prototype: file operation, program execution, db query
};

struct http_s // response
{
  h2o_req_t* h2o_request = NULL;
  // generic fields as business output write to req

  // h2o http response fields fixed format
  inline void status_(int _code, const std::string& _reason = "") // response status: 200, "OK"
  {
    h2o_request->res.status = _code;
    h2o_request->res.reason = _reason.empty()
      ? (_code == 200 ? "OK"
        : _code == 201 ? "Created"
        : _code == 202 ? "Accepted"
        : _code == 203 ? "Non-Authoritative Information"
        : _code == 204 ? "No Content"
        : _code == 205 ? "Reset Content"
        : _code == 206 ? "Partial Content"
        : _code == 207 ? "Multi-Status"
        : _code == 208 ? "Already Reported"
        : _code == 226 ? "IM Used"
        : _code == 300 ? "Multiple Choices"
        : _code == 301 ? "Moved Permanently"
        : _code == 302 ? "Found"
        : _code == 303 ? "See Other"
        : _code == 304 ? "Not Modified"
        : _code == 305 ? "Use Proxy"
        : _code == 306 ? "Switch Proxy"
        : _code == 307 ? "Temporary Redirect"
        : _code == 308 ? "Permanent Redirect"
        : _code == 400 ? "Bad Request"
        : _code == 401 ? "Unauthorized"
        : _code == 403 ? "Forbidden"
        : _code == 404 ? "Not Found"
        : _code == 405 ? "Method Not Allowed"
        : _code == 406 ? "Not Acceptable"
        : _code == 408 ? "Request Timeout"
        : _code == 409 ? "Conflict"
        : _code == 410 ? "Gone"
        : _code == 411 ? "Length Required"
        : _code == 412 ? "Precondition Failed"
        : _code == 500 ? "Internal Server Error"
        : _code == 501 ? "Not Implemented"
        : _code == 502 ? "Bad Gateway"
        : _code == 503 ? "Service Unavailable"
        : _code == 504 ? "Gateway Timeout"
        : _code == 505 ? "HTTP Version Not Supported"
        : _code == 506 ? "Variant Also Negotiates"
        : _code == 507 ? "Insufficient Storage"
        : _code == 508 ? "Loop Detected"
        : _code == 510 ? "Not Extended"
        : "")
      : _reason.c_str()
    ;
  }
  inline void head_(const std::string& _name, const std::string& _value) // any header: "Content-Type", "application/json"
  {
    std::string lc_name = _name;
    std::transform(lc_name.begin(), lc_name.end(), lc_name.begin(), ::tolower); // to lower case
    h2o_add_header_by_str(&h2o_request->pool
      , &h2o_request->res.headers
      , lc_name.c_str()
      , lc_name.size()
      , 1 // maybe_token: try to find token first
      , _name.c_str()
      , _value.c_str()
      , _value.size()
    );
  }
  inline void head_type_(const std::string& _ct) // content type header: "text/plain; charset=utf-8"
  {
    h2o_add_header(&h2o_request->pool
      , &h2o_request->res.headers
      , H2O_TOKEN_CONTENT_TYPE
      , NULL
      , _ct.c_str()
      , _ct.size()
    );
  }

  inline void send_(const std::string& _body) // full answer: generic
  {
    static h2o_generator_t generator = {NULL, NULL};
    if (h2o_request->res.status == 0) status_(200);
    h2o_iovec_t body = h2o_strdup(&h2o_request->pool, _body.data(), _body.size());
    h2o_start_response(h2o_request, &generator);
    h2o_send(h2o_request
      , &body
      , 1
      , H2O_SEND_STATE_FINAL
    );
  }
  inline void send_text_(const std::string& _body) // full answer: plain text utf-8
  {
    static h2o_generator_t generator = {NULL, NULL};
    if (h2o_request->res.status == 0) status_(200);
    h2o_add_header(&h2o_request->pool
      , &h2o_request->res.headers
      , H2O_TOKEN_CONTENT_TYPE
      , NULL
      , H2O_STRLIT("text/plain; charset=utf-8")
    );
    h2o_iovec_t body = h2o_strdup(&h2o_request->pool, _body.data(), _body.size());
    h2o_start_response(h2o_request, &generator);
    h2o_send(h2o_request
      , &body
      , 1
      , H2O_SEND_STATE_FINAL
    );
  }
  inline void send_html_(const std::string& _body) // full answer: html text utf-8
  {
    static h2o_generator_t generator = {NULL, NULL};
    if (h2o_request->res.status == 0) status_(200);
    h2o_add_header(&h2o_request->pool
      , &h2o_request->res.headers
      , H2O_TOKEN_CONTENT_TYPE
      , NULL
      , H2O_STRLIT("text/html; charset=utf-8")
    );
    h2o_iovec_t body = h2o_strdup(&h2o_request->pool, _body.data(), _body.size());
    h2o_start_response(h2o_request, &generator);
    h2o_send(h2o_request
      , &body
      , 1
      , H2O_SEND_STATE_FINAL
    );
  }
  inline void send_json_(const std::string& _json) // full answer: json utf-8
  {
    static h2o_generator_t generator = {NULL, NULL};
    if (h2o_request->res.status == 0) status_(200);
    h2o_add_header(&h2o_request->pool
      , &h2o_request->res.headers
      , H2O_TOKEN_CONTENT_TYPE
      , NULL
      , H2O_STRLIT("application/json; charset=utf-8")
    );
    h2o_iovec_t body = h2o_strdup(&h2o_request->pool, _json.data(), _json.size());
    h2o_start_response(h2o_request, &generator);
    h2o_send(h2o_request
      , &body
      , 1
      , H2O_SEND_STATE_FINAL
    );
  }
  inline void send_json_(const nlohmann::json& _j) { send_json_(_j.dump()); }
};

class http_h : public h2o_handler_t
{
public:
  std::string method;
  http_f business;
  static inline int on_req_(h2o_handler_t* _self, h2o_req_t* _req) // h2o callback signature
  {
    auto* handler = static_cast<http_h*>(_self);
    if (!h2o_memis(_req->method.base
      , _req->method.len
      , handler->method.data()
      , handler->method.size())
    ) return -1; // method mismatch -> h2o continue to next handler
    std::string_view url(_req->path.base, _req->path.len);
    std::string_view url_normal(_req->path_normalized.base, _req->path_normalized.len);
    std::string_view url_prefix(_req->pathconf->path.base, _req->pathconf->path.len);
    std::string_view url_rest = url_normal.substr(url_prefix.size());
    std::string_view url_query = (_req->query_at != SIZE_MAX)
      ? std::string_view(_req->path.base + _req->query_at, _req->path.len - _req->query_at)
      : std::string_view()
    ;
    http_q q{_req, url, url_normal, url_prefix, url_rest, url_query};
    http_s s{_req};
    handler->business(q, s);
    return 0;
  }
  static inline void dispose_(h2o_handler_t* _self) // h2o dispose signature: called when handler is destroyed
  {
    auto* handler = static_cast<http_h*>(_self);
    handler->~http_h();
  }
};

class http_a // http app -> app.ssl_() -> app.register_()/listen_()/signal_() -> app.start_()/thread.serve_() -> app.stop_() -> app.register_()/listen_()/signal_() -> app.start_()/thread.serve_() -> ~()
{
public:
  std::atomic<uint8_t> state;          // 0 = finalized; 1 = initialized; 2 = serving; 3 = stopped;
  std::thread server_t;                // server thread
  std::atomic<bool> server_a;          // true = thread server alive
  std::atomic<bool> server_r;          // true = server does restart
  std::mutex server_m;                 // server mutex
  std::condition_variable server_c;    // server condition variable
  uv_loop_t loop;                      // uv event loop
  uv_async_t loop_a;                   // uv async sender
  uv_timer_t loop_t;                   // uv loop timer
  std::vector<uv_tcp_t*> listeners;    // uv listeners
  std::vector<uv_signal_t*> signalers; // uv signalers
  h2o_globalconf_t gconfig;            // h2o global configuration
  h2o_hostconf_t* hconfig = NULL;      // h2o host configuration
  h2o_hostconf_t* hconfig_a[2];        // h2o hosts configuration
  h2o_context_t ctx;                   // h2o context (per-thread)
  h2o_accept_ctx_t accept_ctx;         // h2o accept context for new connections
  SSL_CTX* ssl_ctx = NULL;             // openssl ssl context
  std::chrono::steady_clock::time_point start_time; // server start time
  struct prefix_c
  {
    h2o_pathconf_t* pathconf;
    std::unordered_map<http_m, http_h*> handlers; // <method, handler>
    prefix_c() : pathconf(NULL) { handlers.reserve(10); }
    prefix_c(h2o_pathconf_t* _pathconf) : pathconf(_pathconf) { handlers.reserve(10); }
    ~prefix_c() { handlers.clear(); }
  };
  std::unordered_map<std::string, prefix_c> prefix_groups; // <prefix, prefix_c>
  http_a() { state.store(0); server_a.store(false); server_r.store(false); init_(); }
  ~http_a() { fina_(); }
  inline void init_()
  {
    if (state.load() != 0) return; // already initialized or serving or stopped
    // 1. setup uv event loop with sender and timer
    uv_loop_init(&loop); // uv event loop local
    uv_async_init(&loop, &loop_a, [](uv_async_t* handle) {});
    uv_timer_init(&loop, &loop_t);
    loop_a.data = this;
    loop_t.data = this;
    // 2. setup h2o global configuration
    h2o_config_init(&gconfig);
    // 3. setup h2o host configuration
    hconfig = h2o_config_register_host(&gconfig
      , h2o_iovec_init(H2O_STRLIT("default"))
      , 65535 // h2o http server at
    );
    // 4. initialize h2o context
    h2o_context_init(&ctx, &loop, &gconfig); // ctx.loop = loop
    // 5. setup accept context
    hconfig_a[0] = hconfig;
    hconfig_a[1] = NULL;
    accept_ctx.hosts = hconfig_a;
    accept_ctx.ctx = &ctx;
    accept_ctx.ssl_ctx = ssl_ctx;
    state.store(1);
  }
  inline void ssl_(const std::string& _cert_file, const std::string& _key_file) // setup at initialized
  {
    if (state.load() != 1) return; // finalized or serving or stopped: one port 80/443 one http(s) protocol
    if (ssl_ctx) // switch ssl cert/key
    {
      SSL_CTX_free(ssl_ctx);
      ssl_ctx = NULL;
    }
    else // for ancient openssl version
    {
      // 0. initialize openssl
      SSL_library_init();
      SSL_load_error_strings();
      OpenSSL_add_all_algorithms();
    }
    // 1. create ssl context
    ssl_ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ssl_ctx)
    {
      throw std::runtime_error("http_a.ssl_(): Failed to create SSL context w/ " + std::string(ERR_error_string(ERR_get_error(), NULL)));
    }
    // 2. load certificate
    if (SSL_CTX_use_certificate_chain_file(ssl_ctx, _cert_file.c_str()) != 1)
    {
      SSL_CTX_free(ssl_ctx);
      ssl_ctx = NULL;
      throw std::runtime_error("http_a.ssl_(): Failed to load certificate: " + _cert_file + " w/ " + std::string(ERR_error_string(ERR_get_error(), NULL)));
    }
    // 3. load private key
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, _key_file.c_str(), SSL_FILETYPE_PEM) != 1)
    {
      SSL_CTX_free(ssl_ctx);
      ssl_ctx = NULL;
      throw std::runtime_error("http_a.ssl_(): Failed to load private key: " + _key_file + " w/ " + std::string(ERR_error_string(ERR_get_error(), NULL)));
    }
    // 4. verify key matches certificate
    if (SSL_CTX_check_private_key(ssl_ctx) != 1)
    {
      SSL_CTX_free(ssl_ctx);
      ssl_ctx = NULL;
      throw std::runtime_error("http_a.ssl_(): Private key " + _key_file + " does not match certificate " + _cert_file + " w/ " + std::string(ERR_error_string(ERR_get_error(), NULL)));
    }
    // 5. set ssl options
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    // 6. update h2o accept context
    accept_ctx.ssl_ctx = ssl_ctx;
  }
  static inline void on_accept_(uv_stream_t* _listener, int _status) // uv ip:port listener callback signature: called when connection comes at its ip:port
  {
    if (_status != 0) return;
    // 1. uv create connection handle for h2o http server
    uv_tcp_t* conn = new uv_tcp_t; // uv -> h2o client(connection) tcp -> http socket
    uv_tcp_init(_listener->loop, conn);
    // 2. uv accept coming connection
    if (uv_accept(_listener, reinterpret_cast<uv_stream_t*>(conn)) != 0)
    {
      uv_close(reinterpret_cast<uv_handle_t*>(conn)
        , [](uv_handle_t* handle) { delete reinterpret_cast<uv_tcp_t*>(handle); }
      );
      return;
    }
    // 3. h2o get its accept context of that listener
    auto* acc_ctx = static_cast<h2o_accept_ctx_t*>(_listener->data);
    // 4. h2o create socket from connection handle (uv -> h2o upgrade)
    h2o_socket_t* sock = h2o_uv_socket_create(reinterpret_cast<uv_handle_t*>(conn)
      , [](uv_handle_t* handle) { delete reinterpret_cast<uv_tcp_t*>(handle); }
    );
    // 5. h2o accept connection socket
    h2o_accept(acc_ctx, sock);
  }
  inline void listen_(const std::string& _host = "0.0.0.0", uint16_t _port = 8080)
  {
    if (state.load() % 2 != 1) return; // finalized or serving
    // 1. uv create ip:port listener handle
    uv_tcp_t* listener = new uv_tcp_t;
    uv_tcp_init(&loop, listener);
    // 2. uv record h2o accept_ctx in listener
    listener->data = &accept_ctx;
    // 3. uv socket bind to ip:port
    struct sockaddr_in addr;
    uv_ip4_addr(_host.c_str(), _port, &addr);
    int r = uv_tcp_bind(listener, reinterpret_cast<struct sockaddr*>(&addr), 0); // uv ip:port socket bind
    if (r != 0)
    {
      uv_close(reinterpret_cast<uv_handle_t*>(listener)
        , [](uv_handle_t* handle) { delete reinterpret_cast<uv_tcp_t*>(handle); }
      );
      throw std::runtime_error("http_a.listen_(): Failed to bind to " + _host + ":" + std::to_string(_port) + " w/ " + uv_strerror(r));
    }
    // 4. uv start listening
    r = uv_listen(reinterpret_cast<uv_stream_t*>(listener), 128, on_accept_); // uv socket set on event loop
    if (r != 0)
    {
      uv_close(reinterpret_cast<uv_handle_t*>(listener)
        , [](uv_handle_t* handle) { delete reinterpret_cast<uv_tcp_t*>(handle); }
      );
      throw std::runtime_error("http_a.listen_(): Failed to listen on " + _host + ":" + std::to_string(_port) + " w/ " + uv_strerror(r));
    }
    // 5. uv manage listeners in STL
    listeners.push_back(listener);
  }
  inline void delisten_()
  {
    for (auto* listener : listeners)
    {
      uv_read_stop(reinterpret_cast<uv_stream_t*>(listener));
      if (uv_is_closing(reinterpret_cast<uv_handle_t*>(listener)) == 0) uv_close(reinterpret_cast<uv_handle_t*>(listener)
        , [](uv_handle_t* handle) { delete reinterpret_cast<uv_tcp_t*>(handle); }
      );
    }
    listeners.clear();
    uv_async_send(&loop_a); // sender acknoledge for last pending listener events
  }
  static inline void on_signal_(uv_signal_t* _sig, int _signum)
  {
    auto* self = static_cast<http_a*>(_sig->data);
    printf("\nhttp_a.signal_() [%d]: Caught signal %d stopping loop ...\n", getpid(), _signum);
    self->stop_(); // graceful stop on signals
  }
  inline void signal_()
  {
    if (state.load() % 2 != 1) return; // finalized or serving
    for (int signum : {SIGINT}) // SIGTERM
    {
      uv_signal_t* signaler = new uv_signal_t;
      int r = uv_signal_init(&loop, signaler);
      if (r != 0)
      {
        delete signaler;
        continue;
      }
      signaler->data = this;
      r = uv_signal_start(signaler, on_signal_, signum);
      if (r != 0)
      {
        uv_close(reinterpret_cast<uv_handle_t*>(signaler)
          , [](uv_handle_t* handle) { delete reinterpret_cast<uv_signal_t*>(handle); }
        );
        continue;
      }
      // uv manage signalers in STL
      signalers.push_back(signaler);
    }
  }
  inline void designal_()
  {
    for (auto* signaler : signalers)
    {
      if (uv_is_closing(reinterpret_cast<uv_handle_t*>(signaler)) == 0) uv_close(reinterpret_cast<uv_handle_t*>(signaler)
        , [](uv_handle_t* handle) { delete reinterpret_cast<uv_signal_t*>(handle); }
      );
    }
    signalers.clear();
    uv_async_send(&loop_a); // sender acknoledge for last pending signaler events
  }
  inline void serve_() // loop in main thread
  {
    bool is_restart = false;
    uint8_t expected = 1;
    if (!state.compare_exchange_weak(expected, 2)) // initialized -> serving
    { // not initialized
      expected = 3; // stopped -> serving
      if (!state.compare_exchange_weak(expected, 2))
      { // not stopped
        if (state.load() != 2) return; // not serving
      }
      is_restart = true;
    }
    is_restart = is_restart || server_r.load();
    if (is_restart)
    {
      // 1. h2o dispose old context
      h2o_context_dispose(&ctx);
      uv_run(&loop, UV_RUN_ONCE); // process h2o context cleaning events
      // 2. h2o re-initialize context
      h2o_context_init(&ctx, &loop, &gconfig); // h2o load new uv listeners before serving
      // 3. h2o reset accept context
      hconfig_a[0] = hconfig;
      hconfig_a[1] = NULL;
      accept_ctx.hosts = hconfig_a;
      accept_ctx.ctx = &ctx;
      accept_ctx.ssl_ctx = ssl_ctx;
      // 4. uv update listeners h2o accept context records
      for (auto* listener : listeners)
      {
        listener->data = &accept_ctx; // uv load all h2o registry before serving
      }
    }
    server_r.store(false);
    start_time = std::chrono::steady_clock::now();
    {
      std::lock_guard<std::mutex> lock_server(server_m);
      server_a.store(true);
      server_c.notify_all();
    }
    uv_run(&loop, UV_RUN_DEFAULT); // main loop blocking: connections -> uv -> h2o socket callback -> h2o handler -> business
    server_a.store(false);
    state.store(3); // stopped
  }
  inline void start_() // loop in worker thread
  {
    server_r.store(false);
    uint8_t expected = 1;
    if (!state.compare_exchange_weak(expected, 2)) // initialized -> serving
    { // not initialized
      expected = 3; // stopped -> serving
      if (!state.compare_exchange_weak(expected, 2)) return; // not stopped
      server_r.store(true);
    }
    server_a.store(false);
    server_t = std::thread([this]() { serve_(); });
    std::unique_lock<std::mutex> lock_server(server_m);
    server_c.wait(lock_server, [this]() { return server_a.load() || state.load() == 3; });
  }
  inline void stop_() // clear h2o http connections then uv tcp listeners/signalers
  {
    uint8_t expected = 2;
    if (!state.compare_exchange_strong(expected, 3)) return; // serving -> stopped
    delisten_();
    designal_();
    h2o_context_request_shutdown(&ctx); // h2o request shutdown active connections
    uv_async_send(&loop_a); // sender acknoledge
    uv_stop(&loop); // set flag for uv_run() to return; persistent sender/timer
    uv_async_send(&loop_a); // sender acknoledge for loop stop flag
    server_c.notify_all();
    if (server_t.joinable() && server_t.get_id() != std::this_thread::get_id()) server_t.join();
    server_a.store(false);
  }
  inline int64_t uptime_() const // ms
  {
    if (state.load() != 2) return 0; // not serving
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
  }
  inline void fina_()
  {
    const uint8_t s = state.load();
    if (s == 0) return; // already finalized
    if (s == 2) stop_(); // serving -> stopped
    // 1. uv stop acceptance
    delisten_();
    // 2. uv close signalers
    designal_();
    // 3. h2o request shutdown active connections
    h2o_context_request_shutdown(&ctx);
    uv_async_send(&loop_a); // sender acknoledge
    // 4. h2o dispose context
    h2o_context_dispose(&ctx); // context-owned
    uv_async_send(&loop_a); // sender acknoledge
    // 5. h2o dispose config
    h2o_config_dispose(&gconfig); // config-owned: h2o_handler_t* -> h2o_pathconf_t* -> h2o_hostconf_t* -> h2o_globalconf_t
    // 6. uv clear all remainings i.e. loop sender and timer
    uv_close(reinterpret_cast<uv_handle_t*>(&loop_a), NULL);
    uv_close(reinterpret_cast<uv_handle_t*>(&loop_t), NULL);
    uv_run(&loop, UV_RUN_DEFAULT); // no persistent sender/timer/listeners/signalers -> instant return
    // 7. uv close loop
    uv_loop_close(&loop);
    // 8. openssl dispose ssl context
    if (ssl_ctx)
    {
      SSL_CTX_free(ssl_ctx);
      ssl_ctx = NULL;
    }
    // 9. deregister all
    prefix_groups.clear();
    // 10. h2o advanced recycle memory management finalize
    h2o_buffer_clear_recycle(1); // clear all recycled buffer blocks
    h2o_mem_clear_recycle(&h2o_mem_pool_allocator, 1); // clear recycled memory pool chunks
    if (server_t.joinable() && server_t.get_id() != std::this_thread::get_id()) server_t.join();
    state.store(0); // finalized
  }
  inline void register_(std::string _prefix
    , const std::string& _method
    , http_f _business
  )
  {
    if (state.load() % 2 != 1) return; // finalized or serving
    const http_m m(_method);
    // 1. prefix_c create if not exists
    auto& pc = prefix_groups[_prefix];
    // 2. h2o pathconf create if not exists
    if (!pc.pathconf) pc.pathconf = h2o_config_register_path(hconfig, _prefix.c_str(), 0);
    // 3. check handler
    auto handler_it = pc.handlers.find(m);
    if (handler_it == pc.handlers.end())
    {
      // 4. h2o create handler allocating memory
      h2o_handler_t* raw_handler = h2o_create_handler(pc.pathconf, sizeof(http_h));
      // 5. placement new to construct http_h in the allocated memory
      http_h* handler = new(raw_handler) http_h();
      // 6. set h2o callback
      raw_handler->on_req = &http_h::on_req_;
      raw_handler->dispose = &http_h::dispose_;
      // 7. set fields in handler
      handler->method = _method;
      handler->business = std::move(_business);
      // 8. store handler in prefix_c
      pc.handlers[m] = handler;
    }
    else
    {
      // 9. update handler in prefix_c
      handler_it->second->method = _method;
      handler_it->second->business = std::move(_business);
    }
  }
  inline void get_(std::string _prefix, http_f _business) { register_(_prefix, "GET", std::move(_business)); }
  inline void post_(std::string _prefix, http_f _business) { register_(_prefix, "POST", std::move(_business)); }
  inline void put_(std::string _prefix, http_f _business) { register_(_prefix, "PUT", std::move(_business)); }
  inline void delete_(std::string _prefix, http_f _business) { register_(_prefix, "DELETE", std::move(_business)); }
  inline void patch_(std::string _prefix, http_f _business) { register_(_prefix, "PATCH", std::move(_business)); }
  inline void options_(std::string _prefix, http_f _business) { register_(_prefix, "OPTIONS", std::move(_business)); }
  inline void head_(std::string _prefix, http_f _business) { register_(_prefix, "HEAD", std::move(_business)); }
  inline void trace_(std::string _prefix, http_f _business) { register_(_prefix, "TRACE", std::move(_business)); }
  inline void connect_(std::string _prefix, http_f _business) { register_(_prefix, "CONNECT", std::move(_business)); }
};
#endif
