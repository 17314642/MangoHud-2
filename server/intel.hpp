#pragma once

#include <cstdint>

#include "gpu.hpp"
#include "hwmon.hpp"
#include "fdinfo.hpp"

// class IntelThrottling : Throttling {
//     std::ifstream throttle_status_stream;
//     std::vector<std::ifstream> throttle_power_streams;
//     std::vector<std::ifstream> throttle_current_streams;
//     std::vector<std::ifstream> throttle_temp_streams;
//     bool check_throttle_reasons(std::vector<std::ifstream> &throttle_reason_streams);
//     int get_throttling_status();

//     const std::vector<std::string> intel_throttle_power = {"reason_pl1", "reason_pl2"};
//     const std::vector<std::string> intel_throttle_current = {"reason_pl4", "reason_vr_tdc"};
//     const std::vector<std::string> intel_throttle_temp = {
//         "reason_prochot", "reason_ratl", "reason_thermal", "reason_vr_thermalert"};
//     void load_xe_i915_throttle_reasons(
//         std::string throttle_folder,
//         std::vector<std::string> throttle_reasons,
//         std::vector<std::ifstream> &throttle_reason_streams);
// }

class Intel : public GPU, private Hwmon, public FDInfo {
private:
    const std::vector<hwmon_sensor> sensors = {
        { "voltage"     , "in0_input"       },
        { "fan_speed"   , "fan1_input"      },
        { "temp"        , "temp1_input"     },
        { "energy"      , "energy1_input"   },
        { "power_limit" , "power1_max"      },
    };

    uint64_t previous_power_usage = 0;
    std::map<pid_t, uint64_t> previous_gpu_times;

    uint64_t get_pid_gpu_time(pid_t pid);
    float get_fdinfo_memory_used(pid_t pid, const std::string& key);

protected:
    void poll_overrides() override;

public:
    Intel(
        const std::string& drm_node, const std::string& pci_dev,
        uint16_t vendor_id, uint16_t device_id
    );

    // System-related functions
    // int     get_load() override;                 // Not available

    // float   get_vram_used() override;            // Not available
    // float   get_gtt_used() override;             // Not available
    // float   get_memory_total() override;         // TODO via DRM uAPI
    // int     get_memory_clock() override;         // Not available
    // int     get_memory_temp() override;          // TODO

    int     get_temperature() override;
    // int     get_junction_temperature() override; // Not available

    // int     get_core_clock() override;           // TODO
    int     get_voltage() override;

    float   get_power_usage() override;
    float   get_power_limit() override;

    // float   get_apu_cpu_power() override;        // Investigate
    // int     get_apu_cpu_temp() override;         // Investigate

    // bool    get_is_power_throttled() override;   // TODO
    // bool    get_is_current_throttled() override; // TODO
    // bool    get_is_temp_throttled() override;    // TODO
    // bool    get_is_other_throttled() override;   // TODO

    int     get_fan_speed() override;
    bool    get_fan_rpm() override;

    // Process-related functions
    int     get_process_load(pid_t pid) override;
    float   get_process_vram_used(pid_t pid) override;
    float   get_process_gtt_used(pid_t pid) override;
};
