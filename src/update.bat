@echo off
chcp 65001 > nul
git submodule update --remote --recursive
cmake -S . -B build 
timeout 2