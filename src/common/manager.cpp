#include <common/manager.hpp>
#include <common/memmap.hpp>
#include <fstream>
#include <fmt/core.h>

std::ofstream console("console.txt", std::ios::out);

ComponentManager::ComponentManager()
{
    ee = std::make_unique<EmotionEngine>(this);
    
    /* Allocate the entire 32MB of RAM */
    ram = new uint8_t[RAM.length];
    bios = new uint8_t[BIOS.length];
    std::memset(ram, 0, RAM.length);

    /* Load the BIOS in our memory */
    /* NOTE: Must make a GUI for this someday */
    this->read_bios();
}

ComponentManager::~ComponentManager()
{
    delete[] ram;
    delete[] bios;
}

void ComponentManager::read_bios()
{  
    /* Yes it's hardcoded for now, don't bite me, I'll change it eventually */
    std::ifstream reader;
    reader.open("SCPH-10000.BIN", std::ios::in | std::ios::binary);

    if (!reader.is_open())
        exit(1);
    
    reader.seekg(0);
    reader.read((char*)bios, BIOS.length);
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
    
    /* Check if it's any memory ranges we care */
    if (RAM.contains(vaddr))
        return *(T*)&ram[RAM.offset(vaddr)];
    else if (BIOS.contains(vaddr))
        return *(T*)&bios[BIOS.offset(vaddr)];
    else /* Otherwise handle specific address diferently. */
    {
        switch (vaddr)
        {
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
            fmt::print("[WARNING] {:d}bit read from unknown address {:#x}\n", sizeof(T) * 8, addr);
            return 0;
        }
    }
}

/* Instanciate the templates here so we can limit their types below */
template <typename T>
void ComponentManager::write(uint32_t addr, T data)
{
    uint32_t vaddr = addr & KUSEG_MASKS[addr >> 29];

    /* Check if it's any memory ranges we care */
    if (RAM.contains(vaddr))
        *(T*)&ram[RAM.offset(vaddr)] = data;
    else
    {
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
            fmt::print("[WARNING] {:d}bit write {:#x} to unknown address {:#x}\n", sizeof(T) * 8, data, addr);
            break;
        };
    }
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