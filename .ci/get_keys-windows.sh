#!/bin/sh -ex

curl --proxy "http://127.0.0.1:1087" -L -o "./llvm.lock" "https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win/llvmlibs_mt.7z.sha256"
curl --proxy "http://127.0.0.1:1087" -L -o "./glslang.lock" "https://github.com/RPCS3/glslang/releases/download/custom-build-win/glslanglibs_mt.7z.sha256"
