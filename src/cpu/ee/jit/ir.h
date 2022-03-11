#pragma once
#include <cstdint>
#include <vector>

namespace ee
{
    struct EmotionEngine;

	namespace jit
	{
        /**
        * Thanks to DobieStation for the enum!
        * 
        * The EE has a dual-issue pipeline, meaning that under ideal circumstances, it can execute two instructions
        * per cycle. The two instructions must have no dependencies with each other, exist in separate physical
        * pipelines, and cause no stalls for the two to be executed in a single cycle.
        *
        * There are six physical pipelines: two integer pipelines, load/store, branch, COP1, and COP2.
        * MMI instructions use both integer pipelines, so an MMI instruction and any other ALU instruction
        * can never both be issued in the same cycle.
        *
        * Because the EE is in-order, assuming no stalls, one can achieve optimal performance by pairing together
        * instructions using different physical pipelines.
        */
        enum class Pipeline : uint16_t
        {
            Unknown = 0,
            Int0 = 1,
            Int1 = 2,
            IntWide = Int0 | Int1,
            IntGeneric = 4,
            LoadStore = 8,
            Branch = 0x10,
            COP0 = 0x20,
            COP1 = 0x40,
            COP2 = 0x80,
            SA = 0x100,
            ERET = 0x200,
            SYNC = 0x400,
            LZC = 0x800,
            MAC0 = 0x1000,
            MAC1 = 0x2000,
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
