#include "intel.hpp"

Intel::Intel(
    const std::string drm_node, const std::string pci_dev,
    uint16_t vendor_id, uint16_t device_id
) : GPUWithHwmon(drm_node, pci_dev, vendor_id, device_id) {
    pthread_setname_np(worker_thread.native_handle(), "gpu-intel");
    init_hwmon();
}

void Intel::init_hwmon() {
    const std::vector<hwmon_sensor> sensors = {
        { "voltage"     , "in0_input"       },
        { "fan_speed"   , "fan1_input"      },
        { "temp"        , "temp1_input"     },
        { "energy"      , "energy1_input"   },
        { "power_limit" , "power1_max"      },
    };

    hwmon.drm_node = drm_node;
    hwmon.base_dir = hwmon.find_hwmon_dir();
    hwmon.add_sensors(sensors);
    hwmon.setup();
}

gpu_metrics Intel::get_metrics() {
    SPDLOG_DEBUG("Intel get_metrics()");
    std::unique_lock lock(metrics_mutex);
    return metrics;
}

int Intel::get_temperature() {
    return hwmon.get_sensor_value("temp");
}

float Intel::get_power_usage() {
    uint64_t current_power_usage = hwmon.get_sensor_value("energy");

    if (!previous_power_usage) {
        previous_power_usage = current_power_usage;
        return 0;
    }

    uint64_t delta = current_power_usage - previous_power_usage;
    previous_power_usage = current_power_usage;

    return delta / 1'000'000.f;
}

float Intel::get_power_limit() {
    float limit = hwmon.get_sensor_value("power_limit") / 1'000'000.f;
    return limit;
}

int Intel::get_voltage() {
    return hwmon.get_sensor_value("voltage");
}
