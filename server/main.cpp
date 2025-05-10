#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <utility>

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>

#include "gpu.hpp"
#include "fdinfo.hpp"
#include "../common/log_errno.hpp"

void receive_data(int fd) {
    SPDLOG_TRACE("receive_data()");

    const size_t buf_size = 4096;
    std::vector<char> buf(buf_size);

    if (buf.capacity() < static_cast<size_t>(buf_size)) {
        SPDLOG_DEBUG("Failed to reserve {} bytes for data buffer", buf_size);
        return;
    }

    ssize_t bytes_read = recv(fd, buf.data(), buf_size, 0);

    if (bytes_read < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't read data from socket fd {}.", fd);
        return;
    }

    std::string s = buf.data();
    SPDLOG_DEBUG("Received new data from fd {} len={}: \"{}\"", fd, bytes_read, s);
}

int main() {
    spdlog::set_level(spdlog::level::level_enum::trace);

    // GPUS gpus;

    // if (FDInfo* ptr = dynamic_cast<FDInfo*>(gpu.get())) {}

    std::string socket_path = "../mangohud.sock";
    std::string msg = "Hello from server!";

    int sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);

    if (sock < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't create socket.");
        return -1;
    }

    SPDLOG_INFO("Socket created.");
    remove(socket_path.c_str());

    const sockaddr_un addr = {
        .sun_family = AF_UNIX
    };

    std::strncpy(const_cast<char*>(addr.sun_path), socket_path.c_str(), socket_path.size());

    int ret = bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_un));

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to assign name to socket.");
        return -1;
    }

    ret = listen(sock, 0);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to listen to socket.");
        return -1;
    }

    std::vector<pollfd> poll_fds = {
        { .fd = sock, .events = POLLIN }
    };

    while (true) {
        ret = poll(poll_fds.data(), poll_fds.size(), 1000);

        if (ret < 0) {
            LOG_UNIX_ERRNO_ERROR("poll() failed.");
            continue;
        }

        // no new data
        if (ret < 1)
            continue;

        std::set<std::vector<pollfd>::iterator> fds_to_close;
        std::vector<pollfd> fds_to_add;

        // Check events
        for (auto fd = poll_fds.begin(); fd != poll_fds.end(); fd++) {
            if (fd->revents == 0)
                continue;

            SPDLOG_TRACE("fd {}: revents = {}", fd->fd, fd->revents);

            if (fd->fd == sock) {
                ret = accept(fd->fd, nullptr, nullptr);

                if (ret < 0) {
                    LOG_UNIX_ERRNO_WARN("Failed to accept new connection.");
                    continue;
                }

                fds_to_add.push_back({.fd = ret, .events = POLLIN});
                SPDLOG_INFO("Accepted new connection: fd={}", ret);
            } else {
                if (fd->revents & POLLHUP || fd->revents & POLLNVAL) {
                    fds_to_close.insert(fd);
                    continue;
                }

                receive_data(fd->fd);
            }
        }

        // Close broken connections in reverse to not trigger vector reordering
        for (auto it = fds_to_close.rbegin(); it != fds_to_close.rend(); it++) {
            std::vector<pollfd>::iterator fd = *it;
            // SPDLOG_TRACE("fd = {}", fd->fd);

            ret = close(fd->fd);

            if (ret < 0) {
                LOG_UNIX_ERRNO_WARN("Failed to close connection fd {}.", fd->fd);
                continue;
            }

            // do not put spdlog_info() after poll_fds.erase(),
            // because fd will now point to next element.
            SPDLOG_INFO("Closed connection fd {}", fd->fd);
            poll_fds.erase(fd);
        }

        // Add new connections to poll()
        for (auto& fd : fds_to_add) {
            poll_fds.push_back(fd);
        }
    }

    return 0;

    // while (true) {
    //     for (auto& gpu : gpus.available_gpus) {
    //         gpu->print_metrics();
    //     }

    //     std::this_thread::sleep_for(1s);
    // }
}
