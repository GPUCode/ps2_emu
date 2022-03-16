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
        using BlockFunc = void(*)(EmotionEngine*);

		struct JITCompiler
		{
            friend void lookup_next_block(EmotionEngine* compiler);

			JITCompiler(EmotionEngine* parent);
			~JITCompiler();

            void run();
			void reset();

        private:
            BlockFunc emit_native(IRBlock& block);
            void emit_block_dispatcher();

            void emit_register_flush();
            void emit_register_restore();

            /* Native implementations of IR instructions */
            void emit_fallback(IRInstruction& instr);

        private:
			/* Emitter */
			asmjit::JitRuntime runtime;
            asmjit::CodeHolder* code;
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
