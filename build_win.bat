@echo off
REM Set path to vcpkg installed protoc
set PROTOC_PATH=D:/work/git/vcpkg/installed/x64-windows/tools/protobuf/protoc.exe

REM Generate C++ files from proto files in message folder
%PROTOC_PATH% --cpp_out=./ ./message/*.proto

REM Configure the project with vcpkg toolchain
cmake -B build_win -S . -DCMAKE_TOOLCHAIN_FILE=D:/work/git/vcpkg/scripts/buildsystems/vcpkg.cmake

REM Build the project
cmake --build build_win
