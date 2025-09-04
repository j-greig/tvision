#include "api_ipc.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include <cstring>
#include <sstream>
#include <map>

// Forward declarations of helper methods implemented in test_pattern_app.cpp.
extern void api_spawn_test(TTestPatternApp& app);
extern void api_spawn_gradient(TTestPatternApp& app, const std::string& kind);
extern void api_open_animation_path(TTestPatternApp& app, const std::string& path);
extern void api_cascade(TTestPatternApp& app);
extern void api_tile(TTestPatternApp& app);
extern void api_close_all(TTestPatternApp& app);
extern void api_set_pattern_mode(TTestPatternApp& app, const std::string& mode);
extern void api_save_workspace(TTestPatternApp& app);
extern void api_open_workspace_path(TTestPatternApp& app, const std::string& path);
extern void api_screenshot(TTestPatternApp& app);

ApiIpcServer::ApiIpcServer(TTestPatternApp* app) : app_(app) {}

ApiIpcServer::~ApiIpcServer() { stop(); }

bool ApiIpcServer::start(const std::string& path) {
#ifdef _WIN32
    (void)path; return false;
#else
    sock_path_ = path;
    // Clean up any stale socket.
    ::unlink(sock_path_.c_str());
    fd_listen_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_listen_ < 0)
        return false;
    // Non-blocking
    int flags = ::fcntl(fd_listen_, F_GETFL, 0);
    ::fcntl(fd_listen_, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path_.c_str());
    if (::bind(fd_listen_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ::close(fd_listen_);
        fd_listen_ = -1;
        return false;
    }
    if (::listen(fd_listen_, 4) < 0) {
        ::close(fd_listen_);
        fd_listen_ = -1;
        return false;
    }
    return true;
#endif
}

void ApiIpcServer::poll() {
#ifdef _WIN32
    return;
#else
    if (fd_listen_ < 0 || !app_) return;
    int fd = ::accept(fd_listen_, nullptr, nullptr);
    if (fd < 0) {
        return; // EAGAIN expected in non-blocking mode
    }
    // Read a single line command.
    char buf[2048];
    ssize_t n = ::read(fd, buf, sizeof(buf)-1);
    if (n <= 0) {
        ::close(fd);
        return;
    }
    buf[n] = 0;
    std::string line(buf);
    // Simple trim
    while (!line.empty() && (line.back()=='\n' || line.back()=='\r' || line.back()==' ')) line.pop_back();

    // Parse: "cmd:<name> k=v k=v"
    std::string cmd;
    std::map<std::string,std::string> kv;
    {
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) {
            if (tok.rfind("cmd:", 0) == 0) {
                cmd = tok.substr(4);
            } else {
                auto eq = tok.find('=');
                if (eq != std::string::npos) {
                    kv[tok.substr(0, eq)] = tok.substr(eq+1);
                }
            }
        }
    }

    std::string resp = "ok\n";
    if (cmd == "create_window") {
        std::string type = kv["type"]; // test_pattern|gradient|frame_player|text_view
        if (type == "test_pattern") {
            api_spawn_test(*app_);
        } else if (type == "gradient") {
            std::string kind = kv.count("gradient") ? kv["gradient"] : std::string("horizontal");
            api_spawn_gradient(*app_, kind);
        } else if (type == "frame_player" || type == "text_view") {
            auto it = kv.find("path");
            if (it != kv.end()) api_open_animation_path(*app_, it->second);
            else resp = "err missing path\n";
        } else {
            resp = "err unknown type\n";
        }
    } else if (cmd == "cascade") {
        api_cascade(*app_);
    } else if (cmd == "tile") {
        api_tile(*app_);
    } else if (cmd == "close_all") {
        api_close_all(*app_);
    } else if (cmd == "pattern_mode") {
        std::string mode = kv["mode"]; // continuous|tiled
        api_set_pattern_mode(*app_, mode);
    } else if (cmd == "save_workspace") {
        api_save_workspace(*app_);
    } else if (cmd == "open_workspace") {
        auto it = kv.find("path");
        if (it != kv.end()) api_open_workspace_path(*app_, it->second);
        else resp = "err missing path\n";
    } else if (cmd == "screenshot") {
        api_screenshot(*app_);
    } else {
        resp = "err unknown cmd\n";
    }

    ::write(fd, resp.c_str(), resp.size());
    ::close(fd);
#endif
}

void ApiIpcServer::stop() {
#ifndef _WIN32
    if (fd_listen_ >= 0) {
        ::close(fd_listen_);
        fd_listen_ = -1;
    }
    if (!sock_path_.empty()) {
        ::unlink(sock_path_.c_str());
    }
#endif
}
