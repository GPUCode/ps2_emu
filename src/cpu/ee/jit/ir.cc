#include <cpu/ee/jit/ir.h>
#include <cpu/ee/ee.h>

namespace ee
{
	namespace jit
	{
		IRBlock::IRBlock()
		{
			/* Reserve some space in the code buffer
			   to avoid costly allocations */
			instructions.reserve(16);
			total_cycles = 0;
		}

		void IRBlock::add_instruction(const IRInstruction& instr)
		{
			instructions.push_back(instr);
			total_cycles += instr.cycle_count;
		}

		IRBuilder::IRBuilder(EmotionEngine* parent) :
			ee(parent)
		{
		}
		
		IRBlock IRBuilder::generate(uint32_t pc)
		{
			IRBlock block;

			// TODO
			return block;
		}
	}
}