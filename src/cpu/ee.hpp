#pragma once
#include <cpu/cop0.hpp>
#include <array>

/* Nice interface for instructions */
struct Instruction {
    union {
        uint32_t value;
        struct { /* Used when polling for the opcode */
            uint32_t : 26;
            uint32_t opcode : 6;
        };
        struct {
            uint32_t immediate : 16;
            uint32_t rt : 5;
            uint32_t rs : 5;
            uint32_t opcode : 6;
        } i_type;
        struct {
            uint32_t target : 26;
            uint32_t opcode : 6;
        } j_type;
        struct {
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

        return *this;
    }
};

union Register {
    uint64_t dword[2] = {};
    uint32_t word[4];
};

constexpr uint8_t SPECIAL_OPCODE = 0b000000;
constexpr const char* const REG[] =
{
    "zero", "at", "v0", "v1",
    "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3",
    "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3",
    "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1",
    "gp", "sp", "fp", "ra"
};

enum class ExceptionVector
{
    V_TLB_REFILL = 0x0,
    V_COMMON = 0x180,
    V_INTERRUPT = 0x200
};

enum class Exception
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

class ComponentManager;

/* A class implemeting the MIPS R5900 CPU. */
class EmotionEngine {
public:
    EmotionEngine(ComponentManager* parent);
    ~EmotionEngine();

    /* CPU functionality. */
    void tick();
    void reset_state();
    void fetch_instruction();
    void exception(Exception exception);

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
    void op_dsllv(); void op_daddiu(); void op_sq();

protected:
    ComponentManager* manager;

    /* Registers. */
    Register gpr[32];
    uint32_t pc;
    uint64_t hi0, hi1, lo0, lo1;
    uint32_t sa;
    Instruction instr, next_instr;

    /* FPU (COP1) registers */
    Register fpr[32];
    uint64_t fcr0, fcr31;

    bool skip_branch_delay = false;
    bool is_delay_slot = false;

    /* Scratchpad */
    uint8_t scratchpad[16 * 1024];

    /* Coprocessors */
    COP0 cop0;

    std::FILE* disassembly;
};