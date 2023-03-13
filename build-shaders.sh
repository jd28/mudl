#!/bin/sh

mkdir -p bin/shaders/metal/
bin/shaderc -f src/vs_mudl.sc --type vertex --platform osx -o bin/shaders/metal/vs_mudl.bin -p metal
bin/shaderc -f src/vs_skin_mudl.sc --type vertex --platform osx -o bin/shaders/metal/vs_skin_mudl.bin -p metal
bin/shaderc -f src/fs_mudl.sc --type fragment --platform osx -o bin/shaders/metal/fs_mudl.bin -p metal

mkdir -p bin/shaders/spirv/
bin/shaderc -f src/vs_mudl.sc --type vertex --platform linux -o bin/shaders/spirv/vs_mudl.bin -p spirv
bin/shaderc -f src/vs_skin_mudl.sc --type vertex --platform linux -o bin/shaders/spirv/vs_skin_mudl.bin -p spirv
bin/shaderc -f src/fs_mudl.sc --type fragment --platform linux -o bin/shaders/spirv/fs_mudl.bin -p spirv
