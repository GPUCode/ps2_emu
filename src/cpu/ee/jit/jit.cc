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
		}

		void JITCompiler::run(uint32_t cycles)
		{
            entry(this);
		}

		void JITCompiler::reset()
		{
			asmjit::CodeHolder code;
			
			code.init(runtime.environment());
			code.setLogger(&logger);
			builder = new asmjit::x86::Assembler(&code);
																		
			/* Initialize our prologue function */
			asmjit::FuncDetail func;
			auto entry_signature = asmjit::FuncSignatureT<void, JITCompiler*>(asmjit::CallConvId::kHost);
			func.init(entry_signature, runtime.environment());
			
			/* Configure the stack size and alignment */
			asmjit::FuncFrame frame;
			frame.setAvxEnabled();
			frame.init(func);
			frame.setLocalStackSize(0x20);
			frame.setLocalStackAlignment(8);
			
			/* Create arguments assignment context */
			asmjit::FuncArgsAssignment args(&func);
			auto arg1 = builder->zcx();
			args.assignAll(arg1);
			args.updateFuncFrame(frame);
            frame.finalize();

			/* Insert function prolog and allocate arguments to registers. */
			builder->emitProlog(frame);
			builder->emitArgsAssignment(frame, args);

			/* Emit block dispatcher */
            emit_block_dispatcher(arg1);

			/* Insert function epilog. */
			builder->emitEpilog(frame);
			if (auto error = runtime.add(&entry, &code); error)
			{
				fmt::print("[JIT] Could not compile entry function!\n");
				std::abort();
			}

            fmt::print("{}\n", logger.data());
            entry(this);
		}

        uint32_t JITCompiler::get_current_pc() const
        {
            return ee->pc;
        }

        IRBlock JITCompiler::generate_ir(uint32_t pc)
        {
            return irbuilder.generate(pc);
        }

		/* This function is responsible for emitting a dispatcher
		   that attempts to find and directly execute the next 
		   block in the instruction stream. If not  */
        void lookup_next_block(JITCompiler* compiler)
		{
            uint32_t pc = compiler->get_current_pc();
            fmt::print("[JIT] Searching for block at PC: {:#x}\n", pc);

            /* Lookup the block in the block cache */
            if (auto block = compiler->block_cache.find(pc); block == compiler->block_cache.end())
            {
                /* Block not found, recompile it */
                IRBlock ir_block = compiler->generate_ir(pc);
                /* TODO: Generate machine code */
            }
            else
            {
                auto function = block->second;
                function(compiler);
            }
		}

        void JITCompiler::emit_block_dispatcher(asmjit::x86::Gp& jitcompiler)
        {
            asmjit::Label exit_dispatcher = builder->newLabel();

            /* If cycles_to_execute > 0 then call lookup_next_block().
               The following is the assembly GCC generates at -O1 */
            auto cycles_ptr = asmjit::x86::ptr(jitcompiler, offsetof(JITCompiler, ee) +
                                               offsetof(EmotionEngine, cycles_to_execute));
            builder->cmp(cycles_ptr, asmjit::imm(0));
            builder->je(exit_dispatcher);
            builder->call(reinterpret_cast<uint64_t>(lookup_next_block));
            builder->bind(exit_dispatcher);
        }
	}
}
