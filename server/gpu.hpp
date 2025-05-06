#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <regex>
#include <fstream>
#include <filesystem>

#include <pthread.h>
#include <spdlog/spdlog.h>

#include "hwmon.hpp"

using namespace std::chrono_literals;
namespace fs = std::filesystem;

struct gpu_metrics {
    int     load;
    
    float   sys_vram_used;
    float   proc_vram_used;
    float   gtt_used;
    float   memory_total;
    int     memory_clock;
    int     memory_temp;

    int     temperature;
    int     junction_temperature;

    int     core_clock;
    int     voltage;

    float   power_usage;
    float   power_limit;

    float   apu_cpu_power;
    int     apu_cpu_temp;

    bool    is_power_throttled;
    bool    is_current_throttled;
    bool    is_temp_throttled;
    bool    is_other_throttled;

    int     fan_speed;
    bool    fan_rpm;

    void print() {
        SPDLOG_TRACE("==========================");
        SPDLOG_TRACE("load                 = {}\n", load);

        SPDLOG_TRACE("sys_vram_used        = {}", sys_vram_used);
        SPDLOG_TRACE("proc_vram_used       = {}", proc_vram_used);
        SPDLOG_TRACE("gtt_used             = {}", gtt_used);
        SPDLOG_TRACE("memory_total         = {}", memory_total);
        SPDLOG_TRACE("memory_clock         = {}", memory_clock);
        SPDLOG_TRACE("memory_temp          = {}\n", memory_temp);

        SPDLOG_TRACE("temperature          = {}", temperature);
        SPDLOG_TRACE("junction_temperature = {}\n", junction_temperature);

        SPDLOG_TRACE("core_clock           = {}", core_clock);
        SPDLOG_TRACE("voltage              = {}\n", voltage);

        SPDLOG_TRACE("power_usage          = {}", power_usage);
        SPDLOG_TRACE("power_limit          = {}\n", power_limit);

        SPDLOG_TRACE("apu_cpu_power        = {}", apu_cpu_power);
        SPDLOG_TRACE("apu_cpu_temp         = {}\n", apu_cpu_temp);

        SPDLOG_TRACE("is_power_throttled   = {}", is_power_throttled);
        SPDLOG_TRACE("is_current_throttled = {}", is_current_throttled);
        SPDLOG_TRACE("is_temp_throttled    = {}", is_temp_throttled);
        SPDLOG_TRACE("is_other_throttled   = {}\n", is_other_throttled);

        SPDLOG_TRACE("fan_speed            = {}", fan_speed);
        SPDLOG_TRACE("fan_rpm              = {}", fan_rpm);
        SPDLOG_TRACE("==========================\n");
    }
};

class Throttling {
public:
    bool is_power_throttled     () { return false; };
    bool is_current_throttled   () { return false; };
    bool is_temp_throttled      () { return false; };
    bool is_other_throttled     () { return false; };
};
    
// class FDInfo {
// private:
//     std::vector<std::ifstream> fdinfo;
//     uint64_t fdinfo_last_update_ms = 0;

// public:
//     std::string drm_engine_type = "EMPTY";
//     std::string drm_memory_type = "EMPTY";

//     std::vector<std::map<std::string, std::string>> parsed_data;

//     void poll();
//     void find_fds();
//     void open_fd(std::string path);
// };

class GPU {
protected:
    gpu_metrics metrics = {};
    std::mutex metrics_mutex;

    std::thread worker_thread;

    virtual void poll_overrides() {}
    void poll();

    virtual int     get_load()                  { return 0; }

    virtual float   get_sys_vram_used()         { return 0.f; }
    virtual float   get_proc_vram_used()        { return 0.f; }
    virtual float   get_gtt_used()              { return 0.f; }
    virtual float   get_memory_total()          { return 0.f; }
    virtual int     get_memory_clock()          { return 0; }
    virtual int     get_memory_temp()           { return 0; }

    virtual int     get_temperature()           { return 0; }
    virtual int     get_junction_temperature()  { return 0; }

    virtual int     get_core_clock()            { return 0; }
    virtual int     get_voltage()               { return 0; }

    virtual float   get_power_usage()           { return 0.f; }
    virtual float   get_power_limit()           { return 0.f; }

    virtual float   get_apu_cpu_power()         { return 0.f; }
    virtual int     get_apu_cpu_temp()          { return 0; }

    virtual bool    get_is_power_throttled()    { return false; }
    virtual bool    get_is_current_throttled()  { return false; }
    virtual bool    get_is_temp_throttled()     { return false; }
    virtual bool    get_is_other_throttled()    { return false; }

    virtual int     get_fan_speed()             { return 0; }
    virtual bool    get_fan_rpm()               { return false; }

public:
    const std::string drm_node;
    const std::string pci_dev;
    const uint16_t vendor_id;
    const uint16_t device_id;

    std::atomic<bool> is_active = false;
    std::atomic<bool> stop_thread = false;

    GPU(
        const std::string drm_node, const std::string pci_dev,
        uint16_t vendor_id, uint16_t device_id
    ) : drm_node(drm_node), pci_dev(pci_dev), vendor_id(vendor_id), device_id(device_id) {
        worker_thread = std::thread(&GPU::poll, this);
    }

    ~GPU() {
        stop_thread = true;
        if (worker_thread.joinable())
            worker_thread.join();
    }

    virtual gpu_metrics get_metrics() {
        SPDLOG_DEBUG("GPU get_metrics()");
        std::unique_lock lock(metrics_mutex);
        return metrics;
    }
};

class GPUWithHwmon : public GPU {
protected:
    Hwmon hwmon;

    void poll_overrides() override {
        hwmon.poll_sensors();
    }

    GPUWithHwmon(
        const std::string drm_node, const std::string pci_dev,
        uint16_t vendor_id, uint16_t device_id
    ) : GPU(drm_node, pci_dev, vendor_id, device_id) {}
};

class GPUS {
private:
    std::string get_pci_device_address(const std::string drm_card_path);

public:
    std::vector<std::shared_ptr<GPU>> available_gpus;
    GPUS();
};