#include <gs/gs.h>
#include <common/emulator.h>
#include <cassert>

static const char* PRIV_REGS[] =
{
	"PMODE", "SMODE1", "SMODE2",
	"SRFSH", "SYNCH1", "SYNCH2",
	"SYNCV", "DISPFB1", "DISPLAY1",
	"DISPFB2", "DISPLAY2", "EXTBUF",
	"EXTDATA", "EXTWRITE", "BGCOLOR",
	"GS_CSR", "GS_IMR", "BUSDIR",
	"SIGLBLID"
};

namespace gs
{
	GraphicsSynthesizer::GraphicsSynthesizer(common::Emulator* parent) :
		emulator(parent)
	{
		emulator->add_handler(0x12001000, this, &GraphicsSynthesizer::read_priv, &GraphicsSynthesizer::write_priv);
	}

	uint64_t GraphicsSynthesizer::read_priv(uint32_t addr)
	{
		bool group = addr & 0xf000;
		uint32_t offset = ((addr >> 4) & 0xf) + 15 * group;
		auto ptr = (uint64_t*)&priv_regs + offset;

		fmt::print("[GS] Reading {:#x} from {}\n", *ptr, PRIV_REGS[offset]);
		/* Only CSR and SIGLBLID are readable! */
		assert(offset == 15 || offset == 18);

		return *ptr;
	}

	void GraphicsSynthesizer::write_priv(uint32_t addr, uint64_t data)
	{
		bool group = addr & 0xf000;
		uint32_t offset = ((addr >> 4) & 0xf) + 15 * group;
		auto ptr = (uint64_t*)&priv_regs + offset;

		fmt::print("[GS] Writing {:#x} to {}\n", data, PRIV_REGS[offset]);

		*ptr = data;
	}

	void GraphicsSynthesizer::write(uint16_t addr, uint64_t data)
	{
		auto ptr = (uint64_t*)&regs + addr;
		*ptr = data;

		fmt::print("[GS] Writting {:#x} to address {:#x}\n", data, addr);
	}
}