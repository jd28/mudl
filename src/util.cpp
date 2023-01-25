#include "util.hpp"

#include <bgfx/bgfx.h>
#include <bx/bx.h>

std::filesystem::path get_shader_path()
{
    std::filesystem::path path;
    switch (bgfx::getRendererType()) {
    case bgfx::RendererType::Noop:
    case bgfx::RendererType::Direct3D9:
        path = "shaders/dx9/";
        break;
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12:
        path = "shaders/dx11/";
        break;
    case bgfx::RendererType::Agc:
    case bgfx::RendererType::Gnm:
        path = "shaders/pssl/";
        break;
    case bgfx::RendererType::Metal:
        path = "shaders/metal/";
        break;
    case bgfx::RendererType::Nvn:
        path = "shaders/nvn/";
        break;
    case bgfx::RendererType::OpenGL:
        path = "shaders/glsl/";
        break;
    case bgfx::RendererType::OpenGLES:
        path = "shaders/essl/";
        break;
    case bgfx::RendererType::Vulkan:
        path = "shaders/spirv/";
        break;
    case bgfx::RendererType::WebGPU:
        path = "shaders/spirv/";
        break;

    case bgfx::RendererType::Count:
        BX_ASSERT(false, "You should not be here!");
        break;
    }
    return path;
}
