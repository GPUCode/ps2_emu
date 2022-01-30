#include <cpu/vu/vif.h>
#include <common/emulator.h>
#include <cpu/vu/vu.h>
#include <cassert>
#include <algorithm>

namespace vu
{
	constexpr const char* VIF_REGS[] =
	{
		"VIF_STAT", "VIF_FBRST",
		"VIF_ERR", "VIF_MARK",
		"VIF_CYCLE", "VIF_MODE",
		"VIF_NUM", "VIF_MASK"
	};

	constexpr const char* UNPACK_FORMATS[4][4] =
	{
		{ "S-32", "V2-32", "V3-32", "V4-32" },
		{ "S-16", "V2-16", "V3-16", "V4-16" },
		{ "S-8",  "V2-8",  "V3-8",  "V4-8" },
		{ "V4-5" }
	};

	VIF::VIF(common::Emulator* parent, int N) :
		emulator(parent), id(N)
	{
		uint32_t reg_addr = 0x10003800 | (id << 10);
		uint32_t fifo_addr = 0x10004000 | (id << 12);

		/* Register VIF register set */
		emulator->add_handler(reg_addr, this, &VIF::read, &VIF::write);
		emulator->add_handler<uint128_t>(fifo_addr, this, nullptr, &VIF::write_fifo<uint128_t>);
	}
	
	void VIF::tick(uint32_t cycles)
	{
		word_cycles = cycles * 4;
		while (!fifo.empty() && word_cycles--)
		{
			if (!subpacket_count)
				process_command();
			else
				execute_command();
		}
	}

	void VIF::reset()
	{
		/* Reset all the VIF registers */
		status = {}; fbrst = {}; cycle = {};
		err = mark = mode = num = 0;
		mask = code = itops = 0;
		base = ofst = tops = itop = top = 0;
		rn = {}; cn = {}; fifo = {}; command = {};
		subpacket_count = address = qwords_written = word_cycles = 0;
		write_mode = WriteMode::Skipping;
	}

	uint32_t VIF::read(uint32_t address)
	{
		auto offset = (address >> 4) & 0xf;
		uint32_t data = 0;
		switch (offset)
		{
		case 0:
			status.fifo_count = fifo.size<uint128_t>();
			data = status.value;
			break;
		default:
			fmt::print("[VIF{}] Writing {:#x} to unknown register {}\n", id, data, VIF_REGS[offset]);
			std::abort();
		}

		fmt::print("[VIF{}] Reading {:#x} from {}\n", id, data, VIF_REGS[offset]);
		return data;
	}

	void VIF::write(uint32_t address, uint32_t data)
	{
		auto offset = (address >> 4) & 0xf;
		switch (offset)
		{
		case 0:
			/* Only the FDR bit in status is writable */
			status.fifo_detection = data & 0x800000;
			break;
		case 1:
			fbrst.value = data;
			
			/* Reset VIF is RST bit is set */
			if (fbrst.reset)
				reset();
			
			break;
		case 2:
			err = data;
			break;
		case 3:
			mark = data;
			status.mark = 0;
			break;
		default:
			fmt::print("[VIF{}] Writing {:#x} to unknown register {}\n", id, data, VIF_REGS[offset]);
			std::abort();
		}

		fmt::print("[VIF{}] Writing {:#x} to {}\n", id, data, VIF_REGS[offset]);
	}

	void VIF::process_command()
	{
		if (fifo.read(&command.value))
		{
			auto immediate = command.immediate;
			switch (command.command)
			{
			case VIFCommands::NOP:
				fmt::print("[VIF{}] NOP\n", id);
				break;
			case VIFCommands::STCYCL:
				cycle.value = immediate;
				fmt::print("[VIF{}] STCYCL: CYCLE = {:#x}\n", id, immediate);
				break;
			case VIFCommands::OFFSET:
				ofst = immediate & 0x3ff;
				status.double_buffer_flag = 0;
				base = tops;
				fmt::print("[VIF{}] OFFSET: OFST = {:#x} BASE = TOPS = {:#x}\n", id, ofst, tops);
				break;
			case VIFCommands::BASE:
				base = immediate & 0x3ff;
				fmt::print("[VIF{}] BASE: BASE = {:#x}\n", id, base);
				break;
			case VIFCommands::ITOP:
				itop = immediate & 0x3ff;
				fmt::print("[VIF{}] ITOP: ITOP = {:#x}\n", id, itop);
				break;
			case VIFCommands::STMOD:
				mode = immediate & 0x3;
				fmt::print("[VIF{}] STMOD: MODE = {:#x}\n", id, mode);
				break;
			case VIFCommands::MSKPATH3:
				fmt::print("[VIF{}] MSKPATH3\n", id);
				/* TODO */
				break;
			case VIFCommands::MARK:
				mark = immediate;
				fmt::print("[VIF{}] MARK: MARK = {:#x}\n", id, mark);
				break;
			case VIFCommands::FLUSHE:
				fmt::print("[VIF{}] FLUSHE\n", id);
				break;
			case VIFCommands::STMASK:
				subpacket_count = 1;
				fmt::print("[VIF{}] STMASK\n", id);
				break;
			case VIFCommands::STROW:
				subpacket_count = 4;
				fmt::print("[VIF{}] STROW:\n", id);
				break;
			case VIFCommands::STCOL:
				subpacket_count = 4;
				fmt::print("[VIF{}] STCOL:\n", id);
				break;
			case VIFCommands::MPG: /* TODO: Account for VU stalls */
				/* NOTE: Since MPG tranfers instructions NUM is measured in dwords */
				subpacket_count = command.num != 0 ? command.num * 2 : 512;
				address = command.immediate * 8;
				fmt::print("[VIF{0}] MPG: Trasfering {1} words to VU{0}\n", id, subpacket_count);
				break;
			case VIFCommands::UNPACKSTART ... VIFCommands::UNPACKEND:
				process_unpack();
				break;
			default:
				fmt::print("[VIF{}] Unkown VIF command {:#x}\n", id, (uint16_t)command.command);
				std::abort();
			}

			fifo.pop<uint32_t>();
		}
	}

	void VIF::process_unpack()
	{
		/* Compute address of unpack transfer */
		address = command.address * 16;
		address += command.flg ? tops * 16 : 0;

		/* Compute the input to unpack ratio, how many input bits are unpacked
		   into a single qword. This depends on the unpack format 
		   EE User's Manual [123] */
		auto bit_size = (32 >> command.vl) * (command.vn + 1);
		auto word_count = ((bit_size + 0x1f) & ~0x1f) / 32;
		subpacket_count = word_count * command.num;
		write_mode = cycle.cycle_length < cycle.write_cycle_length ? WriteMode::Filling : WriteMode::Skipping;

		/* Clear the qword counter */
		qwords_written = 0;

		/* Add some asserts for unimplemented stuff */
		assert(!mode);
		assert(cycle.cycle_length >= cycle.write_cycle_length);
		
		fmt::print("[VIF{}] UNPACK {} of {:d} words of data\n", id, UNPACK_FORMATS[command.vl][command.vn], subpacket_count);
	}

	void VIF::execute_command()
	{
		/* Read command data from the FIFO */
		uint32_t data; 
		if (fifo.read(&data))
		{
			switch (command.command)
			{
			case VIFCommands::STMASK:
				mask = data;
				fmt::print("MASK = {:#x}\n", mask);
				break;
			case VIFCommands::STROW:
				rn[4 - subpacket_count] = data;
				fmt::print("RN[{}] = {:#x} ", 4 - subpacket_count, data);
				break;
			case VIFCommands::STCOL:
				cn[4 - subpacket_count] = data;
				fmt::print("CN[{}] = {:#x} ", 4 - subpacket_count, data);
				break;
			case VIFCommands::MPG:
				assert(id);
				emulator->vu[id]->write<vu::Memory::Code>(address, data);
				fmt::print("[VIF{0}] Transfering {1:#x} to VU{0} address {2:#x}\n", id, data, address);
				address += 4;
				break;
			case VIFCommands::UNPACKSTART ... VIFCommands::UNPACKEND:
				unpack_packet();
				return; /* Important, to skip the fifo pop below! */
			}

			subpacket_count--;
			fifo.pop<uint32_t>();
		}
	}

	void VIF::unpack_packet()
	{
		auto format = command.command & 0xf;
		uint128_t qword = 0;
		switch (format)
		{
		case VIFUFormat::S_32:
		{
			uint32_t data;
			if (fifo.read(&data))
			{
				uint32_t* words = (uint32_t*)&qword;
				words[0] = words[1] = words[2] = words[3] = data;
				
				subpacket_count--;
				fifo.pop<uint32_t>();
				break;
			}
			return;
		}
		case VIFUFormat::V4_32:
		{
			/* Fill the qword with the input data */
			if (fifo.read(&qword))
			{
				subpacket_count -= 4;
				word_cycles = word_cycles >= 4 ? word_cycles - 3 : 0;
				fifo.pop<uint128_t>();
				break;
			}

			return;
		}
		default:
			fmt::print("[VIF{}] Unknown UNPACK format {:#b}\n", id, format);
			std::abort();
		}

		/* TODO: Apply masking/mode settings */

		/* Write qword to VU mem */
		emulator->vu[id]->write<vu::Memory::Data>(address, qword);

		address += 16;
		qwords_written++;

		if (write_mode == WriteMode::Skipping)
		{
			/* Skip, skip, skip... */
			if (qwords_written >= cycle.cycle_length)
				address += (cycle.cycle_length - qwords_written) * 16;
		}
		else
		{
			/* NOP */
			std::abort();
		}
	}
}