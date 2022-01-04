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

		/* Writing to SIF_CTRL is special and
		   a bit mysterious as not much is known
		   about this register */
		if (offset == 4)
		{
			auto& control = regs.ctrl;
			if (comp == 0) /* Writes from IOP */
			{
				uint8_t temp = data & 0xF0;
				if (data & 0xA0)
				{
					control &= ~0xF000;
					control |= 0x2000;
				}

				if (control & temp)
					control &= ~temp;
				else
					control |= temp;
			}
			else /* Writes from the EE */
			{
				/* Bit 8 most probably functions as an 'EE ready' flag */
				if (!(data & 0x100))
					control &= ~0x100;
				else
					control |= 0x100;
			}
		}
		else
		{
			*ptr = data;
		}
	}
}