#include <cpu/ee/jit/jit.h>
#include <cpu/ee/ee.h>

void test_func(ee::jit::JITCompiler* jit)
{
	fmt::print("[JIT] Called C function from JIT\n");
}

namespace ee
{
	namespace jit
	{
		JITCompiler::JITCompiler(EmotionEngine* parent) :
			ee(parent)
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
			// TODO

			/* Insert function epilog. */
			builder->emitEpilog(frame);
			if (auto error = runtime.add(&entry, &code); error)
			{
				fmt::print("[JIT] Could not compile entry function!\n");
				std::abort();
			}
		}

		/* This function is responsible for emitting a dispatcher
		   that attempts to find and directly execute the next 
		   block in the instruction stream. If not  */
		void JITCompiler::lookup_next_block()
		{
			fmt::print("[JIT] Called this from the JIT!\n");
		}
	}
}