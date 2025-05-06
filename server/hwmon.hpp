#pragma once

#include <string>
#include <fstream>
#include <cstdint>
#include <map>
#include <regex>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "gpu.hpp"

namespace fs = std::filesystem;

struct hwmon_sensor {
    std::string generic_name;
    std::string filename;
    bool use_regex = false;
};

class Hwmon {
private:
    struct sensor {
        std::string filename;
        bool use_regex = false;

        std::ifstream stream;
        std::string path;
        unsigned char id = 0;
        uint64_t val = 0;
    };

    std::map<std::string, sensor> sensors;

    void find_sensors();
    void open_sensors();

public:
    std::string drm_node;
    std::string base_dir;

    std::string find_hwmon_dir();
    std::string find_hwmon_dir_by_name(std::string name);

    void add_sensors(const std::vector<hwmon_sensor>& input_sensors);
    void setup();
    void poll_sensors();

    uint64_t get_sensor_value(const std::string generic_name);
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
