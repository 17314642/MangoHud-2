#include "amdgpu.hpp"

AMDGPU::AMDGPU(
    const std::string drm_node, const std::string pci_dev,
    uint16_t vendor_id, uint16_t device_id
) : GPUWithHwmon(drm_node, pci_dev, vendor_id, device_id) {
    pthread_setname_np(worker_thread.native_handle(), "gpu-amdgpu");
    init_hwmon();
    init_sysfs_sensors();
}

void AMDGPU::init_hwmon() {
    std::vector<hwmon_sensor> sensors = {
        { "temp"        , "temp1_input"     },
        { "frequency"   , "freq1_input"     }
    };

    hwmon.drm_node = drm_node;
    hwmon.base_dir = hwmon.find_hwmon_dir();
    hwmon.add_sensors(sensors);
    hwmon.setup();
}

void AMDGPU::init_sysfs_sensors() {
    std::vector<hwmon_sensor> sensors = {
        { "load"        , "gpu_busy_percent"    },
        { "vram_used"   , "mem_info_vram_used"  },
        { "gtt_used"    , "mem_info_gtt_used"   },
        { "vram_total"  , "mem_info_vram_total" },
    };

    sysfs_hwmon.drm_node = drm_node;
    sysfs_hwmon.base_dir = "/sys/class/drm/" + drm_node + "/device";
    sysfs_hwmon.add_sensors(sensors);
    sysfs_hwmon.setup();
}

void AMDGPU::poll_overrides() {
    hwmon.poll_sensors();
    sysfs_hwmon.poll_sensors();
}

gpu_metrics AMDGPU::get_metrics() {
    SPDLOG_DEBUG("AMDGPU get_metrics()");
    std::unique_lock lock(metrics_mutex);
    return metrics;
}

int AMDGPU::get_load() {
    return sysfs_hwmon.get_sensor_value("load");
}

float AMDGPU::get_sys_vram_used() {
    float used = sysfs_hwmon.get_sensor_value("vram_used") / 1024.f / 1024.f;
    return used;
}

float AMDGPU::get_gtt_used() {
    float used = sysfs_hwmon.get_sensor_value("gtt_used") / 1024.f / 1024.f;
    return used;
}

float AMDGPU::get_memory_total() {
    float used = sysfs_hwmon.get_sensor_value("vram_total") / 1024.f / 1024.f;
    return used;
}

int AMDGPU::get_temperature() {
    float temp = hwmon.get_sensor_value("temp") / 1'000.f;
    return std::round(temp);
}

int AMDGPU::get_core_clock() {
    return hwmon.get_sensor_value("frequency") / 1'000'000.f;
}
