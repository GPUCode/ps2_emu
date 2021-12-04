#include <cpu/iop/dma.h>
#include <common/memory.h>
#include <fmt/color.h>

namespace iop
{
	/* DMA Controller class implementation. */
	DMAController::DMAController(ComponentManager* manager) :
		manager(manager)
	{
	}

	void DMAController::tick()
	{
	}

	void DMAController::transfer_finished(DMAChannels dma_channel)
	{
		DMAChannel& channel = channels[dma_channel];

		/* IRQ flags in Bit(24+n) are set upon DMAn completion -
		but caution - they are set ONLY if enabled in Bit(16+n).*/
		if (irq.enable & (1 << dma_channel) || irq.master_enable)
		{
			irq.flags |= 1 << dma_channel;
		}

		/* The master flag is a simple readonly flag that follows the following rules:
		   IF b15=1 OR (b23=1 AND (b16-22 AND b24-30)>0) THEN b31=1 ELSE b31=0
		   Upon 0-to-1 transition of Bit31, the IRQ3 flag (in Port 1F801070h) gets set.
		   Bit24-30 are acknowledged (reset to zero) when writing a "1" to that bits */
		bool previous = irq.master_flag;
		irq.master_flag = irq.force || (irq.master_enable && ((irq.enable & irq.flags) > 0));

		if (irq.master_flag && !previous) 
		{
			irq_pending = true;
		}
	}

	void DMAController::start(DMAChannels dma_channel)
	{
		DMAChannel& channel = channels[dma_channel];

		/* Start linked list copy routine. */
		if (channel.control.sync_mode == SyncType::Linked_List)
		{
			list_copy(dma_channel);
		}
		else /* Start block copy routine. */
		{
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

	uint32_t DMAController::read(uint32_t address)
	{
		uint32_t off = iop::DMA.offset(address);

		/* Get channel information from address. */
		uint32_t channel_num = (off & 0x70) >> 4;
		uint32_t offset = off & 0xf;

		/* One of the main channels is enabled. */
		if (channel_num >= 0 && channel_num <= 6) 
		{
			DMAChannel& channel = channels[channel_num];
			switch (offset) 
			{
			case 0:
				return channel.base;
			case 4:
				return channel.block.value;
			case 8:
				return channel.control.value;
			default:
				fmt::print("[IOP DMA] Unhandled DMA read at offset: {:#x}\n", off);
				std::abort();
			}
		} /* One of the primary registers is selected. */
		else if (channel_num == 7) 
		{

			switch (offset) 
			{
			case 0:
				return control;
			case 4:
				return irq.value;
			default:
				fmt::print("[IOP DMA] Unhandled DMA read at offset: {:#x}\n", offset);
				std::abort();
			}
		}

		fmt::print("[IOP DMA] Invalid channel number {:d}\n", channel_num);
		return 0;
	}

	void DMAController::write(uint32_t address, uint32_t val)
	{
		uint32_t off = iop::DMA.offset(address);

		/* Get channel information from address. */
		uint32_t channel_num = (off & 0x70) >> 4;
		uint32_t offset = off & 0xf;

		uint32_t active_channel = INT_MAX;
		/* One of the main channels is enabled. */
		if (channel_num >= 0 && channel_num <= 6) 
		{
			DMAChannel channel = channels[channel_num];
			switch (offset) {
			case 0:
				channel.base = val & 0xffffff;
				break;
			case 4:
				channel.block.value = val;
				break;
			case 8:
				channel.control.value = val;
				break;
			default:
				fmt::print("[IOP DMA] Unhandled DMA channel write at offset: {:#x}\n", off);
				std::abort();
			}

			/* Check if the channel was just activated. */
			bool trigger = true;
			if (channel.control.sync_mode == SyncType::Manual)
				trigger = channel.control.trigger;

			if (channel.control.enable && trigger)
				active_channel = channel_num;
		} /* One of the primary registers is selected. */
		else if (channel_num == 7) 
		{
			switch (offset) 
			{
				case 0:
					control = val;
					break;
				case 4:
					irq.value = val;
					irq.master_flag = irq.force || (irq.master_enable && ((irq.enable & irq.flags) > 0));
					break;
				default:
					fmt::print("[IOP DMA] Unhandled DMA write at offset: {:#x}\n", offset);
					std::abort();
			}
		}

		/* Start DMA if a channel was just activated. */
		if (active_channel != INT_MAX)
			start((DMAChannels)active_channel);
	}
}