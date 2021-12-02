#pragma once
#include <common/memory.h>
#include <fmt/core.h>
#include <memory>
#include <vector>
#include <fstream>

namespace ee {
    class EmotionEngine;
}

namespace iop {
    class IOProcessor;
}

enum Component
{
    EE = 0x0,
    IOP = 0x1
};

/* This class act as the "motherboard" of sorts */
class ComponentManager {
public:
    ComponentManager();
    ~ComponentManager();

    void tick();

    /* Memory operations */
    template <typename T>
    T read(uint32_t addr, Component comp);
    
    template <typename T>
    void write(uint32_t addr, T data, Component comp);

protected:
    void read_bios();

public:
    /* Components */
    std::unique_ptr<ee::EmotionEngine> ee;
    std::unique_ptr<iop::IOProcessor> iop;
    
    /* Memory - Registers */
    uint8_t* ram, * iop_ram, * bios;

    uint32_t MCH_RICM = 0, MCH_DRD = 0;
    uint8_t rdram_sdevid = 0;

    /* Utilities */
    const char* component_name[2] = { "EE", "IOP" };
    std::atomic_bool stop_thread = false;
    std::ofstream console;
};

/* Instanciate the templates here so we can limit their types below */
template <typename T>
T ComponentManager::read(uint32_t addr, Component comp)
{
    uint32_t vaddr = addr & KUSEG_MASKS[addr >> 29];

    /* Check if it's any memory ranges we care */
    if (ee::RAM.contains(vaddr))
        return *(T*)&ram[ee::RAM.offset(vaddr)];
    else if (ee::BIOS.contains(vaddr))
        return *(T*)&bios[ee::BIOS.offset(vaddr)];
    else if (iop::RAM.contains(vaddr))
        return *(T*)&iop_ram[iop::RAM.offset(vaddr)];
    else /* Otherwise handle specific address diferently. */
    {
        switch (vaddr)
        {
        case 0x1000f130:
        case 0x1000f430:
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
            fmt::print("[{}] {:d}bit read from unknown address {:#x}\n", component_name[comp], sizeof(T) * 8, addr);
            return 0;
        }
    }
}

/* Instanciate the templates here so we can limit their types below */
template <typename T>
void ComponentManager::write(uint32_t addr, T data, Component comp)
{
    uint32_t vaddr = addr & KUSEG_MASKS[addr >> 29];

    /* Check if it's any memory ranges we care */
    if (ee::RAM.contains(vaddr))
        *(T*)&ram[ee::RAM.offset(vaddr)] = data;
    else if (iop::RAM.contains(vaddr))
        *(T*)&iop_ram[iop::RAM.offset(vaddr)] = data;
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
            fmt::print("[{}] {:d}bit write {:#x} to unknown address {:#x}\n", component_name[comp], sizeof(T) * 8, data, addr);
            break;
        };
    }
}