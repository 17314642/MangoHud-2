#include <thread>
#include <chrono>

#include "gpu.hpp"
#include "fdinfo.hpp"

int main() {
    spdlog::set_level(spdlog::level::level_enum::trace);

    GPUS gpus;

    for (auto& gpu : gpus.available_gpus) {
        if (FDInfo* ptr = dynamic_cast<FDInfo*>(gpu.get())) {
            ptr->add_fdinfo_pid(167311);
            ptr->add_fdinfo_pid(153790);
        }

        gpu->add_pid(167311);
        gpu->add_pid(153790);
    }

    while (true) {
        for (auto& gpu : gpus.available_gpus) {
            gpu->print_metrics();
        }

        std::this_thread::sleep_for(1s);
    }
}
