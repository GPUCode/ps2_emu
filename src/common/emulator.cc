#include <common/emulator.h>
#include <cpu/ee/ee.h>
#include <cpu/iop/iop.h>
#include <cassert>

int cycles_executed = 0;

namespace common
{
    Emulator::Emulator()
    {
        /* Allocate the entire 32MB of RAM */
        bios = new uint8_t[4 * 1024 * 1024];

        /* Load the BIOS in our memory */
        /* NOTE: Must make a GUI for this someday */
        read_bios();

        /* Finally construct our components */
        ee = std::make_unique<ee::EmotionEngine>(this);
        iop = std::make_unique<iop::IOProcessor>(this);
        iop_dma = std::make_unique<iop::DMAController>(this);
    }

    Emulator::~Emulator()
    {
        delete[] bios;
    }

    const uint32_t Emulator::calculate_page(const uint32_t addr)
    {
        /* Ensure that byte is 0 for everything! (exclude 0xfff* ranges) */
        assert((addr & 0x000f0000) == 0 || (addr & 0xffff0000) == 0xfffe0000);

        /* Optimize our address to shrink our handler array size */
        uint32_t opt_addr = ((addr & 0x0ff00000) >> 4) | (addr & 0xfffff);
        return opt_addr / HANDLER_PAGE_SIZE;
    }

    void Emulator::read_bios()
    {
        /* Yes it's hardcoded for now, don't bite me, I'll change it eventually */
        std::ifstream reader;
        reader.open("SCPH-10000.BIN", std::ios::in | std::ios::binary);

        if (!reader.is_open())
            std::abort();

        reader.seekg(0);
        reader.read((char*)bios, 4 * 1024 * 1024);
        reader.close();
    }

    void Emulator::tick()
    {
        ee->tick();
        cycles_executed++;

        if (cycles_executed % 8 == 0)
            iop->tick();
    }
}