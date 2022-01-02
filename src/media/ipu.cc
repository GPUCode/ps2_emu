#include <media/ipu.h>
#include <common/emulator.h>
#include <cassert>

constexpr const char* REGS[] =
{
	"IPU_CMD",
	"IPU_CTRL",
	"IPU_BP"
};

namespace media
{
	IPU::IPU(common::Emulator* parent) :
		emulator(parent)
	{
		emulator->add_handler<uint64_t>(0x10002000, this, &IPU::read, &IPU::write);
	}

	uint64_t IPU::read(uint32_t addr)
	{
		assert(addr <= 0x10002030);

		uint32_t offset = (addr & 0xf0) >> 4;

		/* IPU_CMD is handled differently */
		auto ptr = (uint64_t*)&regs + offset - 1;
		auto output = (!offset ? get_command_result() : *ptr);

		fmt::print("[IPU] Read {:#x} from {}\n", output, REGS[offset]);

		return output;
	}

	void IPU::write(uint32_t addr, uint64_t data)
	{
		assert(addr <= 0x10002030);

		uint32_t offset = (addr & 0xf0) >> 4;

		fmt::print("[IPU] Writing {:#x} to {}\n", data, REGS[offset]);

		/* IPU_CMD is handled differently */
		if (!offset)
		{
			IPUCommand command;
			command.value = data & 0xffffffff;
			decode_command(command);
		}
		else
		{
			auto ptr = (uint64_t*)&regs + offset - 1;
			*ptr = data;
		}
	}

	uint128_t IPU::read_fifo(uint32_t addr)
	{
		return uint128_t();
	}

	void IPU::write_fifo(uint32_t addr, uint128_t data)
	{

	}

	void IPU::decode_command(IPUCommand cmd)
	{
		uint32_t opcode = cmd.code;
		switch (opcode)
		{
		default:
			fmt::print("[IPU] Unknown IPU command {:#x}\n", opcode);
		}
	}
	
	uint64_t IPU::get_command_result()
	{
		return 0;
	}
}