#pragma once
#include <cpu/iop/cop0.h>
#include <cpu/iop/timers.h>
#include <common/emulator.h>

namespace iop
{
    /* Used for storing load-delay slots */
    struct LoadInfo 
    {
        uint32_t reg = 0;
        uint32_t value = 0;
    };

    /* Nice interface for instructions */
    struct Instruction 
    {
        union 
        {
            uint32_t value;
            struct /* Used when polling for the opcode */
            {
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
        bool branch_taken = false;

        Instruction operator=(const Instruction& instr)
        {
            value = instr.value;
            pc = instr.pc;
            is_delay_slot = instr.is_delay_slot;
            branch_taken = instr.branch_taken;

            return *this;
        }
    };

    enum class Exception 
    {
        Interrupt = 0x0,
        ReadError = 0x4,
        WriteError = 0x5,
        BusError = 0x6,
        Syscall = 0x8,
        Break = 0x9,
        IllegalInstr = 0xA,
        CoprocessorError = 0xB,
        Overflow = 0xC
    };

    enum class Interrupt
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

    /* A class implemeting the PS2 IOP, a MIPS R3000A CPU. */
    class IOProcessor 
    {
    public:
        IOProcessor(common::Emulator* parent);
        ~IOProcessor();

        /* CPU functionality */
        void tick();
        void reset();
        void fetch();
        void branch();
        void handle_load_delay();
        void trigger(Interrupt intr);
        void handle_interrupts();

        /* Call this after setting the PC to skip delay slot */
        void direct_jump();

        void exception(Exception cause, uint32_t cop = 0);
        void set_reg(uint32_t regN, uint32_t value);
        void load(uint32_t regN, uint32_t value);

        /* Bus communication. */
        template <typename T>
        T read(uint32_t addr);

        template <typename T>
        void write(uint32_t addr, T data);

        /* Opcodes. */
        void op_special(); void op_cop0();

        /* Read/Write instructions. */
        void op_lhu(); void op_lh(); void op_lbu(); void op_sw();
        void op_swr(); void op_swl(); void op_lwr(); void op_lwl();
        void op_lui(); void op_sb(); void op_lw(); void op_lb();
        void op_sh();

        /* Bitwise manipulation instructions. */
        void op_and(); void op_andi(); void op_xor(); void op_xori();
        void op_nor(); void op_or(); void op_ori();

        /* Jump and branch instructions. */
        void op_bcond(); void op_jalr(); void op_blez(); void op_bgtz();
        void op_j(); void op_beq(); void op_jr(); void op_jal();
        void op_bne();

        /* Arithmetic instructions. */
        void op_add(); void op_addi(); void op_addu(); void op_addiu();
        void op_sub(); void op_subu(); void op_mult(); void op_multu();
        void op_div(); void op_divu();

        /* Bit shift-rotation instructions. */
        void op_srl(); void op_srlv(); void op_sra(); void op_srav();
        void op_sll(); void op_sllv(); void op_slt(); void op_slti();
        void op_sltu(); void op_sltiu();

        /* HI/LO instructions. */
        void op_mthi(); void op_mtlo(); void op_mfhi(); void op_mflo();

        /* Exception instructions. */
        void op_break(); void op_syscall(); void op_rfe();

        /* Coprocessor instructions. */
        void op_mfc0(); void op_mtc0();

    public:
        common::Emulator* emulator;

        uint32_t pc;
        uint32_t gpr[32];
        uint32_t hi, lo;
        uint32_t exception_addr[2] = { 0x80000080, 0xBFC00180 };

        /* IOP interrupts */
        struct
        {
            uint32_t i_stat;
            uint32_t i_mask;
            uint32_t i_ctrl;
        } intr = {};
        Timers timers;

        /* Coprocessors. */
        COP0 cop0;

        /* IOP memory */
        uint8_t* ram = nullptr;

        /* Pipeline stuff. */
        LoadInfo write_back, memory_load, delayed_memory_load;
        Instruction instr, next_instr;

        FILE* disassembly;
    };

    template<typename T>
    inline T IOProcessor::read(uint32_t addr)
    {
        uint32_t paddr = addr & common::KUSEG_MASKS[addr >> 29];
        switch (paddr)
        {
        case 0 ... 0x1fffff:
            return *(T*)&ram[paddr];
        case 0x1f801070 ... 0x1f801078:
        {
            uint32_t offset = (paddr & 0xf) >> 2; /* Will probably abstract this */
            return *((uint32_t*)&intr + offset);
        }
        case 0x1f801450:
        case 0x1f801578:
        case 0xfffe0130: /* Cache control */
            return 0;
        default:
            return emulator->read<T, common::ComponentID::IOP>(paddr);
        }
    }

    template<typename T>
    inline void IOProcessor::write(uint32_t addr, T data)
    {
        uint32_t paddr = addr & common::KUSEG_MASKS[addr >> 29];
        switch (paddr)
        {
        case 0 ... 0x1fffff:
        {
            *(T*)&ram[paddr] = data;
            break;
        }
        case 0x1f801070 ... 0x1f801078:
        {
            uint32_t offset = (paddr & 0xf) >> 2;
            auto ptr = (uint32_t*)&intr + offset;

            /* Writing to I_STAT (offset == 0) is special */
            *ptr = (offset == 0 ? *ptr & data : data);
            break;
        }
        case 0x1f801100 ... 0x1f80112c:
        case 0x1f801480 ... 0x1f8014ac:
            return timers.write(paddr, data);
        case 0x1f801578: /* This just causes too much logspam */
            return;
        default:
            emulator->write<T, common::ComponentID::IOP>(paddr, data);
        }
    }
};