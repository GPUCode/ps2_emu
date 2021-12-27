#include <gs/gif.h>
#include <gs/gs.h>
#include <common/emulator.h>
#include <cassert>
#include <fmt/core.h>

static const char* REGS[10] =
{
	"GIF_CTRL",
	"GIF_MODE",
	"GIF_STAT",
	"GIF_TAG0",
	"GIF_TAG1",
	"GIF_TAG2",
	"GIF_TAG3",
	"GIF_CNT",
	"GIF_P3CNT",
	"GIF_P3TAG"
};

namespace gs
{
	GIF::GIF(common::Emulator* parent) :
		emulator(parent)
	{
		emulator->add_handler(0x10003000, this, &GIF::read, &GIF::write);
		emulator->add_handler<uint128_t>(0x10006000, this, nullptr, &GIF::write_path3);
	}
	
	uint32_t GIF::read(uint32_t addr)
	{
		assert(addr <= 0x100030A0);

		uint32_t offset = (addr & 0xf0) >> 4;
		auto ptr = (uint32_t*)&regs + offset;

		fmt::print("[GIF] Read {:#x} from {}\n", *ptr, REGS[offset]);

		return *ptr;
	}
	
	void GIF::write(uint32_t addr, uint32_t data)
	{
		assert(addr <= 0x100030A0);

		uint32_t offset = (addr & 0xf0) >> 4;
		auto ptr = (uint32_t*)&regs + offset;

		fmt::print("[GIF] Write {:#x} to {}\n", data, REGS[offset]);

		*ptr = data;
	}
	
	void GIF::write_path3(uint32_t addr, uint128_t qword)
	{
		assert(addr == 0x10006000);

		if (tag.value == GIF_TAG_NULL) /* Initiallize tag */
		{
			auto& gs_regs = emulator->gs->regs;
			tag.value = qword;
			data_count = tag.nloop;
			reg_count = tag.nreg;

			fmt::print("[GIF] Received new GS primitive!\n");

			/* Set the GS PRIM register to the PRIM field of GIFTag.
			   NOTE: This only happens when PRE == 1 */
			if (tag.pre)
				gs_regs.prim = tag.prim;

			/* The initial value of Q is 1.0f and is set
			   when the GIFTag is read */
			gs_regs.rgbaq.q = 1;
		}
		else
		{
			if (data_count == 0)
			{
				/* Nullify the current tag to get a new one */
				tag.value = GIF_TAG_NULL;
				return;
			}

			if (reg_count == 0)
			{
				data_count--;
				reg_count = tag.nreg;
			}

			uint16_t format = tag.flg;
			switch (format)
			{
			case Format::PACKED:
				process_packed(qword);
				break;
			default:
				fmt::print("[GIF] Unknown format {:d}\n", format);
			}
		}
	}
	
	void GIF::process_packed(uint128_t qword)
	{
		int curr_reg = tag.nreg - reg_count;
		uint64_t regs = tag.regs;
		uint32_t desc = (regs >> 4 * curr_reg) & 0xf;

		/* Process the qword based on the descriptor */
		switch (desc)
		{
		case REGDesc::A_D:
		{
			uint64_t data = qword;
			uint16_t addr = (qword >> 64) & 0xff;
			emulator->gs->write(addr, data);
			break;
		}
		default:
			fmt::print("[GIF] Unknown reg descritptor {:#x}!\n", desc);
		}

		reg_count--;
	}
}