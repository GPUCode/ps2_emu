#include <cpu/ee/dmac.h>
#include <common/emulator.h>
#include <cpu/ee/ee.h>
#include <cpu/iop/iop.h>
#include <common/sif.h>
#include <gs/gif.h>
#include <cpu/vu/vif.h>
#include <cassert>

inline uint32_t get_channel(uint32_t value)
{
	switch (value)
	{
	case 0x80: return 0;
	case 0x90: return 1;
	case 0xa0: return 2;
	case 0xb0: return 3;
	case 0xb4: return 4;
	case 0xc0: return 5;
	case 0xc4: return 6;
	case 0xc8: return 7;
	case 0xd0: return 8;
	case 0xd4: return 9;
	default:
		fmt::print("[DMAC] Invalid channel id provided {:#x}, aborting...\n", value);
		std::abort();
	}
}

constexpr const char* REGS[] =
{
	"Dn_CHCR",
	"Dn_MADR",
	"Dn_QWC",
	"Dn_TADR",
	"Dn_ASR0",
	"Dn_ASR1",
	"", "",
	"Dn_SADR"
};

constexpr const char* GLOBALS[] =
{
	"D_CTRL",
	"D_STAT",
	"D_PCR",
	"D_SQWC",
	"D_RBSR",
	"D_RBOR",
	"D_STADT",
};

namespace ee
{
	DMAController::DMAController(common::Emulator* parent) :
		emulator(parent)
	{
		constexpr uint32_t addresses[] =
		{ 0x10008000, 0x10009000, 0x1000a000, 0x1000b000, 0x1000b400, 
		  0x1000c000, 0x1000c400, 0x1000c800, 0x1000d000, 0x1000d400 };

		/* Register channel handlers */
		for (auto addr : addresses)
		{
			emulator->add_handler(addr, this, &DMAController::read_channel, &DMAController::write_channel);
			/* Sadly the last register eats up a whole page itself */
			emulator->add_handler(addr + 0x80, this, &DMAController::read_channel, &DMAController::write_channel);
		}

		/* Register global register handlers */
		emulator->add_handler(0x1000E000, this, &DMAController::read_global, &DMAController::write_global);
		emulator->add_handler(0x1000F520, this, &DMAController::read_enabler, nullptr);
		emulator->add_handler(0x1000F590, this, nullptr, &DMAController::write_enabler);
	}

	uint32_t DMAController::read_channel(uint32_t addr)
	{
		assert((addr & 0xff) <= 0x80);

		uint32_t id = (addr >> 8) & 0xff;
		uint32_t channel = get_channel(id);
		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&channels[channel] + offset;

		fmt::print("[DMAC] Reading {:#x} from {} of channel {:d}\n", *ptr, REGS[offset], channel);
		return *ptr;
	}

	void DMAController::write_channel(uint32_t addr, uint32_t data)
	{
		assert((addr & 0xff) <= 0x80);

		uint32_t id = (addr >> 8) & 0xff;
		uint32_t channel = get_channel(id);
		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&channels[channel] + offset;

		fmt::print("[DMAC] Writing {:#x} to {} of channel {:d}\n", data, REGS[offset], channel);
		/* The lower bits of MADR must be zero: */
		/* NOTE: This is actually required since the BIOS writes
		   unaligned addresses to the GIF channel for some reason
		   and expects it to be read correctly... */
		data = (offset == 1 ? data &= 0x01fffff0 : data);
		*ptr = data;

		if (channels[channel].control.running)
		{
			fmt::print("\n[DMAC] Transfer for channel {:d} started!\n\n", channel);
		}
	}

	uint32_t DMAController::read_global(uint32_t addr)
	{
		assert(addr <= 0x1000E060);

		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&globals + offset;

		fmt::print("[DMAC] Reading {:#x} from {}\n", *ptr, GLOBALS[offset]);
		return *ptr;
	}

	uint32_t DMAController::read_enabler(uint32_t addr)
	{
		assert(addr == 0x1000F520);

		fmt::print("[DMAC] Reading D_ENABLER = {:#x}\n", globals.d_enable);
		return globals.d_enable;
	}

	void DMAController::write_global(uint32_t addr, uint32_t data)
	{
		assert(addr <= 0x1000E060);

		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&globals + offset;

		fmt::print("[DMAC] Writing {:#x} to {}\n", data, GLOBALS[offset]);

		if (offset == 1) /* D_STAT */
		{
			auto& cop0 = emulator->ee->cop0;

			/* The lower bits are cleared while the upper ones are reversed */
			globals.d_stat.clear &= ~(data & 0xffff);
			globals.d_stat.reverse ^= (data >> 16);

			/* IMPORTANT: Update COP0 INT1 status when D_STAT is written */
			cop0.cause.ip1_pending = globals.d_stat.channel_irq & globals.d_stat.channel_irq_mask;
		}
		else
		{
			*ptr = data;
		}
	}

	void DMAController::write_enabler(uint32_t addr, uint32_t data)
	{
		assert(addr == 0x1000F590);

		fmt::print("[DMAC] Writing D_ENABLEW = {:#x}\n", data);
		globals.d_enable = data;
	}

	void DMAController::tick(uint32_t cycles)
	{
		if (globals.d_enable & 0x10000)
			return;

		for (int cycle = cycles; cycle > 0; cycle--)
		{
			/* Check each channel */
			for (uint32_t id = 0; id < 10; id++)
			{
				auto& channel = channels[id];
				if (channel.control.running)
				{
					/* Transfer any pending qwords */
					if (channel.qword_count > 0)
					{
						/* This is channel specific */
						switch (id)
						{
						case DMAChannels::VIF1:
						{
							auto& vif1 = emulator->vif[1];
							uint128_t qword = *(uint128_t*)&emulator->ee->ram[channel.address];
							if (vif1->write_fifo(NULL, qword))
							{
								uint64_t upper = qword >> 64, lower = qword;
								fmt::print("[DMAC][VIF1] Writing {:#x}{:016x} to VIF1\n", upper, lower);

								channel.address += 16;
								channel.qword_count--;
							}
							break;
						}
						case DMAChannels::GIF:
						{
							auto& gif = emulator->gif;
							uint128_t qword = *(uint128_t*)&emulator->ee->ram[channel.address];
							uint64_t upper = qword >> 64, lower = qword;
							fmt::print("[DMAC][GIF] Writing {:#x}{:016x} to PATH3\n", upper, lower);

							gif->write_path3(0x10006000, qword);
							channel.address += 16;
							channel.qword_count--;

							assert(!channel.control.mode);
							if (!channel.qword_count)
								channel.end_transfer = true;

							break;
						}
						case DMAChannels::SIF0:
						{
							/* SIF0 receives data from the SIF0 fifo */
							auto& sif = emulator->sif;
							if (sif->sif0_fifo.size() >= 4)
							{
								uint32_t data[4];
								for (int i = 0; i < 4; i++)
								{
									data[i] = sif->sif0_fifo.front();
									sif->sif0_fifo.pop();
								}

								uint128_t qword = *(uint128_t*)data;
								uint64_t upper = qword >> 64, lower = qword;
								fmt::print("[DMAC][SIF0] Receiving packet from SIF0 FIFO: {:#x}{:016x}\n", upper, lower);

								/* Write the packet to the specified address */
								emulator->ee->write(channel.address, qword);

								/* MADR/TADR update while a transfer is ongoing */
								channel.qword_count--;
								channel.address += 16;
							}

							break;
						}
						case DMAChannels::SIF1:
						{
							/* SIF1 pushes data to the SIF1 fifo */
							auto& sif = emulator->sif;
							
							uint128_t qword = *(uint128_t*)&emulator->ee->ram[channel.address];
							uint32_t* data = (uint32_t*)&qword;
							for (int i = 0; i < 4; i++)
							{
								sif->sif1_fifo.push(data[i]);
							}

							uint64_t upper = qword >> 64, lower = qword;
							fmt::print("[DMAC][SIF1] Transfering to SIF1 FIFO: {:#x}{:016x}\n", upper, lower);

							/* MADR/TADR update while a transfer is ongoing */
							channel.qword_count--;
							channel.address += 16;
							break;
						}
						default:
							fmt::print("[DMAC] Unknown channel transfer with id {:d}\n", id);
							std::abort();
						}
					} /* If the transfer ended, disable channel */
					else if (channel.end_transfer)
					{
						fmt::print("[DMAC] End transfer of channel {:d}\n", id);

						/* End the transfer */
						channel.end_transfer = false;
						channel.control.running = 0;

						/* Set the channel bit in the interrupt field of D_STAT */
						globals.d_stat.channel_irq |= (1 << id);

						/* Check for interrupts */
						if (globals.d_stat.channel_irq & globals.d_stat.channel_irq_mask)
						{
							fmt::print("\n[DMAC] INT1!\n\n");
							emulator->ee->cop0.cause.ip1_pending = 1;
						}
					}
					else /* Read the next DMAtag */
					{
						fetch_tag(id);
					}
				}
			}
		}
	}
	
	void DMAController::fetch_tag(uint32_t id)
	{
		DMATag tag;
		auto& channel = channels[id];
		switch (id)
		{
		case DMAChannels::VIF1:
		{
			assert(channel.control.mode == 1);
			assert(!channel.tag_address.mem_select);

			auto& vif1 = emulator->vif[1];
			auto address = channel.tag_address.address;

			tag.value = *(uint128_t*)&emulator->ee->ram[address];
			fmt::print("[DMAC] Read VIF1 DMA tag {:#x}\n", (uint64_t)tag.value);

			/* Transfer the tag before any data */
			if (channel.control.transfer_tag && !vif1->write_fifo<uint64_t>(NULL, tag.data))
				return;

			/* Update channel from tag */
			channel.qword_count = tag.qwords;
			channel.control.tag = (tag.value >> 16) & 0xffff;

			uint16_t tag_id = tag.id;
			switch (tag_id)
			{
			case DMASourceID::CNT:
				channel.address = channel.tag_address.address + 16;
				channel.tag_address.value = channel.address + channel.qword_count * 16;
				break;
			case DMASourceID::NEXT:
				channel.address = channel.tag_address.address + 16;
				channel.tag_address.address = tag.address;
				break;
			default:
				fmt::print("\n[DMAC] Unrecognized VIF1 DMAtag id {:d}\n", tag_id);
				std::abort();
			}

			if (channel.control.enable_irq_bit && tag.irq)
			{
				/* Just end transfer, since an interrupt will be raised there anyways */
				channel.end_transfer = true;
			}

			break;
		}
		case DMAChannels::SIF0:
		{
			auto& sif = emulator->sif;
			if (sif->sif0_fifo.size() >= 2)
			{
				uint32_t data[2] = {};
				for (int i = 0; i < 2; i++)
				{
					data[i] = sif->sif0_fifo.front();
					sif->sif0_fifo.pop();
				}

				tag.value = *(uint64_t*)data;
				fmt::print("[DMAC] Read SIF0 DMA tag {:#x}\n", (uint64_t)tag.value);

				/* Update channel from tag */
				channel.qword_count = tag.qwords;
				channel.control.tag = (tag.value >> 16) & 0xffff;
				channel.address = tag.address;
				channel.tag_address.address += 16;

				fmt::print("[DMAC] QWC: {:d}\nADDR: {:#x}\n", channel.qword_count, channel.address);

				/* Just end transfer, since an interrupt will be raised there anyways */
				if (channel.control.enable_irq_bit && tag.irq)
					channel.end_transfer = true;
			}
			break;
		}
		case DMAChannels::SIF1:
		{
			assert(channel.control.mode == 1);
			assert(!channel.tag_address.mem_select);

			auto address = channel.tag_address.address;

			/* Get tag from memory */
			/* TODO: Access to other memory types */
			tag.value = *(uint128_t*)&emulator->ee->ram[address];
			fmt::print("[DMAC] Read SIF1 DMA tag {:#x}\n", (uint64_t)tag.value);

			/* Update channel from tag */
			channel.qword_count = tag.qwords;
			channel.control.tag = (tag.value >> 16) & 0xffff;

			uint16_t tag_id = tag.id;
			switch (tag_id)
			{
			case DMASourceID::REFE:
			{
				/* MADR=DMAtag.ADDR
				   TADR+=16
				   tag_end=true */
				channel.address = tag.address;
				channel.tag_address.value += 16;
				channel.end_transfer = true;
				break;
			}
			case DMASourceID::NEXT:
			{
				channel.address = channel.tag_address.address + 16;
				channel.tag_address.value = tag.address;
				break;
			}
			case DMASourceID::REF:
			{
				/* MADR=DMAtag.ADDR
				   TADR+=16 */
				channel.address = tag.address;
				channel.tag_address.value += 16;
				break;
			}
			default:
				fmt::print("\n[DMAC] Unrecognized SIF1 DMAtag id {:d}\n", tag_id);
				std::abort();
			}

			if (channel.control.enable_irq_bit && tag.irq)
			{
				/* Just end transfer, since an interrupt will be raised there anyways */
				channel.end_transfer = true;
			}

			break;
		}
		default:
			fmt::print("[DMAC] Unknown channel {:d}\n", id);
			std::abort();
		}
	}
}