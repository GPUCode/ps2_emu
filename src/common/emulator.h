#pragma once
#include <common/component.h>
#include <cpu/iop/dma.h>
#include <cpu/vu/vu.h>
#include <fmt/format.h>
#include <memory>
#include <fstream>
#include <type_traits>

namespace ee 
{
    class EmotionEngine;
    struct DMAController;
}

namespace iop 
{
    class IOProcessor;
}

namespace gs
{
    struct GIF;
    struct GraphicsSynthesizer;
}

namespace vu
{
    struct VectorUnit;
    struct VIF;
}

namespace media
{
    struct IPU;
    struct CDVD;
    struct SIO2;
}

namespace spu
{
    struct SPU;
}

namespace common
{
    constexpr uint32_t KUSEG_MASKS[8] = 
    {
        /* KUSEG: Don't touch the address, it's fine */
        0xffffffff, 0x1fffffff, 0x1fffffff, 0xffffffff,
        /* KSEG0: Strip the MSB (0x8 -> 0x0 and 0x9 -> 0x1) */
        0x7fffffff,
        /* KSEG1: Strip the 3 MSB's (0xA -> 0x0 and 0xB -> 0x1) */
        0x1fffffff,
        /* KSEG2: Don't touch the address, it's fine */
        0xffffffff, 0x1fffffff,
    };

    /* These were measured from my PAL PS2 Slim and more specifically:
        VBLANK ON for 421376 EE cycles and 22 HBLANKS 
        VBLANK OFF for 4498432 EE cycles and 248 HBLANKS */
    constexpr uint32_t CYCLES_PER_FRAME = 4919808; /* VBLANK ON + VBLANK OFF */
    constexpr uint32_t CYCLES_VBLANK_ON = 421376;
    constexpr uint32_t CYCLES_VBLANK_OFF = 4498432;
    constexpr uint32_t CYCLES_PER_TICK = 32;

    enum ComponentID
    {
        EE = 0x0,
        IOP = 0x1
    };

    /* This class act as the "motherboard" of sorts */
    class SIF;
    class Emulator
    {
    public:
        Emulator();
        ~Emulator();

        void tick();

        /* Memory operations */
        template <typename T, ComponentID id>
        T read(uint32_t addr);

        template <typename T, ComponentID id>
        void write(uint32_t addr, T data);

        /* Prints a character to our console */
        void print(char c);

        /* Handler interface */
        template <typename T = uint32_t, typename R, typename W>
        void add_handler(uint32_t address, Component* c, R reader, W writer);

        static const uint32_t calculate_page(const uint32_t addr);

    protected:
        void read_bios();

    public:
        /* Components */
        std::unique_ptr<ee::EmotionEngine> ee;
        std::unique_ptr<iop::IOProcessor> iop;
        std::unique_ptr<iop::DMAController> iop_dma;
        std::unique_ptr<gs::GIF> gif;
        std::unique_ptr<gs::GraphicsSynthesizer> gs;
        std::unique_ptr<ee::DMAController> dmac;
        std::unique_ptr<vu::VectorUnit> vu[2];
        std::unique_ptr<vu::VIF> vif[2];
        std::unique_ptr<media::IPU> ipu;
        std::unique_ptr<common::SIF> sif;
        std::unique_ptr<spu::SPU> spu2;
        std::unique_ptr<media::CDVD> cdvd;
        std::unique_ptr<media::SIO2> sio2;

        /* Memory - Registers */
        uint8_t* bios;

        uint32_t MCH_RICM = 0, MCH_DRD = 0;
        uint8_t rdram_sdevid = 0;

        /* Handlers */
        HandlerBase* handlers[0x20000] = {};

        /* Utilities */
        const char* component_name[2] = { "EE", "IOP" };
        bool stop = false;
        std::ofstream console;
    };

    /* Instanciate the templates here so we can limit their types below */
    template <typename T, ComponentID id>
    inline T Emulator::read(uint32_t paddr)
    {
        if (paddr >= 0x1fc00000) /* Read BIOS */
        {
            return *(T*)&bios[paddr - 0x1fc00000];
        }
        else /* Otherwise handle specific address diferently. */
        {
            auto page = Emulator::calculate_page(paddr);
            auto handler = (Handler<T>*)handlers[page];

            if (handler)
            {
                return (*handler)(paddr);
            }
            else
            {
                fmt::print("[{}] {:d}bit read from unknown address {:#x}\n", component_name[id], sizeof(T) * 8, paddr);
                return 0;
            }
        }
    }

    /* Instanciate the templates here so we can limit their types below */
    template <typename T, ComponentID id>
    inline void Emulator::write(uint32_t paddr, T data)
    {
        if (paddr >= 0x11000000 && paddr < 0x11008000)
        {
            vu[0]->write(paddr, data);
        }
        else if (paddr >= 0x1fff8000 && paddr < 0x20000000)
        {
            *(T*)&bios[paddr & 0x3fffff] = data;
        }
        else
        {
            auto page = Emulator::calculate_page(paddr);
            auto handler = (Handler<T>*)handlers[page];

            if (handler)
            {
                return (*handler)(paddr, data);
            }

            if constexpr (std::is_same<T, uint128_t>::value)
            {
                uint64_t upper = (data >> 64);
                fmt::print("[{}] {:d}bit write {:#x}{:016x} to unknown address {:#x}\n", component_name[id], sizeof(T) * 8, upper, (uint64_t)data, paddr);
            }
            else
            {
                fmt::print("[{}] {:d}bit write {:#x} to unknown address {:#x}\n", component_name[id], sizeof(T) * 8, data, paddr);
            }
        }
    }
    
    template <typename T, typename R, typename W>
    inline void Emulator::add_handler(uint32_t address, Component* c, R reader, W writer)
    {
        uint32_t page = calculate_page(address);

        auto h = new Handler<T>;
        h->c = c;
        h->writer = (Writer<T>)writer;
        h->reader = (Reader<T>)reader;
        handlers[page] = h;
    }
}