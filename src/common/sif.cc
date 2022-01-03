#include <common/sif.h>
#include <common/emulator.h>

constexpr const char* REGS[] =
{
	"SIF_MSCOM", "SIF_SMCOM", "SIF_MSFLG",
	"SIF_SMFLG", "SIF_CTRL", "", "SIF_BD6"
};

constexpr const char* COMP[] =
{
	"IOP", "EE"
};

namespace common
{
	SIF::SIF(Emulator* parent) :
		emulator(parent)
	{
		emulator->add_handler(0x1000F200, this, &SIF::read, &SIF::write);
		emulator->add_handler(0x1D000000, this, &SIF::read, &SIF::write);
	}

	uint32_t SIF::read(uint32_t addr)
	{
		auto comp = (addr >> 9) & 0x1;
		uint16_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&regs + offset;

		fmt::print("[SIF][{}] Read {:#x} from {}\n", COMP[comp], *ptr, REGS[offset]);

		return *ptr;
	}

	void SIF::write(uint32_t addr, uint32_t data)
	{
		auto comp = (addr >> 9) & 0x1;
		uint16_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&regs + offset;

		fmt::print("[SIF][{}] Writing {:#x} to {}\n", COMP[comp], data, REGS[offset]);

		*ptr = data;
	}
}