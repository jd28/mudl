md .\bin\shaders\dx11 -ea 0
bin\shaderc -f src/vs_mudl.sc --type vertex --platform windows -o bin/shaders/dx11/vs_mudl.bin -p s_5_0
bin\shaderc -f src/fs_mudl.sc --type fragment --platform windows -o bin/shaders/dx11/fs_mudl.bin -p s_5_0
