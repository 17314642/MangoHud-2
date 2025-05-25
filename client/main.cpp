#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>

#include "../common/socket.hpp"

using namespace std::chrono_literals;

bool create_socket(int& sock) {
    sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);

    if (sock < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't create socket.");
        return false;
    }

    SPDLOG_DEBUG("Socket created.");
    return true;
}

bool connect_to_socket(const int sock, const sockaddr_un& addr) {
    int ret = connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_un));

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to connect to server socket.");
        return false;
    }

    SPDLOG_INFO("Connected to server");
    return true;
}

void print_gpu_info(const gpu& gpu) {
    const gpu_metrics_system* system_metrics = &gpu.system_metrics;
    const gpu_metrics_process* process_metrics = &gpu.process_metrics;

    SPDLOG_INFO("load                 = {}\n", system_metrics->load);

    SPDLOG_INFO("vram_used            = {}", system_metrics->vram_used);
    SPDLOG_INFO("gtt_used             = {}", system_metrics->gtt_used);
    // SPDLOG_TRACE("memory_total         = {}", memory_total);
    // SPDLOG_TRACE("memory_clock         = {}", memory_clock);
    // SPDLOG_TRACE("memory_temp          = {}\n", memory_temp);

    // SPDLOG_TRACE("temperature          = {}", temperature);
    // SPDLOG_TRACE("junction_temperature = {}\n", junction_temperature);

    // SPDLOG_TRACE("core_clock           = {}", core_clock);
    SPDLOG_INFO("voltage              = {}\n", system_metrics->voltage);

    SPDLOG_INFO("power_usage          = {}", system_metrics->power_usage);
    SPDLOG_INFO("power_limit          = {}\n", system_metrics->power_limit);

    // SPDLOG_TRACE("apu_cpu_power        = {}", apu_cpu_power);
    // SPDLOG_TRACE("apu_cpu_temp         = {}\n", apu_cpu_temp);

    // SPDLOG_TRACE("is_power_throttled   = {}", is_power_throttled);
    // SPDLOG_TRACE("is_current_throttled = {}", is_current_throttled);
    // SPDLOG_TRACE("is_temp_throttled    = {}", is_temp_throttled);
    // SPDLOG_TRACE("is_other_throttled   = {}\n", is_other_throttled);

    // SPDLOG_TRACE("fan_speed            = {}", fan_speed);
    // SPDLOG_TRACE("fan_rpm              = {}\n", fan_rpm);

    SPDLOG_INFO("Process stats ({}):", getpid());
    SPDLOG_INFO("    load      = {}", process_metrics->load);
    SPDLOG_INFO("    vram_used = {}", process_metrics->vram_used);
    SPDLOG_INFO("    gtt_used  = {}\n", process_metrics->gtt_used);
}

void poll_server(pollfd& server_poll, bool& connected, int sock) {
    mangohud_message msg = {};
    send_message(server_poll.fd, msg);

    int ret = poll(&server_poll, 1, 1000);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("poll() failed.");
        return;
    }

    // no new data
    if (ret < 1)
        return;

    // Check events
    if (server_poll.revents == 0)
        return;

    SPDLOG_INFO("fd {}: revents = {}", server_poll.fd, server_poll.revents);

    if (server_poll.revents & POLLHUP || server_poll.revents & POLLNVAL) {
        connected = false;
        close(sock);
        return;
    }

    if (!receive_message(server_poll.fd, msg)) {
        SPDLOG_DEBUG("Failed to receive message from server.");
        return;
    }

    SPDLOG_INFO("==========================");

    for (int i = 0; i < msg.num_of_gpus; i++)
        print_gpu_info(msg.gpus[i]);

    SPDLOG_INFO("==========================\n");
}

int main () {
    spdlog::set_level(spdlog::level::level_enum::trace);

    std::string socket_path = "/home/user/Desktop/mangohud.sock";

    const sockaddr_un addr = {
        .sun_family = AF_UNIX
    };

    std::strncpy(const_cast<char*>(addr.sun_path), socket_path.c_str(), socket_path.size());

    int sock = 0;
    pollfd server_poll = { .fd = sock, .events = POLLIN };

    std::chrono::time_point<std::chrono::steady_clock> previous_time;

    bool connected = false;

    while (true) {
        if (!connected) {
            SPDLOG_WARN("Lost connection to server. Retrying...");

            if (!create_socket(sock)) {
                std::this_thread::sleep_for(1s);
                continue;
            }

            if (!connect_to_socket(sock, addr)) {
                close(sock);
                std::this_thread::sleep_for(1s);
                continue;
            }

            server_poll.fd = sock;
            connected = true;
        }

        poll_server(server_poll, connected, sock);
        std::this_thread::sleep_for(1s);
    }

    return 0;
}

void __attribute__ ((constructor)) mangohud_client_init(void) {
    std::thread t = std::thread(&main);
    t.detach();
}
