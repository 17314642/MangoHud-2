#include <filesystem>
#include <vector>
#include <fstream>
#include <set>
#include <spdlog/spdlog.h>
#include "fdinfo.hpp"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

FDInfoBase::FDInfoBase(const std::string& drm_node, const pid_t pid) : drm_node(drm_node), pid(pid) {
    init();
}

void FDInfoBase::init()
{
    std::vector<std::string> fds = find_fds();

    fds_streams.clear();
    fds_data.clear();

    open_fds(fds);

    last_init = std::chrono::steady_clock::now();
}

void FDInfoBase::poll() {
    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - last_init);

    // some games open handles to gpus later in the game,
    // so we need to constantly re-check for new fds
    if (diff >= 10s)
        init();

    for (size_t i = 0; i < fds_streams.size(); i++) {
        fds_streams[i].clear();
        fds_streams[i].seekg(0);

        for (std::string line; std::getline(fds_streams[i], line);) {
            auto key = line.substr(0, line.find(":"));
            auto val = line.substr(key.length() + 2);
            // SPDLOG_TRACE("{} = {}", key, val);
            fds_data[i][key] = val;
        }
    }
}

std::vector<std::string> FDInfoBase::find_fds() {
    std::string dir = "/proc/" + std::to_string(pid) + "/fd";
    fs::path path = dir;

    SPDLOG_DEBUG("fd_dir = {}", dir);

    if (!fs::exists(path)) {
        SPDLOG_DEBUG("{} does not exist", path.string());
        return {};
    }

    std::vector<std::string> fds;

    for (const auto& entry : fs::directory_iterator(path)) {
        if (!entry.is_symlink())
            continue;
        
        std::string link;
        
        try {
            link = fs::read_symlink(entry).filename();
        } catch(const std::filesystem::filesystem_error& ex) {
            SPDLOG_TRACE("{}", ex.what());
            continue;
        }

        if (link != drm_node)
            continue;
        
        fds.push_back(entry.path().filename());
    }

    return fds;
}

void FDInfoBase::open_fds(const std::vector<std::string>& fds) {
    // set of unique ids, dont open fds which contain
    // existing ids, because they will contain same data 
    std::set<std::string> client_ids;
    size_t total = 0;

    for (const std::string& fd: fds) {
        fs::path p = "/proc/" + std::to_string(pid) + "/fdinfo/" + fd;

        if (!fs::exists(p))
            continue;

        std::ifstream file(p);

        if (!file.is_open())
            continue;

        for (std::string line; std::getline(file, line);) {
            std::string key = line.substr(0, line.find(":"));
            std::string val = line.substr(key.length() + 2);

            if (key == "drm-client-id" && client_ids.find(val) != client_ids.end())
                continue;
            
            total += 1;
            client_ids.insert(val);

            fds_streams.push_back(std::move(file));
            fds_data.push_back({});
        }
    }

    SPDLOG_DEBUG("Received {} ids, opened {} unique ids", fds.size(), total);
}

void FDInfoWrapper::add_pid(pid_t pid) {
    SPDLOG_DEBUG("adding pid {} to fdinfo", pid);
    std::unique_lock lock(pids_mutex);
    pids.try_emplace(pid, FDInfoBase(drm_node, pid));
}

void FDInfoWrapper::poll_all() {
    std::unique_lock lock(pids_mutex);
    std::set<pid_t> pids_to_delete;

    for (auto& p : pids) {
        pid_t pid = p.first;
        FDInfoBase* data = &p.second;

        SPDLOG_TRACE("polling pid {}", pid);

        if (!std::filesystem::exists("/proc/" + std::to_string(pid)))
            pids_to_delete.insert(pid);
        else
            data->poll();
    }

    for (const auto& p : pids_to_delete) {
        SPDLOG_TRACE("deleting pid {}", p);
        pids.erase(p);
    }
}

FDInfo::FDInfo(const std::string& drm_node) : fdinfo(FDInfoWrapper(drm_node)) {}
