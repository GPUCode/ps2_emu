#include <cpu/vu/vif.h>
#include <common/emulator.h>
#include <cassert>

constexpr const char* REGS[] =
{
	"VIF_STAT", "VIF_FBRST",
	"VIF_ERR", "VIF_MARK",
	"VIF_CYCLE", "VIF_MODE",
	"VIF_NUM", "VIF_MASK"
};

namespace vu
{
	VIF::VIF(common::Emulator* parent) :
		emulator(parent)
	{
		/* Register VIF register set */
		emulator->add_handler(0x10003800, this, &VIF::read, &VIF::write);
		emulator->add_handler(0x10003C00, this, &VIF::read, &VIF::write);

		/* Register VIF0/VIF1 fifos */
		emulator->add_handler<uint128_t>(0x10004000, this, nullptr, &VIF::write_fifo);
		emulator->add_handler<uint128_t>(0x10005000, this, nullptr, &VIF::write_fifo);
	}
	
	uint32_t VIF::read(uint32_t addr)
	{
		return uint32_t();
	}
	
	void VIF::write(uint32_t addr, uint32_t data)
	{
		uint32_t vif = (addr & 0x4000) >> 14;
		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&registers[vif] + offset;

		fmt::print("[VIF{}] Writing {:#x} to {}\n", vif, data, REGS[offset]);
		*ptr = (offset == 0 ? data & 0x1 : data);
	}
	
	void VIF::write_fifo(uint32_t addr, uint128_t data)
	{
		assert(addr == 0x10005000 || addr == 0x10004000);
		
		/* Get target register set */
		uint32_t vif = (addr & 0x1000) >> 12;
		auto& regs = registers[vif];

		/* This is set by commands that expect data packets */
		static uint32_t command = 0, data_remaining = 0;

		auto vif_packets = (uint32_t*)&data;
		for (int i = 0; i < 4; i++)
		{
			uint32_t packet = *(vif_packets + i);
			if (!data_remaining)
			{
				/* Process VIF command */
				command = (packet >> 24) & 0x7f;
				uint16_t imm = packet & 0xffff;
				switch (command)
				{
				case 0x0:
				{
					fmt::print("[VIF{}] NOP\n", vif);
					break;
				}
				case 0x1:
				{
					regs.cycle.cycle_length = imm;
					fmt::print("[VIF{}] STCYCL: CYCLE = {:#x}\n", vif, imm);
					break;
				}
				case 0x2:
				{
					regs.ofst = imm & 0x3ff;
					regs.status.double_buffer_flag = 0;
					regs.base = regs.tops;
					fmt::print("[VIF{}] OFFSET: OFST = {:#x} BASE = TOPS = {:#x}\n", vif, regs.ofst, regs.tops);
					break;
				}
				case 0x3:
				{
					regs.base = imm & 0x3ff;
					fmt::print("[VIF{}] BASE: BASE = {:#x}\n", vif, regs.base);
					break;
				}
				case 0x4:
				{
					regs.itop = imm & 0x3ff;
					fmt::print("[VIF{}] ITOP: ITOP = {:#x}\n", vif, regs.itop);
					break;
				}
				case 0x5:
				{
					regs.mode = imm & 0x3;
					fmt::print("[VIF{}] STMOD: MODE = {:#x}\n", vif, regs.mode);
					break;
				}
				case 0x6:
				{
					/* This is a NOP for now */
					fmt::print("[VIF{}] MSKPATH3\n", vif);
					break;
				}
				case 0x7:
				{
					regs.mark = imm;
					fmt::print("[VIF{}] MARK: MARK = {:#x}\n", vif, imm);
					break;
				}
				case 0x20:
				{
					data_remaining = 1;
					break;
				}
				case 0x30:
				{
					data_remaining = 4;
					fmt::print("[VIF{}] STROW: ", vif);
					break;
				}
				case 0x31:
				{
					data_remaining = 4;
					fmt::print("[VIF{}] STCOL: ", vif);
					break;
				}
				default:
					fmt::print("[VIF{}] Uknown fifo command {:#x}\n", vif, command);
				}
			}
			else
			{
				switch (command)
				{
				case 0x20:
				{
					regs.mask = packet;
					fmt::print("[VIF{}] STMASK: MASK = {:#x}", vif, packet);
					break;
				}
				case 0x30:
				{
					regs.rn[4 - data_remaining] = packet;
					fmt::print("RN[{}] = {:#x} ", 4 - data_remaining, packet);
					break;
				}
				case 0x31:
				{
					regs.cn[4 - data_remaining] = packet;
					fmt::print("CN[{}] = {:#x} ", 4 - data_remaining, packet);
					break;
				}
				}

				data_remaining--;
				if (!data_remaining)
				{
					fmt::print("\n");
				}
			}
		}
	}
}