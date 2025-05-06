#include <thread>
#include <chrono>

#include "gpu.hpp"

int main() {
    spdlog::set_level(spdlog::level::level_enum::trace);

    GPUS gpus;

    while (true) {
        for (auto& gpu : gpus.available_gpus)
            gpu->get_metrics().print();

        std::this_thread::sleep_for(1000ms);
    }
}
