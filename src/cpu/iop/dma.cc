#include <cpu/iop/dma.h>
#include <common/emulator.h>
#include <cpu/iop/iop.h>
#include <common/sif.h>
#include <spu/spu.h>
#include <media/sio2.h>
#include <fmt/color.h>
#include <cassert>

constexpr const char* REGS[] =
{
	"Dn_MADR", "Dn_BCR",
	"Dn_CHCR", "Dn_TADR"
};

constexpr const char* GLOBALS[] =
{
	"DPCR", "DICR", "DPCR2",
	"DICR2", "DMACEN", "DMACINTEN"
};

namespace iop
{
	/* DMA Controller class implementation. */
	DMAController::DMAController(common::Emulator* emu) :
		emulator(emu)
	{
		/* Register our functions to the Emulator. */
		emulator->add_handler(0x1f801080, this, &DMAController::read, &DMAController::write);
		emulator->add_handler(0x1f801500, this, &DMAController::read, &DMAController::write);
	}

	uint32_t DMAController::read(uint32_t address)
	{
		uint16_t group = (address >> 8) & 0x1;

		/* Read globals */
		if ((address & 0x70) == 0x70)
		{
			uint16_t offset = ((address & 0xf) >> 2) + 2 * group;
			auto ptr = (uint32_t*)&globals + offset;
			fmt::print("[IOP DMA] Reading {:#x} from global register {}\n", *ptr, GLOBALS[offset]);
			return *ptr;
		}
		else /* Read from channel */
		{
			uint16_t channel = ((address & 0x70) >> 4) + group * 7;
			uint16_t offset = (address & 0xf) >> 2;
			auto ptr = (uint32_t*)&channels[channel] + offset;
			fmt::print("[IOP DMA] Reading {:#x} from {} in channel {:d}\n", *ptr, REGS[offset], channel);
			return *ptr;
		}
	}

	void DMAController::write(uint32_t address, uint32_t data)
	{
		/* Get channel information from address. */
		uint16_t group = (address >> 8) & 0x1;

		/* Write globals */
		if ((address & 0x70) == 0x70)
		{
			uint16_t offset = ((address & 0xf) >> 2) + 2 * group;
			auto ptr = (uint32_t*)&globals + offset;
			fmt::print("[IOP DMA] Writing {:#x} to {}\n", data, GLOBALS[offset]);

			/* Handle special case for writing to DICR */
			if (offset == 1)
			{
				auto& irq = globals.dicr;
				auto flags = irq.flags;
				
				/* Writing 1 to the interrupt flags clears them */
				irq.value = data;
				irq.flags = flags & ~((data >> 24) & 0x7f);
				/* Update master interrupt flag */
				irq.master_flag = irq.force || (irq.master_enable && ((irq.enable & irq.flags) > 0));
			}
			else if (offset == 3)
			{
				auto& irq = globals.dicr2;
				auto flags = irq.flags;

				/* Writing 1 to the interrupt flags clears them */
				irq.value = data;
				irq.flags = flags & ~((data >> 24) & 0x7f);
			}
			else
			{
				*ptr = data;
			}
		}
		else /* Write channels */
		{
			uint16_t channel = ((address & 0x70) >> 4) + group * 7;
			uint16_t offset = (address & 0xf) >> 2;
			auto ptr = (uint32_t*)&channels[channel] + offset;
			
			fmt::print("[IOP DMA] Writing {:#x} to {} of channel {:d}\n", data, REGS[offset], channel);
			*ptr = data;

			if (channels[channel].control.running)
			{
				fmt::print("\n[IOP DMA] Started transfer on channel {:d}\n", channel);
			}
		}
	}

	void DMAController::tick(uint32_t cycles)
	{
		for (int cycle = cycles; cycle > 0; cycle--)
		{
			/* Iterate new channels */
			for (int id = 7; id < 13; id++)
			{
				auto& channel = channels[id];
				bool enable = globals.dpcr2 & (1 << ((id - 7) * 4 + 3));

				/* If channel was enabled and has a pending DMA request */
				if (channel.control.running && enable)
				{
					/* Transfer any pending words */
					if (channel.block_conf.count > 0)
					{
						switch (id)
						{
						case DMAChannels::SPU2:
						{
							channel.block_conf.count--;
							break;
						}
						case DMAChannels::SIF0:
						{
							auto& sif = emulator->sif;
							auto data = *(uint32_t*)&emulator->iop->ram[channel.address];
							channel.address += 4;
							channel.block_conf.count--;
							sif->sif0_fifo.push(data);

							break;
						}
						case DMAChannels::SIF1:
						{
							auto& sif = emulator->sif;
							if (!sif->sif1_fifo.empty())
							{
								auto data = sif->sif1_fifo.front();
								sif->sif1_fifo.pop();

								*(uint32_t*)&emulator->iop->ram[channel.address] = data;
								channel.address += 4;
								channel.block_conf.count--;
							}
							break;
						}
						case DMAChannels::SIO2in:
						{
							auto& sio2 = emulator->sio2;
							auto bytes = /*channel.block_conf.count * channel.block_conf.size */ 4;
							for (int i = 0; i < bytes; i++)
							{
								uint8_t cmd = emulator->iop->ram[channel.address + i];
								sio2->upload_command(cmd);
							}

							channel.address += bytes;
							channel.block_conf.count--;
							if (channel.block_conf.count == 0)
							{
								channel.end_transfer = true;
							}

							break;
						}
						case DMAChannels::SIO2out:
						{
							auto& sio2 = emulator->sio2;
							auto bytes = /*channel.block_conf.count * channel.block_conf.size */ 4;
							for (int i = 0; i < bytes; i++)
							{
								emulator->iop->ram[channel.address + i] = sio2->read_fifo();
							}

							channel.address += bytes;
							channel.block_conf.count--;
							if (channel.block_conf.count == 0)
							{
								channel.end_transfer = true;
							}

							break;
						}
						default:
							common::Emulator::terminate("[IOP DMA] Unknown channel {:d} needs data transfer!\n", id);
						}
					}
					else if (channel.end_transfer)
					{
						/* HACK: Trigger interrupt when SPU transfers */
						if (id == DMAChannels::SPU2)
						{
							emulator->spu2->trigger_irq();
						}

						channel.control.running = 0;
						channel.end_transfer = false;
						globals.dicr2.flags |= (1 << (id - 7));

						if (globals.dicr2.flags & globals.dicr2.mask)
						{
							fmt::print("[IOP DMA] Channel {:d} raised interrupt!\n", id);
							emulator->iop->intr.trigger(Interrupt::DMA);
						}
					}
					else
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

		uint32_t* data;
		switch (id)
		{
		case DMAChannels::SPU2:
		{
			/* If chain mode is requested, I want to be notified */
			assert(channel.control.transfer_mode == 1);
			channel.end_transfer = true;

			/* Do nothing */
			break;
		}
		case DMAChannels::SIF0:
		{
			auto& sif = emulator->sif;

			/* SIF0 uses TADR */
			tag.value = *(uint64_t*)&emulator->iop->ram[channel.tadr];

			fmt::print("\n[IOP DMA] Read SIF0 DMA tag {:#x}\n", tag.value);
			channel.address = tag.address;
			/* PCSX2 and DobieStaion round the transfer size to the nearest
			   multiple of 4. This behaviour isn't confirmed on the hardware
			   but I will write some hardware tests for it sometime */
			channel.block_conf.count = (tag.tranfer_size + 3) & 0xfffffffc;
			channel.tadr += 8;

			fmt::print("[IOP DMA] New channel address: {:#x}\n", channel.address);
			fmt::print("[IOP DMA] Words to transfer: {:d}\n\n", channel.block_conf.count);

			/* If Dn_CHCR.8 is set, 2 words after the tag in 
			   memory (TADR + 8) will be transferred first */
			if (channel.control.bit_8)
			{
				uint32_t* data = (uint32_t*)&emulator->iop->ram[channel.tadr];
				channel.tadr += 8;

				/* Push DMAtag to the EE's DMAC */
				sif->sif0_fifo.push(data[0]);
				sif->sif0_fifo.push(data[1]);
			}

			if (tag.end_transfer || tag.irq)
			{
				channel.end_transfer = true;
			}

			break;
		}
		case DMAChannels::SIF1:
		{
			auto& sif = emulator->sif;
			/* Only read if the fifo has data in it */
			if (sif->sif1_fifo.size() >= 4)
			{
				uint32_t data[2];
				for (int i = 0; i < 2; i++)
				{
					data[i] = sif->sif1_fifo.front();
					sif->sif1_fifo.pop();
				}

				/* The EE always sends qwords (4 bytes). Because our tag 
				   is 2 bytes the rest is just padding */
				sif->sif1_fifo.pop();
				sif->sif1_fifo.pop();

				tag.value = *(uint64_t*)data;
				fmt::print("\n[IOP DMA] Read SIF1 DMA tag {:#x}\n", tag.value);

				channel.address = tag.address;
				channel.block_conf.count = tag.tranfer_size;

				fmt::print("[IOP DMA] New channel address: {:#x}\n", channel.address);
				fmt::print("[IOP DMA] Words to transfer: {:d}\n\n", channel.block_conf.count);

				if (tag.end_transfer || tag.irq)
				{
					channel.end_transfer = true;
				}
			}
			break;
		}
		default:
			common::Emulator::terminate("[IOP DMA] Unknown channel {:d}\n", id);
		}
	}
}