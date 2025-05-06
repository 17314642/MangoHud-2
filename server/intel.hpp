#pragma once

#include <cstdint>

#include "gpu.hpp"

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

class Intel : public GPUWithHwmon {
private:
    uint64_t previous_power_usage = 0;

public:
    Intel(
        const std::string drm_node, const std::string pci_dev,
        uint16_t vendor_id, uint16_t device_id
    );

    gpu_metrics get_metrics() override;
    int get_temperature() override;
    float get_power_usage() override;
    float get_power_limit() override;
    int get_voltage() override;
};