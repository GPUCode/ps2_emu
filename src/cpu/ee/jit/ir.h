#pragma once
#include <cstdint>
#include <vector>

namespace ee
{
    struct EmotionEngine;

	namespace jit
	{
        /* Instruction opcodes */
         enum Opcodes
         {
             COP2 = 0b010010,
             SW = 0b101011,
             SLTI = 0b001010,
             BNE = 0b000101,
             ORI = 0b001101,
             ADDI = 0b001000,
             LQ = 0b011110,
             LUI = 0b001111,
             ADDIU = 0b001001,
             MMI = 0b011100,
             SD = 0b111111,
             JAL = 0b000011,
             REGIMM = 0b000001,
             ANDI = 0b001100,
             BEQ = 0b000100,
             BEQL = 0b010100,
             SLTIU = 0b001011,
             BNEL = 0b010101,
             LB = 0b100000,
             SWC1 = 0b111001,
             LBU = 0b100100,
             LD = 0b110111,
             J = 0b000010,
             LW = 0b100011,
             SB = 0b101000,
             BLEZ = 0b000110,
             BGTZ = 0b000111,
             LHU = 0b100101,
             SH = 0b101001,
             XORI = 0b001110,
             DADDIU = 0b011001,
             SQ = 0b011111,
             LH = 0b100001,
             CACHE = 0b101111,
             LWU = 0b100111,
             LDL = 0b011010,
             LDR = 0b011011,
             SDL = 0b101100,
             SDR = 0b101101,
             LWC1 = 0b110001,
             BLEZL = 0b010110,
             LWL = 0b100010,
             LWR = 0b100110,
             SWL = 0b101010,
             SWR = 0b101110,
             SQC2 = 0b111110
        };

        /* Instruction representation consumable from CodeGenerator */
		struct IRInstruction
		{
            /* Used in jump instructions */
            uint32_t jump_destination;
            bool skip_branch_delay;
            uint32_t return_address;
            bool is_likely, is_link;

            /* Operands for arithemtic operations (rd, rt, rs) */
            uint32_t destination, target, source;

            /* Keep track of the cycles between instructions */
            uint32_t cycles_till_now, cycle_count;

            /* Interpreter fallback */
            uint32_t opcode;
		};

        /* Thin wrapper around a linear stream of instructions */
        struct IRBlock
        {
            IRBlock();
            ~IRBlock() = default;

            void add_instruction(const IRInstruction& instr);

            uint32_t total_cycles;
            std::vector<IRInstruction> instructions;
        };

        /* Analyzes the instruction stream in the given PC and
           generates IR code */
        struct IRBuilder
        {
            IRBuilder(EmotionEngine* parent);
            ~IRBuilder() = default;

            /* Will analyze and generate an IRBlock starting
               at the given PC */
            IRBlock generate(uint32_t pc);

        private:
            EmotionEngine* ee;
        };
	}
}
