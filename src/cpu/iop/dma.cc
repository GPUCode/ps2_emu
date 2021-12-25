#include <cpu/iop/dma.h>
#include <common/emulator.h>
#include <fmt/color.h>
#include <cassert>

namespace iop
{
	/* DMA Controller class implementation. */
	DMAController::DMAController(common::Emulator* emu) :
		emulator(emu)
	{
		/* Register our functions to the Emulator. */
		const uint32_t PAGE1 = common::Emulator::calculate_page(0x1f801080);
		const uint32_t PAGE2 = common::Emulator::calculate_page(0x1f801500);
		
		emulator->add_handler(PAGE1, this, &DMAController::read, &DMAController::write);
		emulator->add_handler(PAGE2, this, &DMAController::read, &DMAController::write);
	}

	void DMAController::tick()
	{
	}

	void DMAController::transfer_finished(DMAChannels dma_channel)
	{
	}

	void DMAController::start(DMAChannels dma_channel)
	{
		DMAChannel& channel = channels[dma_channel];

		/* Start linked list copy routine. */
		switch (channel.channel_ctrl.transfer_mode)
		{
		case TransferMode::Linked_List:	
			list_copy(dma_channel); break;
		default:
			block_copy(dma_channel);
		}

		/* Complete the transfer. */
		transfer_finished(dma_channel);
	}

	void DMAController::block_copy(DMAChannels dma_channel)
	{
	}

	void DMAController::list_copy(DMAChannels dma_channel)
	{
	}

	uint64_t DMAController::read(uint32_t address)
	{
		assert(address >= 0x1F801080 && address <= 0x1F8010F4 || address >= 0x1F801500 && address <= 0x1F80157C);

		/* Get channel information from address. */
		bool group = address & 0x100;
		uint16_t channel_num = (address & 0x70) >> 4;
		uint16_t offset = (address & 0xf) >> 2;
		
		uint32_t* ptr = nullptr;
		if (channel_num >= 0 && channel_num <= 6) 
		{
			ptr = (uint32_t*)&channels[channel_num + group * 7] + offset;
		}
		else if (channel_num == 7) 
		{
			ptr = (uint32_t*)&globals + 2 * offset + group;
		}
		else
		{
			fmt::print("[IOP DMA] Invalid channel number {:d}\n", channel_num);
			std::abort();
		}

		fmt::print("[IOP DMA] Reading {:#x} from DMA channel {:d} at address {:#x}\n", *ptr, channel_num, address);
		return *ptr;
	}

	void DMAController::write(uint32_t address, uint64_t data)
	{
		assert(address >= 0x1F801080 && address <= 0x1F8010F4 || address >= 0x1F801500 && address <= 0x1F80157C);

		/* Get channel information from address. */
		bool group = address & 0x100;
		uint16_t channel_num = (address & 0x70) >> 4;
		uint16_t offset = (address & 0xf) >> 2;

		fmt::print("[IOP DMA] Writing {:#x} to DMA channel {:d} at address {:#x}\n", data, channel_num, address);

		if (channel_num >= 0 && channel_num <= 6) 
		{
			auto ptr = (uint32_t*)&channels[channel_num + group * 7] + offset;
			*ptr = data;
		}
		else if (channel_num == 7) 
		{
			auto ptr = (uint32_t*)&globals + 2 * offset + group;
			*ptr = data;

			if (offset == 4 && !group)
			{
				/* Handle special case for writing to DICR */
				auto& irq = globals.dicr;
				irq.master_flag = irq.force || (irq.master_enable && ((irq.enable & irq.flags) > 0));
			}
		}
		else
		{
			fmt::print("[IOP DMA] Invalid channel number {:d}\n", channel_num);
			std::abort();
		}
	}
}