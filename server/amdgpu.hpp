#pragma once

#include <cstdint>

#include "gpu.hpp"

class AMDGPU : public GPUWithHwmon {
private:
    uint64_t previous_power_usage = 0;

    Hwmon sysfs_hwmon;

    void init_hwmon();
    void init_sysfs_sensors();

public:
    AMDGPU(
        const std::string drm_node, const std::string pci_dev,
        uint16_t vendor_id, uint16_t device_id
    );

    void poll_overrides() override;
    gpu_metrics get_metrics() override;

    int     get_load()          override;       
    float   get_sys_vram_used() override;    
    float   get_gtt_used()      override;
    float   get_memory_total()  override;
    int     get_temperature()   override;
    int     get_core_clock()    override;
};
