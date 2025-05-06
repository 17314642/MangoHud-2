#include <algorithm>

#include "gpu.hpp"
#include "intel.hpp"
#include "../common/helpers.hpp"

GPUS::GPUS() {
    std::vector<std::string> gpu_entries;

    for (const auto& entry : fs::directory_iterator("/sys/class/drm")) {
        if (!entry.is_directory())
            continue;

        std::string node_name = entry.path().filename().string();

        // Check if the directory is a render node (e.g., renderD128, renderD129, etc.)
        if (node_name.find("renderD") == 0 && node_name.length() > 7) {
            // Ensure the rest of the string after "renderD" is numeric
            std::string render_number = node_name.substr(7);
            if (std::all_of(render_number.begin(), render_number.end(), ::isdigit)) {
                gpu_entries.push_back(node_name);  // Store the render entry
            }
        }
    }

    // Sort the entries based on the numeric value of the render number
    std::sort(gpu_entries.begin(), gpu_entries.end(), [](const std::string& a, const std::string& b) {
        int num_a = std::stoi(a.substr(7));
        int num_b = std::stoi(b.substr(7));
        return num_a < num_b;
    });

    // Now process the sorted GPU entries
    uint8_t idx = 0, total_active = 0;

    for (const auto& drm_node : gpu_entries) {
        std::string path = "/sys/class/drm/" + drm_node;
        std::string device_address = get_pci_device_address(path);  // Store the result
        const char* pci_dev = device_address.c_str();

        uint32_t vendor_id = 0;
        uint32_t device_id = 0;

        if (!device_address.empty())
        {
            try {
                vendor_id = std::stoul(read_line("/sys/bus/pci/devices/" + device_address + "/vendor"), nullptr, 16);
            } catch(...) {
                SPDLOG_ERROR("stoul failed on: {}", "/sys/bus/pci/devices/" + device_address + "/vendor");
            }

            try {
                device_id = std::stoul(read_line("/sys/bus/pci/devices/" + device_address + "/device"), nullptr, 16);
            } catch (...) {
                SPDLOG_ERROR("stoul failed on: {}", "/sys/bus/pci/devices/" + device_address + "/device");
            }
        }

        if (!vendor_id) {
            auto line = read_line("/sys/class/drm/" + drm_node + "/device/uevent" );
            if (line.find("DRIVER=msm_dpu") != std::string::npos) {
                SPDLOG_DEBUG("MSM device found!");
                vendor_id = 0x5143;
            } else if (line.find("DRIVER=panfrost") != std::string::npos) {
                SPDLOG_DEBUG("Panfrost device found!");
                vendor_id = 0x1337; // what's panfrost vid?
            }
        }

        std::shared_ptr<GPU> gpu;

        if (vendor_id == 0x8086) {
            gpu = std::make_shared<Intel>(drm_node, pci_dev, vendor_id, device_id);
        } else {
            continue;
        }

        available_gpus.push_back(gpu);

        // if (params->gpu_list.size() == 1 && params->gpu_list[0] == idx++)
        //     gpu->is_active = true;

        // if (!params->pci_dev.empty() && pci_dev == params->pci_dev)
        //     gpu->is_active = true;

        SPDLOG_DEBUG("GPU Found: drm_node: {}, vendor_id: {:x} device_id: {:x} pci_dev: {}", drm_node, vendor_id, device_id, pci_dev);

        if (gpu->is_active) {
            SPDLOG_INFO("Set {} as active GPU (id={:x}:{:x} pci_dev={})", drm_node, vendor_id, device_id, pci_dev);
            total_active++;
        }
    }

    if (total_active < 2)
        return;

    for (auto& gpu : available_gpus) {
        if (!gpu->is_active)
            continue;

        SPDLOG_WARN(
            "You have more than 1 active GPU, check if you use both pci_dev "
            "and gpu_list. If you use fps logging, MangoHud will log only "
            "this GPU: name = {}, vendor = {:x}, pci_dev = {}",
            gpu->drm_node, gpu->vendor_id, gpu->pci_dev
        );

        break;
    }
}

std::string GPUS::get_pci_device_address(const std::string drm_card_path) {
    // /sys/class/drm/renderD128/device/subsystem -> /sys/bus/pci
    auto subsystem = fs::canonical(drm_card_path + "/device/subsystem").string();
    auto idx = subsystem.rfind("/") + 1; // /sys/bus/pci
                                         //         ^
                                         //         |- find this guy
    if (subsystem.substr(idx) != "pci")
        return "";

    // /sys/class/drm/renderD128/device
    //           convert to
    // /sys/devices/pci0000:00/0000:00:01.0/0000:01:00.0/0000:02:01.0/0000:03:00.0
    auto pci_addr = fs::read_symlink(drm_card_path + "/device").string();
    idx = pci_addr.rfind("/") + 1; // /sys/.../0000:03:00.0
                                   //         ^
                                   //         |- find this guy

    return pci_addr.substr(idx); // 0000:03:00.0
}

void GPU::poll() {
    while (!stop_thread) {
        SPDLOG_DEBUG("poll()");

        poll_overrides();

        gpu_metrics cur_metrics = {
            .load                   = get_load(),

            .sys_vram_used          = get_sys_vram_used(),
            .proc_vram_used         = get_proc_vram_used(),
            .gtt_used               = get_gtt_used(),
            .memory_total           = get_memory_total(),
            .memory_clock           = get_memory_clock(),
            .memory_temp            = get_memory_temp(),

            .temperature            = get_temperature(),
            .junction_temperature   = get_junction_temperature(),

            .core_clock             = get_core_clock(),
            .voltage                = get_voltage(),

            .power_usage            = get_power_usage(),
            .power_limit            = get_power_limit(),

            .apu_cpu_power          = get_apu_cpu_power(),
            .apu_cpu_temp           = get_apu_cpu_temp(),

            .is_power_throttled     = get_is_power_throttled(),
            .is_current_throttled   = get_is_current_throttled(),
            .is_temp_throttled      = get_is_temp_throttled(),
            .is_other_throttled     = get_is_other_throttled(),

            .fan_speed              = get_fan_speed(),
            .fan_rpm                = get_fan_rpm()
        };

        {
            std::unique_lock lock(metrics_mutex);
            metrics = cur_metrics;
        }

        std::this_thread::sleep_for(1s);
    }
}