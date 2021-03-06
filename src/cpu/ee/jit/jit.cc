#include <cpu/ee/jit/jit.h>
#include <cpu/ee/ee.h>
#include <fmt/core.h>

namespace ee
{
	namespace jit
	{
		JITCompiler::JITCompiler(EmotionEngine* parent) :
            ee(parent), irbuilder(parent)
		{
		}
		
		JITCompiler::~JITCompiler()
		{
            delete code;
            delete builder;
		}

		void JITCompiler::reset()
		{
            code = new asmjit::CodeHolder;
            code->init(runtime.environment());
            code->setLogger(&logger);
            builder = new asmjit::x86::Assembler(code);
																		
			/* Emit block dispatcher */
            emit_block_dispatcher();

            if (auto error = runtime.add(&entry, code); error)
            {
                common::Emulator::terminate("[JIT] Could not compile entry function!\n");
            }

            fmt::print("{}\n", logger.data());
		}

        void JITCompiler::emit_register_flush()
        {
            static asmjit::x86::Gp preserved[] = { asmjit::x86::rbx, asmjit::x86::r12, asmjit::x86::r13,
                                                   asmjit::x86::r14, asmjit::x86::r15 };
            for (auto& reg : preserved)
                builder->push(reg);
        }

        void JITCompiler::emit_register_restore()
        {
            static asmjit::x86::Gp preserved[] = { asmjit::x86::r15, asmjit::x86::r14, asmjit::x86::r13,
                                                   asmjit::x86::r12, asmjit::x86::rbx };
            for (auto& reg : preserved)
                builder->pop(reg);
        }

        void __attribute__((noinline)) test()
        {
            return;
        }

        BlockFunc JITCompiler::emit_native(IRBlock& block)
        {
            BlockFunc handler = nullptr;
            logger.clear();

            /* Init the asmjit code buffer */
            code->reset();
            code->init(runtime.environment());
            code->setLogger(&logger);
            code->attach(builder);

            auto block_end = builder->newLabel();
            auto block_epilogue = builder->newLabel();
            auto dec_pc = builder->newLabel();

            /* Push new stack frame for our block */
            builder->push(asmjit::x86::rbp);
            builder->mov(asmjit::x86::rbp, asmjit::x86::rsp);
            //builder->sub(asmjit::x86::rsp, 0x1B8);
            //emit_register_flush();

            /* Set EE PC to the start of the block */
            builder->mov(asmjit::x86::rbx, asmjit::x86::rdi);
            auto pc_ptr = asmjit::x86::dword_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, pc));
            builder->mov(pc_ptr, block.pc);

            for (int i = 0; i < block.size(); i++)
            {
                auto& instr = block[i];
                switch (instr.operation)
                {
                case IROperation::None:
                    break;
                default:
                    emit_fallback(instr);
                }

                /* Normally in the interpreter the pc is always 2 instructions ahead of the
                 * one currently being executed, assuming no branches. This causes problems
                 * in the JIT if the instruction uses fetch_next() to direct jump, as that
                 * increments the pc, causing the JIT to skip the first instruction of the branch.
                 */
                if (block[i].operation == IROperation::ExceptionReturn ||
                    block[i].operation == IROperation::Syscall)
                {
                    builder->sub(pc_ptr, 4);
                }

                /* When a branch likely instructions fails the delay slot is skipped! */
                if (block[i].operation == IROperation::BranchLikely)
                {
                    auto skip_ptr = asmjit::x86::byte_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, skip_branch_delay));
                    builder->cmp(skip_ptr, 1);
                    builder->mov(skip_ptr, 0);
                    builder->je(dec_pc);
                }
            }

            builder->bind(block_epilogue);

            /* Decrement cycles counter in the EE */
            auto cycles_ptr = asmjit::x86::dword_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, cycles_to_execute));
            builder->sub(cycles_ptr, block.total_cycles);

            /* Move the PC forward by the block size, ONLY if a jump didn't occur */
            auto taken_ptr = asmjit::x86::byte_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, branch_taken));
            builder->cmp(taken_ptr, 1);
            builder->mov(taken_ptr, 0);
            builder->je(block_end);
            builder->mov(pc_ptr, block.pc + block.instructions.size() * 4);
            builder->bind(block_end);

            /* Clean up stack before exiting */
            //emit_register_restore();
            //builder->add(asmjit::x86::rsp, 0x1B8);
            builder->pop(asmjit::x86::rbp);
            builder->ret();

            /* Decrement pc to account for the call to fetch_next and
             * jump directly to the epilogue */
            builder->bind(dec_pc);
            builder->sub(pc_ptr, 4);
            builder->jmp(block_epilogue);

            /* Build! */
            if (auto error = runtime.add(&handler, code); error)
            {
                common::Emulator::terminate("[JIT] Could not compile block at PC: {:#x}\n", block.pc);
            }

            //fmt::print("{}\n", logger.data());

            return handler;
        }

        void JITCompiler::emit_fallback(IRInstruction& instr)
        {
            /* The interpreter functions were written to not depend too much on internal
             * state, so we only need to update the instr to make sure branches read correct
             * values
             */
             auto instr_pc_ptr = asmjit::x86::dword_ptr(asmjit::x86::rdi,
                                            offsetof(EmotionEngine, instr) +
                                            offsetof(Instruction, pc));
             auto instr_value_ptr = asmjit::x86::dword_ptr(asmjit::x86::rdi,
                                            offsetof(EmotionEngine, instr) +
                                            offsetof(Instruction, value));

             builder->mov(instr_pc_ptr, instr.pc);
             builder->mov(instr_value_ptr, instr.value);

             builder->call(reinterpret_cast<uint64_t>(instr.handler));
             builder->mov(asmjit::x86::rdi, asmjit::x86::rbx);

             /* Sometimes an instruction might write to GPR[0].
                To avoid branches, reset the register each time */
             //builder->mov(asmjit::x86::qqword_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, gpr[0])), 0);
        }

        /* Look up the block cache for the block */
        BlockFunc lookup_next_block(EmotionEngine* ee)
		{
            uint32_t pc = ee->pc;

            JITCompiler* compiler = ee->compiler;
            //fmt::print("[JIT] Searching for block at PC: {:#x}\n", pc);

            BlockFunc block = nullptr;
            if (auto result = compiler->block_cache.find(pc); result == compiler->block_cache.end())
            {
                /* Block not found, recompile it */
                IRBlock ir_block = compiler->irbuilder.generate(pc);

                block = compiler->emit_native(ir_block);
                compiler->block_cache[pc] = block;
            }
            else
            {
                block = result->second;
            }

            return block;
        }

        void JITCompiler::run()
        {
            entry(ee);
        }

        /* This function is responsible for emitting a dispatcher
           that attempts to find and directly execute the next
           block in the instruction stream. */
        void JITCompiler::emit_block_dispatcher()
        {
            asmjit::Label loop_start = builder->newLabel();

            /*  do
                {
                    BlockFunc block = lookup_next_block(ee);
                    block(ee);
                } while (ee->cycles_to_execute > 0);
            */

            auto cycles_ptr = asmjit::x86::dword_ptr(asmjit::x86::rbx, offsetof(EmotionEngine, cycles_to_execute));

            builder->push(asmjit::x86::rbx);
            builder->mov(asmjit::x86::rbx, asmjit::x86::rdi);
            builder->bind(loop_start);

            /* Call lookup_next_block */
            builder->mov(asmjit::x86::rdi, asmjit::x86::rbx);
            builder->call(reinterpret_cast<uint64_t>(lookup_next_block));

            builder->mov(asmjit::x86::rdi, asmjit::x86::rbx);
            builder->call(asmjit::x86::rax);

            builder->cmp(cycles_ptr, 0);
            builder->jg(loop_start);
            builder->pop(asmjit::x86::rbx);
            builder->ret();
        }
	}
}
