#pragma once
#include <asmjit/asmjit.h>
#include <robin_hood.h>

namespace ee
{
	struct EmotionEngine;

	namespace jit
	{
		struct JITCompiler;
		using PrologueFunc = void(*)(JITCompiler*);

		struct JITCompiler
		{
			JITCompiler(EmotionEngine* parent);
			~JITCompiler();

			void run(uint32_t cycles);
			void reset();

			void lookup_next_block();

		private:
			/* Emitter */
			asmjit::JitRuntime runtime;
			asmjit::x86::Assembler* builder;
			asmjit::StringLogger logger;

			/* Pointer to the EE struct used for interpreter fallbacks */
			EmotionEngine* ee;

			/* Transition from host -> JIT */
			PrologueFunc entry;
		};
	}
}
