#include <glad/glad.h>
#include <common/emulator.h>
#include <stdexcept>
#include <gs/vulkan/window.h>

int main()
{
    VkWindow window(800, 600, "PS2");
    common::Emulator emulator(&window);

    try
    {
        while (!window.should_close())
        {
            window.begin_frame();
            emulator.tick();
            window.end_frame();
        }

    }
    catch (std::exception& e)
    {
        fmt::print("{}\n", e.what());
    }

    return 0;
}

