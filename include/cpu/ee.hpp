#pragma once
#include <cstdint>
#include <cpu/cop0.hpp>

/* Nice interface for instructions */
union Instruction {
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

    Instruction& operator=(uint32_t val)
    {
        value = val;
        return *this;
    }
};

using uint128_t = unsigned __int128;
union Register {
    uint128_t quadword;
    uint64_t doubleword[2];
    uint32_t word[4];
};

constexpr uint8_t SPECIAL_OPCODE = 0b000000;

class ComponentManager;

/* A class implemeting the MIPS R5900 CPU. */
class EmotionEngine {
public:
    EmotionEngine(ComponentManager* parent);
    ~EmotionEngine() = default;

    /* CPU functionality. */
    void tick();
    void reset_state();
    void fetch_instruction();

    /* Opcodes */
    void op_cop0(); void op_mfc0(); void op_sw();
    void op_special(); void op_sll(); void op_slti();
    void op_bne(); void op_ori(); void op_addi();
    void op_lq();

protected:
    ComponentManager* manager;

    /* Registers. */
    Register gpr[32];
    uint32_t pc;
    uint64_t hi0, hi1, lo0, lo1;
    uint32_t sa;
    Instruction instr, next_instr;

    /* Coprocessors */
    COP0 cop0;
};