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
																		
            /* Store the calle-saved registers */
            //emit_register_flush();

			/* Emit block dispatcher */
            //emit_block_dispatcher();

            /* Restore saved registers before exiting */
            //emit_register_restore();

            //if (auto error = runtime.add(&entry, code); error)
            //{
            //	common::Emulator::terminate("[JIT] Could not compile entry function!\n");
            //}

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

        void test(EmotionEngine* ee)
        {
            if (ee->pc == 0x9fc42564)
            {
                fmt::print("fff");
                fmt::print("fsd");
                fmt::print("fsds");
            }
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

            /* Push new stack frame for our block */
            builder->push(asmjit::x86::rbp);
            builder->mov(asmjit::x86::rsp, asmjit::x86::rbp);
            builder->sub(asmjit::x86::rsp, 0x1B8);
            //emit_register_flush();
            builder->push(asmjit::x86::rbx);

            /* Set EE PC to the start of the block */
            builder->mov(asmjit::x86::rbx, asmjit::x86::rdi);
            auto pc_ptr = asmjit::x86::dword_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, pc));
            builder->mov(pc_ptr, block.pc);

            for (int i = 0; i < block.size(); i++)
            {
                auto& instr = block[i];

                /* If the instruction is a likely branch, the delay slot is skipped
                 * in some cases. This is bad for us, because direct_jump in the interpreter
                 * messes with the pc causing the detection logic below to fail. To deal
                 * with this set the pc to the address of the delay slot so it gets
                 * incremented to the correct place
                 */
                if (block[i].is_likely_branch)
                {
                    builder->mov(pc_ptr, instr.pc + 4);
                }

                //builder->call((uint64_t)test);
                //builder->mov(asmjit::x86::rdi, asmjit::x86::rbx);

                switch (instr.operation)
                {
                case IROperation::None:
                    break;
                default:
                    emit_fallback(instr);
                }

                /* When a branch likely instructions fails the delay slot is skipped! */
                if (block[i].is_likely_branch)
                {
                    auto skip_ptr = asmjit::x86::dword_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, skip_branch_delay));
                    builder->cmp(skip_ptr, 1);
                    builder->mov(skip_ptr, 0);
                    builder->je(block_epilogue);
                }
            }

            builder->bind(block_epilogue);

            /* Decrement cycles counter in the EE */
            auto cycles_ptr = asmjit::x86::dword_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, cycles_to_execute));
            builder->sub(cycles_ptr, block.total_cycles);

            /* Move the PC forward by the block size, ONLY if a jump didn't occur */
            builder->cmp(pc_ptr, block.pc);
            builder->jne(block_end);
            builder->add(pc_ptr, block.instructions.size() * 4);
            builder->bind(block_end);

            builder->mov(asmjit::x86::rdi, asmjit::x86::rbx);
            builder->call(reinterpret_cast<uint64_t>(test));

            /* Clean up stack before exiting */
            builder->pop(asmjit::x86::rbx);
            //emit_register_restore();
            builder->add(asmjit::x86::rsp, 0x1B8);
            builder->pop(asmjit::x86::rbp);
            builder->ret();

            /* Build! */
            if (auto error = runtime.add(&handler, code); error)
            {
                common::Emulator::terminate("[JIT] Could not compile block at PC: {:#x}\n", block.pc);
            }

            fmt::print("{}\n", logger.data());

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
             //builder->mov(pc_ptr, instr.pc);
             builder->mov(instr_value_ptr, instr.value);

             builder->call(reinterpret_cast<uint64_t>(instr.handler));
             builder->mov(asmjit::x86::rdi, asmjit::x86::rbx);

             /* Sometimes an instruction might write to GPR[0].
                To avoid branches, reset the register each time */
             //builder->mov(asmjit::x86::qqword_ptr(asmjit::x86::rdi, offsetof(EmotionEngine, gpr[0])), 0);
        }

        /* Look up the block cache for the block */
        void lookup_next_block(EmotionEngine* ee)
		{
            uint32_t pc = ee->pc;

            if (pc == 2680431928)
            {
                fmt::print("ff");
            }

            JITCompiler* compiler = ee->compiler;
            fmt::print("[JIT] Searching for block at PC: {:#x}\n", pc);

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

            block(ee);
        }

        void JITCompiler::run()
        {
            //entry(ee);
            while (ee->cycles_to_execute > 0)
                lookup_next_block(ee);
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

            builder->push(asmjit::x86::rbx);
            builder->mov(asmjit::x86::rbx, asmjit::x86::rdi);
            builder->bind(loop_start);
            /* Call lookup_next_block */
            builder->mov(asmjit::x86::rdi, asmjit::x86::rbx);
            builder->call(reinterpret_cast<uint64_t>(lookup_next_block));

            builder->cmp(asmjit::x86::rax, 1);
            builder->je(loop_start);
            builder->pop(asmjit::x86::rbx);
            builder->ret();
        }
	}
}
