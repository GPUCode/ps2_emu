#pragma once
#include <cstdint>

namespace gs
{
	union RGBAQREG
	{
		uint64_t value;
		struct
		{
			uint64_t r : 8;
			uint64_t g : 8;
			uint64_t b : 8;
			uint64_t a : 8;
			uint64_t q : 32;
		};
	};

	/* All of the internal GS registers are here */
	struct GSRegs
	{
		uint64_t prim;
		RGBAQREG rgbaq;
		uint64_t st;
		uint64_t uv;
		uint64_t xyzf2;
		uint64_t xyz2;
		uint64_t tex0_1;
		uint64_t tex0_2;
		uint64_t clamp_1;
		uint64_t clamp_2;
		uint64_t fog;
		uint64_t xyzf3;
		uint64_t pad0;
		uint64_t xyz3;
		uint64_t pad1[6]; /* Used so we can read the struct correctly */
		uint64_t tex1_1;
		uint64_t tex1_2;
		uint64_t tex2_1;
		uint64_t tex2_2;
		uint64_t xyoffset_1;
		uint64_t xyoffset_2;
		uint64_t prmodecont;
		uint64_t prmode;
		uint64_t texclut;
		uint64_t pad2[5];
		uint64_t scanmsk;
		uint64_t pad3[17];
		uint64_t miptbp1_1;
		uint64_t miptbp1_2;
		uint64_t miptbp2_1;
		uint64_t miptbp2_2;
		uint64_t pad4[3];
		uint64_t texa;
		uint64_t pad5;
		uint64_t fogcol;
		uint64_t pad6;
		uint64_t texflush;
		uint64_t scissor_1;
		uint64_t scissor_2;
		uint64_t alpha_1;
		uint64_t alpha_2;
		uint64_t dimx;
		uint64_t dthe;
		uint64_t colclamp;
		uint64_t test_1;
		uint64_t test_2;
		uint64_t pabe;
		uint64_t fba_1;
		uint64_t fba_2;
		uint64_t frame_1;
		uint64_t frame_2;
		uint64_t zbuf_1;
		uint64_t zbuf_2;
		uint64_t bitbltbuf;
		uint64_t trxpos;
		uint64_t trxreg;
		uint64_t trxdir;
		uint64_t hwreg;
		uint64_t pad7[11];
		uint64_t signal;
		uint64_t finish;
		uint64_t label;
	};
}