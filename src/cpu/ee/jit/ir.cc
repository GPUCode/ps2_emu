#include <cpu/ee/jit/ir.h>
#include <cpu/ee/ee.h>

namespace ee
{
	namespace jit
	{
        IRBlock::IRBlock(uint32_t pc) :
            pc(pc)
		{
			/* Reserve some space in the code buffer
			   to avoid costly allocations */
			instructions.reserve(16);
		}

		void IRBlock::add_instruction(const IRInstruction& instr)
		{
            IRInstruction copy = instr;
            /* Remember to set the instruction PC */
            copy.pc = pc + instructions.size() * 4;
            instructions.push_back(copy);
            total_cycles += copy.instruction_cycle_count;
		}

		IRBuilder::IRBuilder(EmotionEngine* parent) :
			ee(parent)
		{
		}
		
		IRBlock IRBuilder::generate(uint32_t pc)
		{
            IRBlock block(pc);

            /* Add decoded instructions in the block until a jump */
            bool branch_delay = false;
            do
            {
                uint32_t value = ee->read<uint32_t>(pc);
                pc += 4;

                auto instr = decode(value);
                instr.cycles_till_now = block.total_cycles;
                block.add_instruction(instr);

                branch_delay = block.size() > 1 ? block[block.size() - 2].is_branch : false;
            } while (!branch_delay);

			return block;
		}

        IRInstruction IRBuilder::decode(uint32_t value)
        {
            Instruction instr{.value = value};

            IRInstruction ir_instr;
            ir_instr.instruction_cycle_count = 1;

            /* Decode global instruction fields */
            ir_instr.destination = instr.r_type.rd;
            ir_instr.source = instr.r_type.rs;
            ir_instr.target = instr.r_type.rt;
            ir_instr.shift = instr.r_type.sa;
            /* Except from the J instruction, everyone uses this */
            ir_instr.immediate = instr.i_type.immediate;
            /* Used in interpreter fallback */
            ir_instr.value = value;

            switch (instr.opcode)
            {
            case COP0_OPCODE:
            {
                uint16_t type = instr.r_type.rs;
                switch (type)
                {
                case COP0_MF0:
                    switch (instr.value & 0x7)
                    {
                    case 0b000:
                        ir_instr.operation = IROperation::MoveFromCop0;
                        ir_instr.handler = jit_callable<&EmotionEngine::op_mfc0>;
                        break;
                    default:
                        common::Emulator::terminate("[JIT] Failed to decode COP0 MF0 instruction: {:#03b}\n", instr.value & 0x7);
                    }
                    break;
                case COP0_MT0:
                    switch (instr.value & 0x7)
                    {
                    case 0b000:
                        ir_instr.operation = IROperation::MoveToCop0;
                        ir_instr.handler = jit_callable<&EmotionEngine::op_mtc0>;
                        break;
                    default:
                        common::Emulator::terminate("[JIT] Failed to decode COP0 MT0 instruction: {:#03b}\n", instr.value & 0x7);
                    }
                    break;
                case COP0_TLB:
                    switch (instr.value & 0x3f)
                    {
                    case 0b000010:
                        ir_instr.operation = IROperation::None;
                        ir_instr.handler = jit_callable<&EmotionEngine::op_tlbwi>;
                        break;
                    case 0b111001:
                        ir_instr.operation = IROperation::DisableInterrupts;
                        ir_instr.handler = jit_callable<&EmotionEngine::op_di>;
                        break;
                    case 0b011000:
                        ir_instr.operation = IROperation::ExceptionReturn;
                        ir_instr.is_branch = true;
                        ir_instr.handler = jit_callable<&EmotionEngine::op_eret>;
                        break;
                    case 0b111000:
                        ir_instr.operation = IROperation::EnableInterrupts;
                        ir_instr.handler = jit_callable<&EmotionEngine::op_ei>;
                        break;
                    default:
                        common::Emulator::terminate("[JIT] Failed to decode COP0 TLB instruction {:#08b}\n", instr.value & 0x3f);
                    }
                    break;
                default:
                    common::Emulator::terminate("[ERROR] Unimplemented COP0 instruction: {:#05b}\n", type);
                }
                break;
            }
            case SPECIAL_OPCODE:
            {
                uint16_t type = instr.r_type.funct;
                switch (type)
                {
                case 0b000000:
                    ir_instr.operation = value == 0 ? IROperation::None : IROperation::LogicalShiftLeftWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_sll>;
                    break;
                case 0b001000:
                    ir_instr.operation = IROperation::Jump;
                    ir_instr.is_branch = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_jr>;
                    break;
                case 0b001111:
                    ir_instr.operation = IROperation::None;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_sync>;
                    break;
                case 0b001001:
                    ir_instr.operation = IROperation::JumpLink;
                    ir_instr.is_branch = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_jalr>;
                    break;
                case 0b000011:
                    ir_instr.operation = IROperation::ArithmeticShiftRightWord;
                    ir_instr.immediate_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_sra>;
                    break;
                case 0b100001:
                    ir_instr.operation = IROperation::AddWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_addu>;
                    break;
                case 0b101101:
                    ir_instr.operation = IROperation::AddDword;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_daddu>;
                    break;
                case 0b100101:
                    ir_instr.operation = IROperation::OrWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_or>;
                    break;
                case 0b011000:
                    ir_instr.operation = IROperation::MulWord;
                    ir_instr.signed_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_mult>;
                    break;
                case 0b011011:
                    ir_instr.operation = IROperation::DivWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_divu>;
                    break;
                case 0b010010:
                    ir_instr.operation = IROperation::MoveFromLo;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_mflo>;
                    break;
                case 0b011010:
                    ir_instr.operation = IROperation::DivWord;
                    ir_instr.signed_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_div>;
                    break;
                case 0b010000:
                    ir_instr.operation = IROperation::MoveFromHi;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_mfhi>;
                    break;
                case 0b101011:
                    ir_instr.operation = IROperation::SetLessThanWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_sltu>;
                    break;
                case 0b100011:
                    ir_instr.operation = IROperation::SubWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_subu>;
                    break;
                case 0b001011:
                    ir_instr.operation = IROperation::Move;
                    ir_instr.condition = BranchCond::NotEqual;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_movn>;
                    break;
                case 0b101010:
                    ir_instr.operation = IROperation::SetLessThanWord;
                    ir_instr.signed_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_slt>;
                    break;
                case 0b100100:
                    ir_instr.operation = IROperation::AndWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_and>;
                    break;
                case 0b000010:
                    ir_instr.operation = IROperation::LogicalShiftRightWord;
                    ir_instr.immediate_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_srl>;
                    break;
                case 0b111100:
                    ir_instr.operation = IROperation::LogicalShiftLeftDword;
                    ir_instr.immediate_data = true;
                    ir_instr.shift += 32;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsll32>;
                    break;
                case 0b111111:
                    ir_instr.operation = IROperation::ArithmeticShiftRightDword;
                    ir_instr.immediate_data = true;
                    ir_instr.shift += 32;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsra32>;
                    break;
                case 0b111000:
                    ir_instr.operation = IROperation::LogicalShiftLeftDword;
                    ir_instr.immediate_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsll>;
                    break;
                case 0b010111:
                    ir_instr.operation = IROperation::ArithmeticShiftRightDword;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsrav>;
                    break;
                case 0b001010:
                    ir_instr.operation = IROperation::Move;
                    ir_instr.condition = BranchCond::Equal;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_movz>;
                    break;
                case 0b010100:
                    ir_instr.operation = IROperation::LogicalShiftLeftDword;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsllv>;
                    break;
                case 0b000100:
                    ir_instr.operation = IROperation::LogicalShiftLeftWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_sllv>;
                    break;
                case 0b000111:
                    ir_instr.operation = IROperation::ArithmeticShiftRightWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_srav>;
                    break;
                case 0b100111:
                    ir_instr.operation = IROperation::NorWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_nor>;
                    break;
                case 0b111010:
                    ir_instr.operation = IROperation::LogicalShiftRightDword;
                    ir_instr.immediate_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsrl>;
                    break;
                case 0b000110:
                    ir_instr.operation = IROperation::LogicalShiftRightWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_srlv>;
                    break;
                case 0b111110:
                    ir_instr.operation = IROperation::LogicalShiftRightDword;
                    ir_instr.immediate_data = true;
                    ir_instr.shift += 32;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsrl32>;
                    break;
                case 0b001100:
                    ir_instr.operation = IROperation::Syscall;
                    ir_instr.is_branch = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_syscall>;
                    break;
                case 0b101000:
                    ir_instr.operation = IROperation::MoveFromSa;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_mfsa>;
                    break;
                case 0b010001:
                    ir_instr.operation = IROperation::MoveToHi;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_mthi>;
                    break;
                case 0b010011:
                    ir_instr.operation = IROperation::MoveToLo;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_mtlo>;
                    break;
                case 0b101001:
                    ir_instr.operation = IROperation::MoveToSa;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_mtsa>;
                    break;
                case 0b101111:
                    ir_instr.operation = IROperation::SubDword;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsubu>;
                    break;
                case 0b100110:
                    ir_instr.operation = IROperation::XorWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_xor>;
                    break;
                case 0b011001:
                    ir_instr.operation = IROperation::MulWord;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_multu>;
                    break;
                case 0b111011:
                    ir_instr.operation = IROperation::ArithmeticShiftRightDword;
                    ir_instr.immediate_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_dsra>;
                    break;
                case 0b100010:
                    ir_instr.operation = IROperation::SubWord;
                    ir_instr.signed_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_sub>;
                    break;
                case 0b100000:
                    ir_instr.operation = IROperation::AddWord;
                    ir_instr.signed_data = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_add>;
                    break;
                default:
                    common::Emulator::terminate("[JIT] Failed to decode SPECIAL instruction: {:#06b}\n", type);
                }
                break;
            }
            //case COP1_OPCODE: op_cop1(); break;
            //case 0b010010: op_cop2(); break;
            case 0b101011:
                ir_instr.operation = IROperation::StoreWord;
                ir_instr.handler = jit_callable<&EmotionEngine::op_sw>;
                break;
            case 0b001010:
                ir_instr.operation = IROperation::SetLessThanWord;
                ir_instr.immediate_data = true;
                ir_instr.signed_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_slti>;
                break;
            case 0b000101:
                ir_instr.operation = IROperation::Branch;
                ir_instr.condition = BranchCond::NotEqual;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_bne>;
                break;
            case 0b001101:
                ir_instr.operation = IROperation::OrWord;
                ir_instr.immediate_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_ori>;
                break;
            case 0b001000:
                ir_instr.operation = IROperation::AddWord;
                ir_instr.immediate_data = true;
                ir_instr.signed_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_addi>;
                break;
            case 0b011110:
                ir_instr.operation = IROperation::LoadQword;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lq>;
                break;
            case 0b001111:
                ir_instr.operation = IROperation::LoadUpperImmediate;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lui>;
                break;
            case 0b001001:
                ir_instr.operation = IROperation::AddWord;
                ir_instr.immediate_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_addiu>;
                break;
            case 0b111111:
                ir_instr.operation = IROperation::StoreQword;
                ir_instr.handler = jit_callable<&EmotionEngine::op_sq>;
                break;
            case 0b000011:
                ir_instr.operation = IROperation::JumpLink;
                ir_instr.immediate_data = true;
                ir_instr.immediate = instr.j_type.target;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_jal>;
                break;
            case 0b000001:
            {
                uint16_t type = instr.r_type.rt;
                switch (type)
                {
                case 0b00001:
                    ir_instr.operation = IROperation::Branch;
                    ir_instr.condition = BranchCond::GreaterThanOrEqual;
                    ir_instr.is_branch = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_bgez>;
                    break;
                case 0b00000:
                    ir_instr.operation = IROperation::Branch;
                    ir_instr.condition = BranchCond::LessThan;
                    ir_instr.is_branch = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_bltz>;
                    break;
                case 0b00010:
                    ir_instr.operation = IROperation::BranchLikely;
                    ir_instr.condition = BranchCond::LessThan;
                    ir_instr.is_branch = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_bltzl>;
                    break;
                case 0b00011:
                    ir_instr.operation = IROperation::BranchLikely;
                    ir_instr.condition = BranchCond::GreaterThanOrEqual;
                    ir_instr.is_branch = true;
                    ir_instr.handler = jit_callable<&EmotionEngine::op_bgezl>;
                    break;
                default:
                    common::Emulator::terminate("[JIT] Failed to decode REGIMM instruction: {:#07b}\n", type);
                }
                break;
            }
            case 0b001100:
                ir_instr.operation = IROperation::AndWord;
                ir_instr.immediate_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_andi>;
                break;
            case 0b000100:
                ir_instr.operation = IROperation::Branch;
                ir_instr.condition = BranchCond::Equal;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_beq>;
                break;
            case 0b010100:
                ir_instr.operation = IROperation::BranchLikely;
                ir_instr.condition = BranchCond::Equal;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_beql>;
                break;
            case 0b001011:
                ir_instr.operation = IROperation::SetLessThanWord;
                ir_instr.immediate_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_sltiu>;
                break;
            case 0b010101:
                ir_instr.operation = IROperation::BranchLikely;
                ir_instr.condition = BranchCond::NotEqual;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_bnel>;
                break;
            case 0b100000:
                ir_instr.operation = IROperation::LoadByte;
                ir_instr.signed_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lb>;
                break;
            case 0b111001:
                ir_instr.operation = IROperation::StoreFloat;
                ir_instr.handler = jit_callable<&EmotionEngine::op_swc1>;
                break;
            case 0b100100:
                ir_instr.operation = IROperation::LoadByte;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lbu>;
                break;
            case 0b110111:
                ir_instr.operation = IROperation::LoadDword;
                ir_instr.handler = jit_callable<&EmotionEngine::op_ld>;
                break;
            case 0b000010:
                ir_instr.operation = IROperation::Jump;
                ir_instr.immediate_data = true;
                ir_instr.immediate = instr.j_type.target;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_j>;
                break;
            case 0b100011:
                ir_instr.operation = IROperation::LoadWord;
                ir_instr.signed_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lw>;
                break;
            case 0b101000:
                ir_instr.operation = IROperation::StoreByte;
                ir_instr.handler = jit_callable<&EmotionEngine::op_sb>;
                break;
            case 0b000110:
                ir_instr.operation = IROperation::Branch;
                ir_instr.condition = BranchCond::LessThanOrEqual;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_blez>;
                break;
            case 0b000111:
                ir_instr.operation = IROperation::Branch;
                ir_instr.condition = BranchCond::GreaterThan;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_bgtz>;
                break;
            case 0b100101:
                ir_instr.operation = IROperation::LoadHalfWord;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lhu>;
                break;
            case 0b101001:
                ir_instr.operation = IROperation::StoreHalfWord;
                ir_instr.handler = jit_callable<&EmotionEngine::op_sh>;
                break;
            case 0b001110:
                ir_instr.operation = IROperation::XorWord;
                ir_instr.immediate_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_xori>;
                break;
            case 0b011001:
                ir_instr.operation = IROperation::AddDword;
                ir_instr.immediate_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_daddiu>;
                break;
            case 0b011111:
                ir_instr.operation = IROperation::StoreQword;
                ir_instr.handler = jit_callable<&EmotionEngine::op_sq>;
                break;
            case 0b100001:
                ir_instr.operation = IROperation::LoadHalfWord;
                ir_instr.signed_data = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lh>;
                break;
            case 0b101111:
                ir_instr.operation = IROperation::None;
                ir_instr.handler = jit_callable<&EmotionEngine::op_cache>;
                break;
            case 0b100111:
                ir_instr.operation = IROperation::LoadWord;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lwu>;
                break;
            case 0b011010:
                ir_instr.operation = IROperation::LoadDwordLeft;
                ir_instr.handler = jit_callable<&EmotionEngine::op_ldl>;
                break;
            case 0b011011:
                ir_instr.operation = IROperation::LoadDwordRight;
                ir_instr.handler = jit_callable<&EmotionEngine::op_ldr>;
                break;
            case 0b101100:
                ir_instr.operation = IROperation::StoreDwordLeft;
                ir_instr.handler = jit_callable<&EmotionEngine::op_sdl>;
                break;
            case 0b101101:
                ir_instr.operation = IROperation::StoreDwordRight;
                ir_instr.handler = jit_callable<&EmotionEngine::op_sdr>;
                break;
            case 0b110001:
                ir_instr.operation = IROperation::LoadFloat;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lwc1>;
                break;
            case 0b010110:
                ir_instr.operation = IROperation::BranchLikely;
                ir_instr.condition = BranchCond::LessThanOrEqual;
                ir_instr.is_branch = true;
                ir_instr.handler = jit_callable<&EmotionEngine::op_blezl>;
                break;
            case 0b100010:
                ir_instr.operation = IROperation::LoadWordLeft;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lwl>;
                break;
            case 0b100110:
                ir_instr.operation = IROperation::LoadWordRight;
                ir_instr.handler = jit_callable<&EmotionEngine::op_lwr>;
                break;
            case 0b101010:
                ir_instr.operation = IROperation::StoreWordLeft;
                ir_instr.handler = jit_callable<&EmotionEngine::op_swl>;
                break;
            case 0b101110:
                ir_instr.operation = IROperation::StoreWordRight;
                ir_instr.handler = jit_callable<&EmotionEngine::op_swr>;
                break;
            default:
                common::Emulator::terminate("[JIT] Failed to decode opcode: {:#06b}\n", instr.opcode & 0x3F);
            }

            return ir_instr;
        }
    }
}
