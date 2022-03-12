#pragma once
#include <cpu/ee/jit/ir.h>
#include <asmjit/asmjit.h>
#include <robin_hood.h>

namespace ee
{
	struct EmotionEngine;

	namespace jit
	{
		struct JITCompiler;
        using BlockFunc = void(*)(JITCompiler*);

		struct JITCompiler
		{
			JITCompiler(EmotionEngine* parent);
			~JITCompiler();

			void run(uint32_t cycles);
			void reset();

            uint32_t get_current_pc() const;
            IRBlock generate_ir(uint32_t pc);

            void emit_block_dispatcher(asmjit::x86::Gp& jitcompiler);

        private:
			/* Emitter */
			asmjit::JitRuntime runtime;
			asmjit::x86::Assembler* builder;
			asmjit::StringLogger logger;

			/* Pointer to the EE struct used for interpreter fallbacks */
			EmotionEngine* ee;

			/* Transition from host -> JIT */
            BlockFunc entry;

            /* Builds IR code that the JIT can convert to native */
            IRBuilder irbuilder;

        public:
            /* Maps a PS2 address to a code block */
            robin_hood::unordered_flat_map<uint32_t, BlockFunc> block_cache;
		};
	}
}
