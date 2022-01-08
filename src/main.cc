#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <common/emulator.h>
#include <thread>

int main()
{
    common::Emulator* emulator = new common::Emulator;
    while (!emulator->stop) 
    { 
        emulator->tick(); 
    }

    delete emulator;
    return 0;
}