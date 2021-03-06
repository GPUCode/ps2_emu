#pragma once
#include <common/emulator.h>
#include <cpu/ee/cop0.h>
#include <cpu/ee/cop1.h>
#include <cpu/ee/intc.h>
#include <cpu/ee/timers.h>
#include <cpu/ee/jit/jit.h>

namespace ee
{
    /* Nice interface for instructions */
    struct Instruction
    {
        union
        {
            uint32_t value;
            struct
            { /* Used when polling for the opcode */
                uint32_t : 26;
                uint32_t opcode : 6;
            };
            struct
            {
                uint32_t immediate : 16;
                uint32_t rt : 5;
                uint32_t rs : 5;
                uint32_t opcode : 6;
            } i_type;
            struct
            {
                uint32_t target : 26;
                uint32_t opcode : 6;
            } j_type;
            struct
            {
                uint32_t funct : 6;
                uint32_t sa : 5;
                uint32_t rd : 5;
                uint32_t rt : 5;
                uint32_t rs : 5;
                uint32_t opcode : 6;
            } r_type;
        };

        uint32_t pc;
        bool is_delay_slot = false;

        Instruction operator=(const Instruction& instr)
        {
            value = instr.value;
            pc = instr.pc;
            is_delay_slot = instr.is_delay_slot;

            return *this;
        }
    };

    union Register
    {
        uint128_t qword;
        uint64_t dword[2];
        uint32_t word[4];
        uint16_t hword[8];
        uint8_t byte[16];
    };

    constexpr uint8_t SPECIAL_OPCODE = 0b000000;

    enum ExceptionVector
    {
        V_TLB_REFILL = 0x0,
        V_COMMON = 0x180,
        V_INTERRUPT = 0x200
    };

    enum Exception
    {
        Interrupt = 0,
        TLBModified = 1,
        TLBLoad = 2,
        TLBStore = 3,
        AddrErrorLoad = 4,
        AddrErrorStore = 5,
        Syscall = 8,
        Break = 9,
        Reserved = 10,
        CopUnusable = 11,
        Overflow = 12,
        Trap = 13,
    };

    /* A class implemeting the MIPS R5900 CPU. */
    struct EmotionEngine
    {
        EmotionEngine(common::Emulator* parent);
        ~EmotionEngine();

        /* CPU functionality. */
        void reset();
        void tick(uint32_t cycles);
        void exception(Exception exception, bool log = true);
        void fetch_next();

        /* Memory operations */
        template <typename T>
        T read(uint32_t addr);

        template <typename T>
        void write(uint32_t addr, T data);

    public:
        /* Logging */
        bool print_pc = false;

        /* Registers. */
        Register gpr[32] = {};
        uint32_t pc;
        uint64_t hi0 = 0, hi1 = 0, lo0 = 0, lo1 = 0;
        uint32_t sa = 0;
        Instruction instr, next_instr;
        uint32_t exception_addr[2] = { 0x80000000, 0xBFC00200 };
        bool skip_branch_delay = 0, branch_taken = false;

        /* Used by the JIT for cycle counting */
        int cycles_to_execute = 0;

        /* EE memory */
        uint8_t scratchpad[16 * 1024];
        uint8_t* ram = nullptr;

        /* MCH registers (Idk what these are) */
        uint32_t MCH_RICM = 0, MCH_DRD = 0;
        uint8_t rdram_sdevid = 0;

        /* Coprocessors */
        COP0 cop0 = {};
        COP1 cop1 = {};

        /* Interrupts/Timers */
        INTC intc;
        Timers timers;

        /* EE JIT compiler */
        jit::JITCompiler* compiler;
        common::Emulator* emulator;
    };

    template <typename T>
    T EmotionEngine::read(uint32_t addr)
    {
        uint32_t paddr = addr & common::KUSEG_MASKS[addr >> 29];
        switch (paddr)
        {
        case 0 ... 0x1ffffff:
            return *(T*)&ram[paddr];
        case 0x1000f000:
        case 0x1000f010:
            return intc.read(paddr);
        case 0x1000f130: /* Idk what's this */
        case 0x1000f430: /* Read from MCH_DRD */
            return 0;
        case 0x1000f440: /* Read from MCH_RICM */
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
        case 0x70000000 ... 0x70003fff:
            return *(T*)&scratchpad[paddr & 0x3FFF];
        default:
            return emulator->read<T, common::ComponentID::EE>(paddr);
        }
    }

    template <typename T>
    void EmotionEngine::write(uint32_t addr, T data)
    {
        uint32_t paddr = addr & common::KUSEG_MASKS[addr >> 29];
        switch (paddr)
        {
        case 0 ... 0x1ffffff:
            *(T*)&ram[paddr] = data;
            break;
        case 0x1000f000:
        case 0x1000f010:
            intc.write(paddr, data);
            break;
        case 0x1000f180: /* Record any console output */
            emulator->print(data);
            break;
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
            MCH_DRD = data;
            break;
        case 0x70000000 ... 0x70003fff:
            *(T*)&scratchpad[paddr & 0x3FFF] = data;
            break;
        case 0x1000f500: /* Hold off some uknown addresses */
        case 0x1000f510:
            return;
        default:
            emulator->write<T, common::ComponentID::EE>(paddr, data);
        }
    }
};
