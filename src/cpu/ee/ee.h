#pragma once
#include <common/emulator.h>
#include <cpu/ee/cop0.h>
#include <cpu/ee/cop1.h>
#include <cpu/ee/intc.h>
#include <cpu/ee/timers.h>

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
        void direct_jump();

        /* Memory operations */
        template <typename T>
        T read(uint32_t addr);

        template <typename T>
        void write(uint32_t addr, T data);

        /* Opcodes */
        void op_cop0(); void op_mfc0(); void op_sw();
        void op_special(); void op_sll(); void op_slti();
        void op_bne(); void op_ori(); void op_addi();
        void op_lq(); void op_lui(); void op_jr(); void op_addiu();
        void op_tlbwi(); void op_mtc0(); void op_lw(); void op_mmi();
        void op_madd1(); void op_jalr(); void op_sd(); void op_jal();
        void op_sra(); void op_regimm(); void op_bgez(); void op_addu();
        void op_daddu(); void op_andi(); void op_beq(); void op_or();
        void op_mult(); void op_divu(); void op_beql(); void op_mflo();
        void op_sltiu(); void op_bnel(); void op_sync(); void op_lb();
        void op_swc1(); void op_lbu(); void op_ld(); void op_j();
        void op_sb(); void op_div(); void op_mfhi(); void op_sltu();
        void op_blez(); void op_subu(); void op_bgtz(); void op_movn();
        void op_slt(); void op_and(); void op_srl(); void op_dsll32();
        void op_dsra32(); void op_dsll(); void op_lhu(); void op_bltz();
        void op_sh(); void op_madd(); void op_divu1(); void op_mflo1();
        void op_dsrav(); void op_xori(); void op_mult1(); void op_movz();
        void op_dsllv(); void op_daddiu(); void op_sq(); void op_lh();
        void op_cache(); void op_sllv(); void op_srav(); void op_nor();
        void op_lwu(); void op_ldl(); void op_ldr(); void op_sdl();
        void op_sdr(); void op_dsrl(); void op_srlv(); void op_dsrl32();
        void op_syscall(); void op_bltzl(); void op_bgezl(); void op_mfsa();
        void op_mthi(); void op_mtlo(); void op_mtsa();

        /* COP0 instructions */
        void op_di(); void op_eret(); void op_ei();

        /* COP1 instructions */
        void op_cop1(); void op_mtc1(); void op_ctc1(); void op_cfc1();

        /* COP2 instructions */
        void op_cop2();

        /* Parallel instructions */
        void op_por(); void op_padduw();

        /* MMI instructions */
        void op_plzcw(); void op_mfhi1(); void op_mthi1(); void op_mtlo1();

        /* Registers. */
        Register gpr[32] = {};
        uint32_t pc;
        uint64_t hi0 = 0, hi1 = 0, lo0 = 0, lo1 = 0;
        uint32_t sa = 0;
        Instruction instr, next_instr;
        uint32_t exception_addr[2] = { 0x80000000, 0xBFC00200 };
        bool skip_branch_delay = false;

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

    protected:
        common::Emulator* emulator;

        /* Logging */
        std::FILE* disassembly;
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