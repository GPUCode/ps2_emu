#pragma once
#include <common/component.h>
#include <gs/gs_regs.h>

namespace common
{
	class Emulator;
}

namespace gs
{
	union GS_CSR
	{
		uint64_t value;
		struct 
		{
			uint64_t signal : 1;
			uint64_t finish : 1;
			uint64_t hsint : 1;
			uint64_t vsint : 1;
			uint64_t edwint : 1;
			uint64_t zero : 2;
			uint64_t : 1;
			uint64_t flush : 1;
			uint64_t reset : 1;
			uint64_t : 2;
			uint64_t nfield : 1;
			uint64_t field : 1;
			uint64_t fifo : 2;
			uint64_t rev : 8;
			uint64_t id : 8;
			uint64_t : 32;
		};
	};

	/* These can be accessed from the EE */
	struct GSPRegs
	{
		uint64_t pmode;
		uint64_t smode1;
		uint64_t smode2;
		uint64_t srfsh;
		uint64_t synch1;
		uint64_t synch2;
		uint64_t syncv;
		uint64_t dispfb1;
		uint64_t display1;
		uint64_t dispfb2;
		uint64_t display2;
		uint64_t extbuf;
		uint64_t extdata;
		uint64_t extwrite;
		uint64_t bgcolor;
		GS_CSR csr;
		uint64_t imr;
		uint64_t busdir;
		uint64_t siglblid;
	};

	struct GraphicsSynthesizer : public common::Component
	{
		friend struct GIF;
		GraphicsSynthesizer(common::Emulator* parent);
		~GraphicsSynthesizer() = default;

		/* Used by the EE */
		uint64_t read_priv(uint32_t addr);
		void write_priv(uint32_t addr, uint64_t data);

		/* Used by the GIF */
		uint64_t read(uint16_t addr);
		void write(uint16_t addr, uint64_t data);

	private:
		common::Emulator* emulator;
		GSPRegs priv_regs = {};
		GSRegs regs = {};
	};
}