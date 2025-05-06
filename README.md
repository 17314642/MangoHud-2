#### Compilation

Project requires spdlog

```bash
mkdir build && \
g++ -ggdb -O0 server/*.cpp common/*.cpp -o build/mangohud_server -lspdlog -lfmt -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
```
