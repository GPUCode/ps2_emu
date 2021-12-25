#pragma once
#include <cstdint>

namespace iop
{
    struct INTRRegs
    {
        uint32_t i_stat;
        uint32_t i_mask;
        uint32_t i_ctrl;
    };

    enum class Interrupt : uint32_t
    {
        VBLANKBegin = 0,
        GPU = 1,
        CDVD = 2,
        DMA = 3,
        Timer0 = 4,
        Timer1 = 5,
        Timer2 = 6,
        SIO0 = 7,
        SIO1 = 8,
        SPU2 = 9,
        PIO = 10,
        VBLANKEnd = 11,
        PCMCIA = 13,
        Timer3 = 14,
        Timer4 = 15,
        Timer5 = 16,
        SIO2 = 17,
    };

    class IOProcessor;
	struct INTR
	{
        INTR(IOProcessor* parent);
        ~INTR() = default;

        uint64_t read(uint32_t addr);
        void write(uint32_t addr, uint64_t data);

        void trigger(Interrupt intr);
        bool interrupt_pending();

    private:
        IOProcessor* iop;
        INTRRegs regs = {};
	};
}