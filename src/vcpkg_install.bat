@echo off
chcp 65001 > nul
git submodule add https://github.com/nlohmann/json.git third_party/json
git submodule add https://github.com/AmokHuginnsson/replxx.git third_party/replxx
git submodule add https://github.com/p-ranav/indicators.git third_party/indicators
vcpkg install
timeout 5