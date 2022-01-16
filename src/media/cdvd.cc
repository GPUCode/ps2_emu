#include <media/cdvd.h>
#include <common/emulator.h>
#include <cassert>

namespace media
{
	CDVD::CDVD(common::Emulator* parent) :
		emulator(parent)
	{
		emulator->add_handler<uint8_t>(0x1f402000, this, &CDVD::read, &CDVD::write);

		/* Initialize S status */
		S.status = 0x40;
	}
	
	uint8_t CDVD::read(uint32_t address)
	{
		assert(address <= 0x1f402018);
		switch (address & 0xff)
		{
		case 0x16:
		{
			return S.command;
		}
		case 0x17:
		{
			return S.status;
		}
		case 0x18:
		{
			auto result = read_S_result();
			fmt::print("[DVD] Reading command result {:#x}\n", result);
			return result;
		}
		default:
			fmt::print("[DVD] Unknown read from address {:#x}\n", address);
			std::abort();
		}
	}
	
	void CDVD::write(uint32_t address, uint8_t data)
	{
		assert(address <= 0x1f402018);
		switch (address & 0xff)
		{
		case 0x16:
		{
			process_S_command(data);
			break;
		}
		case 0x17:
		{
			fmt::print("[DVD] Writing S parameter {:#x}\n", data);
			S.params.push(data);
			break;
		}
		default:
			fmt::print("[DVD] Unknown write to address {:#x}\n", address);
			std::abort();
		}
	}

	uint8_t CDVD::read_S_result()
	{
		if (S.result.empty())
			return 0;

		auto output = S.result.front();
		S.result.pop();

		/* Update status if all data has been read */
		if (S.result.empty())
			S.status |= 0x40;

		return output;
	}
	
	void CDVD::process_S_command(uint8_t cmd)
	{
		/* Data is being processed */
		S.status &= ~0x40;
		S.command = cmd;

		switch (cmd)
		{
		case 0x15:
		{
			fmt::print("[DVD] ForbidDVD\n");
			S.result.push(5);
			break;
		}
		case 0x40:
		{
			fmt::print("[DVD] OpenConfig\n");
			S.result.push(0);
			break;
		}
		case 0x41:
		{
			fmt::print("[DVD] ReadConfig\n");
			for (int i = 0; i < 16; i++)
				S.result.push(0);
			break;
		}
		case 0x43:
		{
			fmt::print("[DVD] CloseConfig\n");
			S.result.push(0);
			break;
		}
		default:
			fmt::print("[DVD] Unknown S command {:#x}\n", S.command);
			std::abort();
		}

		/* Clear parameters, since we used them */
		S.params = {};
	}
}