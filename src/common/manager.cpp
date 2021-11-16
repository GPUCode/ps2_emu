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
    switch (vaddr)
    {
    case 0x1000f410:
        return 0;
    case 0x1000f440:
    {
        uint8_t SOP = (MCH_RICM >> 6) & 0xF;
        uint8_t SA = (MCH_RICM >> 16) & 0xFFF;
        if (!SOP)
        {
            switch (SA)
            {
            case 0x21:
                if (rdram_sdevid < 2)
                {
                    rdram_sdevid++;
                    return 0x1F;
                }
                return 0;
            case 0x23:
                return 0x0D0D;
            case 0x24:
                return 0x0090;
            case 0x40:
                return MCH_RICM & 0x1F;
            }
        }
        return 0;
    }
    default:
        return *(T*)&memory[vaddr];
    }
}

/* Instanciate the templates here so we can limit their types below */
template <typename T>
void ComponentManager::write(uint32_t addr, T data)
{
    uint32_t vaddr = addr & KUSEG_MASKS[addr >> 29];
    switch (vaddr)
    {
    /* Record any console output */
    case 0x1000f180:
    {
        console << (char)data;
        console.flush();
        break;
    }
    case 0x1000f430:
    {
        uint8_t SA = (data >> 16) & 0xFFF;
        uint8_t SBC = (data >> 6) & 0xF;

        if (SA == 0x21 && SBC == 0x1 && ((MCH_DRD >> 7) & 1) == 0)
            rdram_sdevid = 0;

        MCH_RICM = data & ~0x80000000;
        break;
    }
    case 0x1000f440:
    {
        MCH_DRD = data;
        break;
    }
    default:
        *(T*)&memory[vaddr] = data;
        break;
    };
}

/* Template definitions. */
template uint32_t ComponentManager::read<uint32_t>(uint32_t);
template uint64_t ComponentManager::read<uint64_t>(uint32_t);
template uint8_t ComponentManager::read<uint8_t>(uint32_t);
template uint16_t ComponentManager::read<uint16_t>(uint32_t);
template void ComponentManager::write<uint32_t>(uint32_t, uint32_t);
template void ComponentManager::write<uint64_t>(uint32_t, uint64_t);
template void ComponentManager::write<uint8_t>(uint32_t, uint8_t);
template void ComponentManager::write<uint16_t>(uint32_t, uint16_t);