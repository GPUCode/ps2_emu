#pragma once
#include <common/component.h>
#include <gs/gsvram.h>

namespace common
{
	class Emulator;
}

namespace gs
{
	union GS_CSR
	{
		uint64_t value;
		struct 
		{
			uint64_t signal : 1;
			uint64_t finish : 1;
			uint64_t hsint : 1;
			uint64_t vsint : 1;
			uint64_t edwint : 1;
			uint64_t zero : 2;
			uint64_t : 1;
			uint64_t flush : 1;
			uint64_t reset : 1;
			uint64_t : 2;
			uint64_t nfield : 1;
			uint64_t field : 1;
			uint64_t fifo : 2;
			uint64_t rev : 8;
			uint64_t id : 8;
			uint64_t : 32;
		};
	};

	/* These can be accessed from the EE */
	struct GSPRegs
	{
		uint64_t pmode;
		uint64_t smode1;
		uint64_t smode2;
		uint64_t srfsh;
		uint64_t synch1;
		uint64_t synch2;
		uint64_t syncv;
		uint64_t dispfb1;
		uint64_t display1;
		uint64_t dispfb2;
		uint64_t display2;
		uint64_t extbuf;
		uint64_t extdata;
		uint64_t extwrite;
		uint64_t bgcolor;
		GS_CSR csr;
		uint64_t imr;
		uint64_t busdir;
		uint64_t siglblid;
	};

	union RGBAQReg
	{
		uint64_t value;
		struct
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
			float q;
		};
	};

	union XYZ
	{
		uint64_t value;
		struct
		{
			uint64_t x : 16;
			uint64_t y : 16;
			uint64_t z : 32;
		};
	};

	union BITBLTBUF
	{
		uint64_t value;
		struct
		{
			uint64_t source_base : 14;
			uint64_t : 2;
			uint64_t source_width : 6;
			uint64_t : 2;
			uint64_t source_pixel_format : 6;
			uint64_t : 2;
			uint64_t dest_base : 14;
			uint64_t : 2;
			uint64_t dest_width : 6;
			uint64_t : 2;
			uint64_t dest_pixel_format : 6;
		};
	};

	union TRXPOS
	{
		uint64_t value;
		struct
		{
			uint64_t source_top_left_x : 11;
			uint64_t : 5;
			uint64_t source_top_left_y : 11;
			uint64_t : 5;
			uint64_t dest_top_left_x : 11;
			uint64_t : 5;
			uint64_t dest_top_left_y : 11;
			uint64_t dir : 2;
		};
	};

	union TRXREG
	{
		uint64_t value;
		struct
		{
			uint64_t width : 12;
			uint64_t : 20;
			uint64_t height : 12;
		};
	};

	struct GSRegs
	{
		uint64_t prim;
		RGBAQReg rgbaq;
		uint64_t st, uv;
		XYZ xyz2, xyz3;
		uint64_t tex0[2], tex1[2], tex2[2];
		uint64_t clamp[2];
		uint64_t fog, fogcol;
		uint64_t xyoffset[2];
		uint64_t prmodecont, prmode;
		uint64_t texclut;
		uint64_t scanmsk;
		uint64_t miptbp1[2], miptbp2[2];
		uint64_t texa, texflush;
		uint64_t scissor[2];
		uint64_t alpha[2];
		uint64_t dimx, dthe;
		uint64_t colclamp;
		uint64_t test[2];
		uint64_t pabe, fba[2];
		uint64_t frame[2], zbuf[2];
		BITBLTBUF bitbltbuf;
		TRXPOS trxpos;
		TRXREG trxreg;
		uint64_t trxdir;
	};

	enum TRXDir : uint8_t
	{
		HostLocal = 0,
		LocalHost = 1,
		LocalLocal = 2,
		None = 3
	};

	struct GraphicsSynthesizer : public common::Component
	{
		friend struct GIF;
		GraphicsSynthesizer(common::Emulator* parent);
		~GraphicsSynthesizer();

		/* Used by the EE */
		uint64_t read_priv(uint32_t addr);
		void write_priv(uint32_t addr, uint64_t data);

		/* Used by the GIF */
		uint64_t read(uint16_t addr);
		void write(uint16_t addr, uint64_t data);

		/* Transfer data to/from VRAM */
		void write_hwreg(uint64_t data);

	private:
		common::Emulator* emulator;
		GSPRegs priv_regs = {};
		GSRegs regs = {};

		/* GS VRAM is divided into 8K pages */
		Page* vram = nullptr;
		/* Used to track how many pixels where written during a transfer */
		int data_written = 0;
	};
}