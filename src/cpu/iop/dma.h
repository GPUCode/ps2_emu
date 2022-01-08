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

	/* General info about DMA for each channel. */
	union DCHCR
	{
		uint32_t value;
		struct
		{
			uint32_t direction : 1;
			uint32_t increment : 1;
			uint32_t : 6;
			uint32_t bit_8 : 1;
			uint32_t transfer_mode : 2;
			uint32_t : 5;
			uint32_t chop_dma : 3;
			uint32_t : 1;
			uint32_t chop_cpu : 3;
			uint32_t : 1;
			uint32_t running : 1;
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
			uint16_t size;
			uint16_t count;
		};
	};

	/* A DMA channel */
	struct DMAChannel
	{
		uint32_t address;
		DMABlockReg block_conf;
		DCHCR control;
		uint32_t tadr;

		/* Used by the emulator */
		bool end_transfer = false;
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
		/* Lower channel globals */
		uint32_t dpcr;
		DICR dicr;

		/* Upper channel globals */
		uint32_t dpcr2;
		DICR2 dicr2;
		uint32_t dmacen;
		uint32_t dmacinten;
	};

	union DMATag
	{
		uint64_t value;
		struct
		{
			uint64_t address : 24;
			uint64_t : 6;
			uint64_t irq : 1;
			uint64_t end_transfer : 1;
			uint64_t tranfer_size : 24;
		};
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

	/* A class that manages all DMA routines. */
	class DMAController : public common::Component
	{
	public:
		DMAController(common::Emulator* emu);

		void tick(uint32_t cycles);

		uint32_t read(uint32_t address);
		void write(uint32_t address, uint32_t data);

	private:
		void fetch_tag(uint32_t id);

	public:
		/* The last channel is used as a dummy */
		DMAChannel channels[14] = {};
		DMAGlobals globals = {};

		bool transfer_pending = false;
		common::Emulator* emulator = nullptr;
	};
}