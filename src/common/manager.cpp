#include <common/manager.hpp>
#include <common/memmap.hpp>
#include <fstream>

std::ofstream console("console.txt", std::ios::out);

ComponentManager::ComponentManager()
{
    ee = std::make_unique<EmotionEngine>(this);
    
    /* Allocate the entire 32MB of RAM */
    memory = new uint8_t[MEMORY_RANGE];
    std::memset(memory, 0, MEMORY_RANGE);

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
    reader.open("SCPH-10000.BIN", std::ios::in | std::ios::binary);

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

    if (addr == 0xb000f180) /* Record any console output */
        console << (char)data;
}

/* Template definitions. */
template uint32_t ComponentManager::read<uint32_t>(uint32_t);
template uint64_t ComponentManager::read<uint64_t>(uint32_t);
template uint8_t ComponentManager::read<uint8_t>(uint32_t);
template uint16_t ComponentManager::read<uint16_t>(uint32_t);
template void ComponentManager::write<uint32_t>(uint32_t, uint32_t);
template void ComponentManager::write<uint64_t>(uint32_t, uint64_t);
template void ComponentManager::write<uint8_t>(uint32_t, uint8_t);