const std::string SECRET_TOKEN = "xxx";

std::cout << "Starting CMS Campaign Monitor Deployment Server ..." << std::endl;
http_a app;
//app.ssl_("/etc/letsencrypt/live/xxx.com/fullchain.pem", "/etc/letsencrypt/live/xxx.com/privkey.pem"); // ssl terminated by nginx
app.listen_("127.0.0.1", 15472);
app.signal_();
// campaign monitor endpoint
app.get_("/campmoni/v2", [&](const http_q& q, http_s& s)
{
  std::string auth_header = q.header_("authorization"); // authorize
  if (auth_header != "Bearer " + SECRET_TOKEN && auth_header != SECRET_TOKEN)
  { // curl -H "Authorization: Bearer <token>" https://xxx.com/campmoni/v2 or http://127.0.0.1:15472/campmoni/v2
    s.status_(401);
    s.header_("WWW-Authenticate", "Bearer");
    s.send_json_(nlohmann::json{
      {"error", "Unauthorized"},
      {"message", "Invalid or missing authorization token"}
    });
    return;
  }
  try
  {
    if (q.query_has_("clear") && q.query_("clear") == "1") // /campmoni/v2?clear=1
    {
      exe_r result = q.exe_load_("./A5472CAMPMONI0V2", "clear", "", {});
      if (result.status != 3) // check execute complete
      { // not completed
        s.status_(500);
        s.send_json_(nlohmann::json{
          {"error", "Execution failed"},
          {"status", result.status},
          {"exit_code", result.exit_code},
          {"stderr", result.stderr_info}
        });
        return;
      }
      if (!result.exit_normal || result.exit_code < 0) // check execute code
      {
        s.status_(500);
        s.send_json_(nlohmann::json{
          {"error", "Command failed"},
          {"exit_code", result.exit_code},
          {"exit_normal", result.exit_normal},
          {"stderr", result.stderr_info}
        });
        return;
      }
      s.status_(200);
      s.send_json_(nlohmann::json{
        {"status", "success"},
        {"message", "Update timestamp cleared."}
      });
      return;
    }

    std::unordered_map<std::string, std::string> env_vars; // for Python venv environment
    env_vars["PATH"] = "./a5472_venv/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";
    env_vars["VIRTUAL_ENV"] = "./a5472_venv";
    env_vars["PYTHONHOME"] = ""; // clear PYTHONHOME for venv
    exe_r result = q.exe_load_("./A5472CAMPMONI0V2", {}, "", env_vars); // execute with venv
    if (result.status != 3) // check execute complete
    { // not completed
      s.status_(500);
      s.send_json_(nlohmann::json{
        {"error", "Execution failed"},
        {"status", result.status},
        {"exit_code", result.exit_code},
        {"stderr", result.stderr_info}
      });
      return;
    }
    if (!result.exit_normal || result.exit_code < 0) // check execute code
    {
      s.status_(500);
      s.send_json_(nlohmann::json{
        {"error", "Command failed"},
        {"exit_code", result.exit_code},
        {"exit_normal", result.exit_normal},
        {"stderr", result.stderr_info}
      });
      return;
    }
    try // parse json stdout
    {
      nlohmann::json monitor_data = nlohmann::json::parse(result.stdout_info);
      s.status_(200);
      s.send_json_(monitor_data);
    }
    catch (const std::exception& e)
    {
      s.status_(500);
      s.send_json_(nlohmann::json{
        {"error", "Invalid JSON output"},
        {"parse_error", e.what()},
        {"raw_output", result.stdout_info}
      });
    }
  }
  catch (const std::exception& e)
  {
    s.status_(500);
    s.send_json_(nlohmann::json{
      {"error", "Internal server error"},
      {"message", e.what()}
    });
  }
});
// health check endpoint with user blocking
app.get_("/health", [&](const http_q& q, http_s& s)
{
  std::thread([s]() mutable
  {
    try
    {
      std::this_thread::sleep_for(std::chrono::seconds(10)); // make user wait
      s.status_(200);
      s.send_json_(nlohmann::json{
        {"status", "healthy"},
        {"service", "CMS Campaign Monitor"},
        {"version", "v2"},
        {"processing_time", "10 seconds"},
        {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
      });
    }
    catch (const std::exception& e)
    {
      s.status_(500);
      s.send_json_(nlohmann::json{
        {"status", "error"},
        {"error", "Internal server error during health check"}
      });
    }
  }).detach();
});

std::cout << "Service ready. Listening on http://127.0.0.1:15472" << std::endl;
std::cout << "Endpoints:" << std::endl;
std::cout << "  GET /campmoni/v2 ?clear=1 (requires authorization)" << std::endl;
std::cout << "  GET /health" << std::endl;
std::cout << "Serving by process [" << getpid() << "] ..." << std::endl;

app.serve_();

return 0;
