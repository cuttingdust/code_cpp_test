@echo off
chcp 65001 > nul
cmake -S . -B build 
timeout 2