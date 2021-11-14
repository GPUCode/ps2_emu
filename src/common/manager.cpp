#include <common/manager.hpp>
#include <common/memmap.hpp>
#include <fstream>
#include <iostream>
#include <memory>

ComponentManager::ComponentManager()
{
    ee = std::make_unique<EmotionEngine>(this);
    
    /* Allocate the entire 32MB of RAM */
    memory = new uint8_t[MEMORY_RANGE];

    /* Load the BIOS in our memory */
    /* NOTE: Must make a GUI for this someday */
    this->read_bios();
}

ComponentManager::~ComponentManager()
{
    delete[] memory;
}

void ComponentManager::read_bios()
{  
    /* Yes it's hardcoded for now, don't bite me, I'll change it eventually */
    std::ifstream reader;
    reader.open("SCPH-30003.BIN", std::ios::in | std::ios::binary);

    if (!reader.is_open())
        exit(1);
    
    reader.seekg(0);
    reader.read((char*)memory + BIOS.start, BIOS.length);
    reader.close();
}

void ComponentManager::tick()
{
    ee->tick();
}

/* Instanciate the templates here so we can limit their types below */
template <typename T>
T ComponentManager::read(uint32_t addr)
{
    uint32_t vaddr = addr & KUSEG_MASKS[addr >> 29];
    T data = *(T*)&memory[vaddr];
    return data;
}

/* Instanciate the templates here so we can limit their types below */
template <typename T>
void ComponentManager::write(uint32_t addr, T data)
{
    uint32_t vaddr = addr & KUSEG_MASKS[addr >> 29];
    *(T*)&memory[vaddr] = data;
}

/* Template definitions. */
template uint32_t ComponentManager::read<uint32_t>(uint32_t);
template uint64_t ComponentManager::read<uint64_t>(uint32_t);
template uint8_t ComponentManager::read<uint8_t>(uint32_t);
template void ComponentManager::write<uint32_t>(uint32_t, uint32_t);
template void ComponentManager::write<uint64_t>(uint32_t, uint64_t);
template void ComponentManager::write<uint8_t>(uint32_t, uint8_t);