#include <common/manager.hpp>
#include <common/memmap.hpp>
#include <fstream>
#include <iostream>
#include <memory>

ComponentManager::ComponentManager() :
    memory(MEMORY_RANGE, 0)
{
    ee = std::make_unique<EmotionEngine>(this);

    /* Load the BIOS in our memory */
    /* NOTE: Must make a GUI for this someday */
    this->read_bios();
}

void ComponentManager::read_bios()
{  
    /* Yes it's hardcoded for now, don't bite me, I'll change it eventually */
    std::ifstream reader;
    reader.open("/home/emufan/Desktop/gcnemu/build/bin/bios.bin", std::ios::in | std::ios::binary);

    if (!reader.is_open())
        exit(1);
    
    reader.seekg(0);
    reader.read((char*)memory.data() + BIOS.start, BIOS.length);
    reader.close();
}

void ComponentManager::tick_components()
{
    ee->tick();
}

/* Instanciate the templates here so we can limit their types below */
template <typename T>
T ComponentManager::read_memory(uint32_t addr)
{
    uint32_t vaddr = addr & KUSEG_MASKS[addr >> 29];
    auto ptr = (T*)(memory.data() + vaddr);

    return ptr[0];
}

/* Instanciate the templates here so we can limit their types below */
template <typename T>
void ComponentManager::write_memory(uint32_t addr, T data)
{
    uint32_t vaddr = addr & KUSEG_MASKS[addr >> 29];
    auto ptr = (T*)(memory.data() + vaddr);

    ptr[0] = data;
}

/* Template definitions. */
template uint32_t ComponentManager::read_memory<uint32_t>(uint32_t);
template void ComponentManager::write_memory<uint32_t>(uint32_t, uint32_t);