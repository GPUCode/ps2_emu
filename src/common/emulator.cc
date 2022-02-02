#include <common/emulator.h>
#include <common/sif.h>
#include <cpu/ee/ee.h>
#include <cpu/ee/intc.h>
#include <cpu/ee/dmac.h>
#include <cpu/iop/iop.h>
#include <cpu/iop/intr.h>
#include <cpu/vu/vu.h>
#include <cpu/vu/vif.h>
#include <gs/gif.h>
#include <gs/gs.h>
#include <media/ipu.h>
#include <spu/spu.h>
#include <media/cdvd.h>
#include <media/sio2.h>
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
        gif = std::make_unique<gs::GIF>(this);
        gs = std::make_unique<gs::GraphicsSynthesizer>(this);
        dmac = std::make_unique<ee::DMAController>(this);
        vu[0] = std::make_unique<vu::VectorUnit>(ee.get());
        vu[1] = std::make_unique<vu::VectorUnit>(ee.get());
        vif[0] = std::make_unique<vu::VIF>(this, 0);
        vif[1] = std::make_unique<vu::VIF>(this, 1);
        ipu = std::make_unique<media::IPU>(this);
        sif = std::make_unique<common::SIF>(this);
        spu2 = std::make_unique<spu::SPU>(this);
        cdvd = std::make_unique<media::CDVD>(this);
        sio2 = std::make_unique<media::SIO2>(this);

        /* Initialize console */
        console.open("console.txt", std::ios::out);
    }

    Emulator::~Emulator()
    {
        /* Clean up handler table */
        for (auto ptr : handlers)
        {
            if (ptr != nullptr)
                delete ptr;
        }
        
        /* Cleanup BIOS memory */
        delete[] bios;
    }

    void Emulator::print(char c)
    {
        console << c;
        console.flush();
    }

    const uint32_t Emulator::calculate_page(const uint32_t addr)
    {
        /* Ensure that byte is 0 for everything! (exclude 0x1ffe* addresses) */
        assert((addr & 0x000f0000) == 0 || (addr & 0x1fff0000) == 0x1ffe0000);

        /* Optimize our address to shrink our handler array size */
        uint32_t opt_addr = ((addr & 0x0ff00000) >> 4) | (addr & 0xfffff);
        return opt_addr / HANDLER_PAGE_SIZE;
    }

    bool Emulator::load_elf(const char* filename)
    {
        /* ELF header structure */
        struct Elf32_Ehdr 
        {
            uint8_t  e_ident[16];
            uint16_t e_type;
            uint16_t e_machine;
            uint32_t e_version;
            uint32_t e_entry;
            uint32_t e_phoff;
            uint32_t e_shoff;
            uint32_t e_flags;
            uint16_t e_ehsize;
            uint16_t e_phentsize;
            uint16_t e_phnum;
            uint16_t e_shentsize;
            uint16_t e_shnum;
            uint16_t e_shstrndx;
        };

        /* Program header */
        struct Elf32_Phdr 
        {
            uint32_t p_type;
            uint32_t p_offset;
            uint32_t p_vaddr;
            uint32_t p_paddr;
            uint32_t p_filesz;
            uint32_t p_memsz;
            uint32_t p_flags;
            uint32_t p_align;
        };

        std::ifstream reader;
        reader.open(filename, std::ios::in | std::ios::binary);

        if (reader.is_open())
        {
            reader.seekg(0, std::ios::end);
            int size = reader.tellg();
            uint8_t* buffer = new uint8_t[size];

            reader.seekg(0, std::ios::beg);
            reader.read((char*)buffer, size);
            reader.close();
            
            auto header = *(Elf32_Ehdr*)&buffer[0];
            fmt::print("[CORE][ELF] Loading {}\n", filename);
            fmt::print("Entry: {:#x}\n", header.e_entry);
            fmt::print("Program header start: {:#x}\n", header.e_phoff);
            fmt::print("Section header start: {:#x}\n", header.e_shoff);
            fmt::print("Program header entries: {:d}\n", header.e_phnum);
            fmt::print("Section header entries: {:d}\n", header.e_shnum);
            fmt::print("Section header names index: {:d}\n", header.e_shstrndx);

            for (auto i = header.e_phoff; i < header.e_phoff + (header.e_phnum * 0x20); i += 0x20)
            {
                auto pheader = *(Elf32_Phdr*)&buffer[i];
                fmt::print("\nProgram header\n");
                fmt::print("p_type: {:#x}\n", pheader.p_type);
                fmt::print("p_offset: {:#x}\n", pheader.p_offset);
                fmt::print("p_vaddr: {:#x}\n", pheader.p_vaddr);
                fmt::print("p_paddr: {:#x}\n", pheader.p_paddr);
                fmt::print("p_filesz: {:#x}\n", pheader.p_filesz);
                fmt::print("p_memsz: {:#x}\n", pheader.p_memsz);

                int mem_w = pheader.p_paddr;
                for (auto file_w = pheader.p_offset; file_w < (pheader.p_offset + pheader.p_filesz); file_w += 4)
                {
                    uint32_t word = *(uint32_t*)&buffer[file_w];
                    ee->write<uint32_t>(mem_w, word);
                    mem_w += 4;
                }
            }
            
            ee->pc = header.e_entry;
            ee->direct_jump();

            return true;
        }

        return false;
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
        uint32_t total_cycles = 0;
        bool vblank_started = false;
        while (total_cycles < CYCLES_PER_FRAME)
        {
            uint32_t cycles = CYCLES_PER_TICK;

            /* Tick componets that run at EE speed */
            ee->tick(cycles);

            /* Tick bus components */
            cycles /= 2;
            dmac->tick(cycles);
            vif[0]->tick(cycles);
            vif[1]->tick(cycles);
            gif->tick(cycles);

            /* Tick IOP components */
            cycles /= 4;
            iop->tick(cycles);
            iop_dma->tick(cycles);

            total_cycles += CYCLES_PER_TICK;

            if (!vblank_started && total_cycles >= CYCLES_VBLANK_OFF)
            {
                vblank_started = true;
                gs->priv_regs.csr.vsint = true;
                gs->renderer.render();
                ee->intc.trigger(ee::Interrupt::INT_VB_ON);
                iop->intr.trigger(iop::Interrupt::VBLANKBegin);
            }
        }

        /* VBlank end */
        iop->intr.trigger(iop::Interrupt::VBLANKEnd);
        ee->intc.trigger(ee::Interrupt::INT_VB_OFF);
        vblank_started = false;
        gs->priv_regs.csr.vsint = false;
    }
}