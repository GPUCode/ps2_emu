#pragma once
#include <cstdint>
#include <vector>

namespace ee
{
    struct EmotionEngine;

	namespace jit
	{
        enum class BranchCond
        {
            None,
            Equal = 1 << 0,
            NotEqual = 1 << 1,
            LessThan = 1 << 2,
            LessThanOrEqual = LessThan | Equal,
            GreaterThan = 1 << 3,
            GreaterThanOrEqual = GreaterThan | Equal
        };

        enum class IROperation
        {
            /* Nop */
            None,

            /* Arithemetic */
            AddWord, AddDword, SubWord, SubDword,
            MulWord, MulDword, DivWord, DivDword,

            /* Branch */
            Jump, JumpLink,
            Branch, BranchLikely,
            Syscall, ExceptionReturn,

            /* Logic */
            AndWord, NorWord, OrWord, XorWord,
            SetLessThanWord,

            /* Shift */
            LogicalShiftLeftWord, LogicalShiftRightWord,
            ArithmeticShiftRightWord,
            LogicalShiftLeftDword, LogicalShiftRightDword,
            ArithmeticShiftRightDword,

            /* Memory */
            LoadByte, LoadHalfWord, LoadWord, LoadDword, LoadQword,
            StoreByte, StoreHalfWord, StoreWord, StoreDword, StoreQword,
            LoadWordLeft, LoadWordRight, StoreWordLeft, StoreWordRight,
            LoadDwordLeft, LoadDwordRight, StoreDwordLeft, StoreDwordRight,
            LoadUpperImmediate,
            LoadFloat, StoreFloat,

            /* Move */
            Move, MoveToHi, MoveToLo, MoveToSa,
            MoveFromHi, MoveFromLo, MoveFromSa,
            MoveFromCop0, MoveToCop0,

            /* Interrupts */
            EnableInterrupts, DisableInterrupts
        };

        using InterpreterFunc = void (EmotionEngine::*)();

        template <InterpreterFunc Func>
        void jit_callable(EmotionEngine* ee)
        {
            (ee->*Func)();
        }

        /* Instruction representation consumable from CodeGenerator */
		struct IRInstruction
		{
            /* High level IR instruction */
            IROperation operation;
            bool signed_data = false;
            bool immediate_data = false;

            /* Decoded instruction */
            uint16_t destination, source, target, shift;
            uint32_t immediate;

            /* Used for branch and conditional move instructions */
            BranchCond condition = BranchCond::None;
            bool is_branch = false;

            /* Keep track of the cycles between instructions */
            uint32_t cycles_till_now, instruction_cycle_count;

            /* Interpreter fallback */
            void (*handler)(EmotionEngine*);
            uint32_t pc, value;
		};

        /* Thin wrapper around a linear stream of instructions */
        struct IRBlock
        {
            IRBlock(uint32_t pc);
            ~IRBlock() = default;

            void add_instruction(const IRInstruction& instr);
            int size() const { return instructions.size(); }
            IRInstruction& operator[](int offset) { return instructions[offset]; }

            uint32_t total_cycles = 0, pc = 0;
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
            IRInstruction decode(uint32_t value);

        private:
            EmotionEngine* ee;
        };
	}
}
