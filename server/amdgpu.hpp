#pragma once

#include <cstdint>

#include "gpu.hpp"
#include "hwmon.hpp"
#include "fdinfo.hpp"

class AMDGPU : public GPU, private Hwmon {
private:
    const std::vector<hwmon_sensor> sensors = {
        { "temp"        , "temp1_input"     },
        { "frequency"   , "freq1_input"     }
    };

    const std::vector<hwmon_sensor> sysfs_sensors = {
        { "load"        , "gpu_busy_percent"    },
        { "vram_used"   , "mem_info_vram_used"  },
        { "gtt_used"    , "mem_info_gtt_used"   },
        { "vram_total"  , "mem_info_vram_total" },
    };

    HwmonBase sysfs_hwmon;

public:
    AMDGPU(
        const std::string& drm_node, const std::string& pci_dev,
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
