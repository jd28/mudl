#!/bin/sh

bin/shaderc -f src/vs_mudl.sc --type vertex --platform osx -o bin/shaders/metal/vs_mudl.bin -p metal
bin/shaderc -f src/fs_mudl.sc --type fragment --platform osx -o bin/shaders/metal/fs_mudl.bin -p metal
