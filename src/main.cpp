#include "model.hpp"

#include "util.hpp"

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

    nw::Image texture{nw::kernel::resman().demand({"c_cat_dire"sv, nw::ResourceType::tga})};
    auto size = texture.channels() * texture.height() * texture.width();
    auto mem = bgfx::makeRef(texture.data(), size);
    auto tex_handle = bgfx::createTexture2D(uint16_t(texture.width()), uint16_t(texture.height()), false, 1,
        texture.channels() == 4 ? bgfx::TextureFormat::RGBA8 : bgfx::TextureFormat::RGB8,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, mem);

    auto model = load_model(&mdl.model, tex_handle);
    if (!model) {
        LOG_F(FATAL, "uanble to load model.");
    }

    bool exit = false;
    SDL_Event event;
    while (!exit) {
        while (SDL_PollEvent(&event)) {

            switch (event.type) {
            case SDL_QUIT:
                exit = true;
                break;

            case SDL_WINDOWEVENT: {
                const SDL_WindowEvent& wev = event.window;
                switch (wev.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    break;

                case SDL_WINDOWEVENT_CLOSE:
                    exit = true;
                    break;
                }
            } break;
            }
        }
        bgfx::touch(0);
        const bx::Vec3 at = {0.0f, 1.0f, 0.0f};
        const bx::Vec3 eye = {5.0f, 2.0f, -6.0f};

        // Set view and projection matrix for view 0.
        {
            float view[16];
            bx::mtxLookAt(view, eye, at);

            float proj[16];
            bx::mtxProj(proj, 60.0f, float(width) / float(height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
            bgfx::setViewTransform(0, view, proj);

            // Set view 0 default viewport.
            bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
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
