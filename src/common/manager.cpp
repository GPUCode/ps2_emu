#include <common/manager.hpp>
#include <fstream>
#include <iostream>
#include <memory>

ComponentManager::ComponentManager()
{
    ee = std::make_unique<EmotionEngine>(this);
}

void ComponentManager::read_bios()
{  
    /* Yes it's hardcoded for now, don't bite me, I'll change it eventually */
    std::ifstream reader;
    reader.open("/home/emufan/Desktop/gcnemu/build/bin/bios.bin", std::ios::in | std::ios::binary);

    if (!reader.is_open())
        exit(1);
    
    reader.seekg(0);
    reader.read((char*)bios.data(), 4 * 1024 * 1024);
    reader.close();
}

void ComponentManager::tick()
{
    ee->tick();
}