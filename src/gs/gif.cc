#include <gs/gif.h>
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
		uint32_t PAGE1 = common::Emulator::calculate_page(0x10003000);
		emulator->add_handler(PAGE1, this, &GIF::read, &GIF::write);
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
}