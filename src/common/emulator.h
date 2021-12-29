#pragma once
#include <common/component.h>
#include <cpu/iop/dma.h>
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

namespace common
{
    constexpr uint32_t KUSEG_MASKS[8] = 
    {
        /* KUSEG: Don't touch the address, it's fine */
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        /* KSEG0: Strip the MSB (0x8 -> 0x0 and 0x9 -> 0x1) */
        0x7fffffff,
        /* KSEG1: Strip the 3 MSB's (0xA -> 0x0 and 0xB -> 0x1) */
        0x1fffffff,
        /* KSEG2: Don't touch the address, it's fine */
        0xffffffff, 0xffffffff,
    };

    enum ComponentID
    {
        EE = 0x0,
        IOP = 0x1
    };

    /* This class act as the "motherboard" of sorts */
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

        /* Memory - Registers */
        uint8_t* bios;

        uint32_t MCH_RICM = 0, MCH_DRD = 0;
        uint8_t rdram_sdevid = 0;

        /* Handlers */
        HandlerBase* handlers[0x20000] = {};

        /* Utilities */
        const char* component_name[2] = { "EE", "IOP" };
        std::atomic_bool stop_thread = false;
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
        auto page = Emulator::calculate_page(paddr);
        auto handler = (Handler<T>*)handlers[page];

        if (handler)
        {
            return (*handler)(paddr, data);
        }
        
        if constexpr (std::is_same<T, unsigned __int128>::value)
        {
            uint64_t upper = (data >> 64);
            fmt::print("[{}] {:d}bit write {:#x}{:016x} to unknown address {:#x}\n", component_name[id], sizeof(T) * 8, upper, (uint64_t)data, paddr);
        }
        else
        {
            fmt::print("[{}] {:d}bit write {:#x} to unknown address {:#x}\n", component_name[id], sizeof(T) * 8, data, paddr);
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