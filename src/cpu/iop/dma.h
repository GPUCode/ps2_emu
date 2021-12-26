#pragma once
#include <common/component.h>
#include <cstdint>

namespace common
{
	class Emulator;
}

namespace iop
{
	enum class TransferMode : uint32_t
	{
		Burst,
		Slice,
		Linked_List,
		Chain
	};

	enum DMAChannels : uint32_t
	{
		MDECin,
		MDECout,
		SIF2,
		CDVD,
		SPU1,
		PIO,
		OTC,
		SPU2,
		DEV9,
		SIF0,
		SIF1,
		SIO2in,
		SIO2out
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
			TransferMode transfer_mode : 2;
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
			uint16_t size; /* Block size in words. */
			uint16_t count; /* Only used in Request sync mode. */
		};
	};

	/* A DMA channel */
	struct DMAChannel
	{
		uint32_t base;
		DMABlockReg block_ctrl;
		DMAControlReg channel_ctrl;
		uint32_t tadr;
	};

	union DICR
	{
		uint32_t value;
		struct
		{
			uint32_t completion : 7;
			uint32_t : 8;
			uint32_t force : 1;
			uint32_t enable : 7;
			uint32_t master_enable : 1;
			uint32_t flags : 7;
			uint32_t master_flag : 1;
		};
	};

	union DICR2
	{
		uint32_t value;
		struct
		{
			uint32_t tag : 13;
			uint32_t : 3;
			uint32_t mask : 6;
			uint32_t : 2;
			uint32_t flags : 6;
			uint32_t : 2;
		};
	};

	/* The standalone registers */
	struct DMAGlobals
	{
		uint32_t dpcr[2];
		DICR dicr;
		DICR2 dicr2;
		uint32_t pad1; /* Added so we can read the struct nicely using pointer arithmetic */
		uint32_t dmacen;
		uint32_t pad2;
		uint32_t dmacinten;
	};

	/* A class that manages all DMA routines. */
	class DMAController : public common::Component
	{
	public:
		DMAController(common::Emulator* emu);

		void tick();
		void transfer_finished(DMAChannels channel);

		void start(DMAChannels channel);
		void block_copy(DMAChannels channel);
		void list_copy(DMAChannels channel);

		uint32_t read(uint32_t address);
		void write(uint32_t address, uint32_t data);

	public:
		DMAChannel channels[13] = {};
		DMAGlobals globals = {};

		bool irq_pending = false;
		common::Emulator* emulator = nullptr;
	};
}