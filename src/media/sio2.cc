#include <media/sio2.h>
#include <media/gamepad.h>
#include <common/emulator.h>
#include <cpu/iop/iop.h>

media::Gamepad gamepad;

namespace media
{
	SIO2::SIO2(common::Emulator* parent) :
		emulator(parent)
	{
		emulator->add_handler(0x1f808200, this, &SIO2::read, &SIO2::write);

		current_device = SIO2Peripheral::None;
	}

	uint32_t SIO2::read(uint32_t address)
	{
		switch (address & 0xff)
		{
		case 0x64: return read_fifo();
		case 0x68: return sio2_ctrl;
		case 0x6c: return 0x0d102; /* Return connected */
		case 0x70: return 0xf; /* Always equal to 0xF? */
		case 0x74: return 0; /* Unknown */
		default:
			fmt::print("[SIO2] Writing to unknown address {:#x}\n", address);
			std::abort();
		}
	}

	void SIO2::write(uint32_t address, uint32_t data)
	{
		switch (address & 0xff)
		{
		case 0x0 ... 0x3f:
		{
			int offset = (address & 0x3f) / 4;
			send3[offset] = data;
			fmt::print("[SIO2] Writing {:#x} to SEND3[{:d}]\n", data, offset);
			break;
		}
		case 0x40 ... 0x5f:
		{
			/* When bit 2 of the address is set, SEND2 is accessed */
			bool send2 = address & 0x4;
			int offset = (address & 0x1f) / 8;
			send1_2[offset + send2 * 4] = data;
			fmt::print("[SIO2] Writing {:#x} to SEND{:d}[{:d}]\n", data, send2 + 1, offset);
			break;
		}
		case 0x60:
		{
			/* Writing to SIO2_FIFOIN, sends a new command to the SIO2 */
			upload_command(data);
			break;
		}
		case 0x68:
		{
			sio2_ctrl = data;

			fmt::print("[SIO2] Set SIO2_CTRL to {:#x}\n", data);
			if (data & 0x1)
			{
				/* Touching this bit causes an interrupt? */
				emulator->iop->intr.trigger(iop::Interrupt::SIO2);
				/* HACK: Complete transfer instantly */
				sio2_ctrl &= ~0x1;
			}
			
			/* Bits 2 and 3 reset the SIO2 */
			if (data & 0xc)
			{
				/* Reset SIO2 */
				command = {};
				current_device = SIO2Peripheral::None;
			}

			break;
		}
		default:
            common::Emulator::terminate("[SIO2] Writing {:#x} to unknown address {:#x}\n", data, address);
		}
	}
	
	void SIO2::upload_command(uint8_t cmd)
	{
		fmt::print("[SIO2] FIFOIN: {:#x}\n", cmd);

		/* Fetch parameters for new command */
		bool just_started = false;
		if (!command.size)
		{
			auto params = send3[command.index];
			if (!params)
			{
				fmt::print("[SIO2] SEND3 parameter empty!\n");
				std::abort();
			}

			/* Read the following data size */
			command.size = (params >> 8) & 0x1ff;
			command.index++;

			/* If the command was just started, select the target peripheral */
			current_device = (SIO2Peripheral)cmd;
			just_started = true;
		}
		
		command.size--;

		/* NOTE: The peripheral byte also gets sent to the peripheral! */
		switch (current_device)
		{
		case SIO2Peripheral::Controller:
		{
			/* Reset the gamepad internal pointer when
			   first starting a transfer */
			if (just_started) gamepad.written = 0;
			auto reply = gamepad.write_byte(cmd);

			fmt::print("[SIO2] Gamepad returned: {:#x}\n", reply);
			sio2_fifo.push(reply);
			break;
		}
		case SIO2Peripheral::MemCard:
		{
			sio2_fifo.push(0xff);
			break;
		}
		default:
            common::Emulator::terminate("[SIO2] Unknown component {:#x}\n", current_device);
		}
	}
	
	uint8_t SIO2::read_fifo()
	{
		auto data = sio2_fifo.front();
		sio2_fifo.pop();
		return data;
	}
}
