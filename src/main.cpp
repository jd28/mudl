#include "model.hpp"

#include "util.hpp"

#include "bgfx-imgui/imgui_impl_bgfx.h"
#include "imgui.h"
#include "sdl-imgui/imgui_impl_sdl.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <bx/math.h>
#include <bx/mutex.h>
#include <bx/thread.h>
#include <glm/gtx/normal.hpp>
#include <nw/kernel/Kernel.hpp>
#include <nw/kernel/Resources.hpp>
#include <nw/legacy/Image.hpp>
#include <nw/model/Mdl.hpp>

#include <string>
#include <vector>

using namespace std::literals;

int counter = 0;
int main(int argc, char** argv)
{
    nw::init_logger(argc, argv);
    auto info = nw::probe_nwn_install();
    nw::kernel::config().initialize({
        info.version,
        info.install,
        info.user,
    });
    nw::kernel::resman().add_container(new nw::Directory("assets"));
    nw::kernel::services().start();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    const int width = 800;
    const int height = 600;
    SDL_Window* window = SDL_CreateWindow(
        argv[0], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
        height, SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        printf("Window could not be created. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    Node::layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

#if !BX_PLATFORM_EMSCRIPTEN
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window, &wmi)) {
        printf(
            "SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n",
            SDL_GetError());
        return 1;
    }
    bgfx::renderFrame(); // single threaded mode
#endif                   // !BX_PLATFORM_EMSCRIPTEN

    bgfx::PlatformData pd{};
#if BX_PLATFORM_WINDOWS
    pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_OSX
    pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_LINUX
    pd.ndt = wmi.info.x11.display;
    pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif BX_PLATFORM_EMSCRIPTEN
    pd.nwh = (void*)"#canvas";
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
       // BX_PLATFORM_EMSCRIPTEN

    bgfx::Init bgfx_init;
    bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
    bgfx_init.resolution.width = width;
    bgfx_init.resolution.height = height;
    bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx_init.platformData = pd;
    bgfx::init(bgfx_init);

    ImGui::CreateContext();

    ImGui_Implbgfx_Init(255);
#if BX_PLATFORM_WINDOWS
    ImGui_ImplSDL2_InitForD3D(window);
#elif BX_PLATFORM_OSX
    ImGui_ImplSDL2_InitForMetal(window);
#elif BX_PLATFORM_LINUX || BX_PLATFORM_EMSCRIPTEN
    ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
       // BX_PLATFORM_EMSCRIPTEN

    nw::ByteArray vs_mudl_bytes = nw::ByteArray::from_file("vs_mudl.bin");
    if (vs_mudl_bytes.size() == 0) {
        return 1;
    }
    nw::ByteArray fs_mudl_bytes = nw::ByteArray::from_file("fs_mudl.bin");
    if (fs_mudl_bytes.size() == 0) {
        return 1;
    }

    auto vs_mudl_shd_handle = bgfx::createShader(bgfx::makeRef(vs_mudl_bytes.data(),
        uint32_t(vs_mudl_bytes.size())));
    auto fs_mudl_shd_handle = bgfx::createShader(bgfx::makeRef(fs_mudl_bytes.data(),
        uint32_t(fs_mudl_bytes.size())));

    auto program = bgfx::createProgram(vs_mudl_shd_handle, fs_mudl_shd_handle, true);
    bgfx::setViewClear(
        0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xD3D3D3FF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, width, height);

    // Proof of concept, one hardcoded model and texture, obviously this is stupid.
    auto bytes = nw::kernel::resman().demand({"dire_cat"sv, nw::ResourceType::mdl});
    if (bytes.size() == 0) {
        LOG_F(FATAL, "Failed to load model");
    }
    nw::model::Mdl mdl{std::move(bytes)};
    if (!mdl.valid()) { return 1; }

    auto model = load_model(&mdl.model);
    if (!model) {
        LOG_F(FATAL, "uanble to load model.");
    }

    int prev_mouse_x = 0;
    int prev_mouse_y = 0;
    float cam_pitch = 0.0f;
    float cam_yaw = 0.0f;
    float rot_scale = 0.01f;

    bool exit = false;
    while (!exit) {
        for (SDL_Event currentEvent; SDL_PollEvent(&currentEvent) != 0;) {
            ImGui_ImplSDL2_ProcessEvent(&currentEvent);
            if (currentEvent.type == SDL_QUIT) {
                exit = true;
                break;
            }
        }

        ImGui_Implbgfx_NewFrame();
        ImGui_ImplSDL2_NewFrame();

        ImGui::NewFrame();
        ImGui::ShowDemoWindow(); // your drawing here
        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

        if (!ImGui::GetIO().WantCaptureMouse) {
            // simple input code for orbit camera
            int mouse_x, mouse_y;
            const int buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
            if ((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0) {
                int delta_x = mouse_x - prev_mouse_x;
                int delta_y = mouse_y - prev_mouse_y;
                cam_yaw += float(-delta_x) * rot_scale;
                cam_pitch += float(-delta_y) * rot_scale;
            }
            prev_mouse_x = mouse_x;
            prev_mouse_y = mouse_y;
        }

        bgfx::touch(0);

        // Set view and projection matrix for view 0.
        {
            float cam_rotation[16];
            bx::mtxRotateXYZ(cam_rotation, cam_pitch, cam_yaw, 0.0f);

            float cam_translation[16];
            bx::mtxTranslate(cam_translation, 0.0f, 0.0f, -5.0f);

            float cam_transform[16];
            bx::mtxMul(cam_transform, cam_translation, cam_rotation);

            float view[16];
            bx::mtxInverse(view, cam_transform);

            float proj[16];
            bx::mtxProj(proj, 60.0f, float(width) / float(height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
            bgfx::setViewTransform(0, view, proj);
        }

        glm::mat4 mtx = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), {1.0f, 0.0f, 0.0f});
        model->submit(0, program, mtx);

        bgfx::frame();
    }

    bgfx::shutdown();

    while (bgfx::RenderFrame::NoContext != bgfx::renderFrame()) {
    };

    SDL_DestroyWindow(window);
    SDL_Quit();
}
