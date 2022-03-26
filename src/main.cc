#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <common/emulator.h>
#include <stdexcept>
#include <gs/vulkan/window.h>
#include <gs/vulkan/context.h>
#include <gs/vulkan/buffer.h>

int main()
{
    VkWindow window(800, 600, "PS2");
    common::Emulator* emulator = new common::Emulator(&window);

    try
    {
        while (!window.should_close())
        {
            window.begin_frame();
            emulator->tick();
            window.end_frame();
        }

    }
    catch (std::exception& e)
    {
        fmt::print("{}\n", e.what());
    }

    window.destroy();
    delete emulator;
    return 0;
}
