#pragma once
#include <cstdint>

namespace iop
{
	enum class SyncType : uint32_t
	{
		Manual = 0,
		Request = 1,
		Linked_List = 2
	};

	enum DMAChannels : uint32_t
	{
		MDECin = 0x0,
		MDECout = 0x1,
		GPU = 0x2,
		CDROM = 0x3,
		SPU = 0x4,
		PIO = 0x5,
		OTC = 0x6
	};

	/* General info about DMA for each channel. */
	union DMAControlReg
	{
		uint32_t value;
		struct
		{
			uint32_t trans_dir : 1;
			uint32_t addr_step : 1;
			uint32_t : 6;
			uint32_t chop_enable : 1;
			SyncType sync_mode : 2;
			uint32_t : 5;
			uint32_t chop_dma : 3;
			uint32_t : 1;
			uint32_t chop_cpu : 3;
			uint32_t : 1;
			uint32_t enable : 1;
			uint32_t : 3;
			uint32_t trigger : 1;
			uint32_t : 3;
		};
	};

	/* Holds info about block size and count. */
	union DMABlockReg
	{
		uint32_t value;
		struct
		{
			uint16_t block_size;
			uint16_t block_count; /* Only used in Request sync mode. */
		};
	};

	/* A DMA channel */
	struct DMAChannel
	{
		uint32_t base;
		DMABlockReg block;
		DMAControlReg control;
	};

	/* DMA Interrupt Register. */
	union DMAIRQReg
	{
		uint32_t value;
		struct
		{
			uint32_t : 15;
			uint32_t force : 1;
			uint32_t enable : 7;
			uint32_t master_enable : 1;
			uint32_t flags : 7;
			uint32_t master_flag : 1;
		};
	};

	union ListPacket
	{
		uint32_t value;
		struct
		{
			uint32_t next_addr : 24;
			uint32_t size : 8;
		};
	};

	/* A class that manages all DMA routines. */
	class ComponentManager;
	class DMAController {
	public:
		DMAController(ComponentManager* manager);

		void tick();
		void transfer_finished(DMAChannels channel);

		void start(DMAChannels channel);
		void block_copy(DMAChannels channel);
		void list_copy(DMAChannels channel);

		uint32_t read(uint32_t address);
		void write(uint32_t address, uint32_t data);

	public:
		uint32_t control;
		DMAIRQReg irq;
		DMAChannel channels[7];

		bool irq_pending = false;
		ComponentManager* manager;
	};
}