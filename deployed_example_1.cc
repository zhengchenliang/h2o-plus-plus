// CMS Campaign Monitor Deployment Service
// ======================================
const std::string SECRET_TOKEN = "xxx";

std::cout << "Starting CMS Campaign Monitor Deployment Service on port 15472..." << std::endl;
http_a app;
app.ssl_("xxx.crt", "xxx.key");
// Listen on port 15472 for deployment monitoring
app.listen_("127.0.0.1", 15472);
// Set up signal handling
app.signal_();
// Register the campaign monitoring endpoint
app.get_("/5472/campmoni/v2", [&](const http_q& q, http_s& s)
{
  // Check authorization header
  std::string auth_header = q.header_("authorization");
  if (auth_header != "Bearer " + SECRET_TOKEN && auth_header != SECRET_TOKEN)
  {
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
    // Execute the campaign monitor program
    exe_r result = q.exe_load_("/root/G1/new2/A5472/A5472CAMPMONI0V2");
    // Check if execution was successful
    if (result.status != 3)
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
    // Check exit code
    if (!result.exit_normal || result.exit_code != 0)
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
    // Parse and validate the JSON output
    try
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
// Health check endpoint
app.get_("/health", [&](const http_q& q, http_s& s)
{
  s.status_(200);
  s.send_json_(nlohmann::json{
    {"status", "healthy"},
    {"service", "CMS Campaign Monitor"},
    {"version", "v2"},
    {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
  });
});

std::cout << "Service ready. Listening on http://127.0.0.1:15472" << std::endl;
std::cout << "Endpoints:" << std::endl;
std::cout << "  GET /5472/campmoni/v2 (requires authorization)" << std::endl;
std::cout << "  GET /health" << std::endl;
std::cout << "Serving by process [" << getpid() << "] ..." << std::endl;

app.serve_();

return 0;
