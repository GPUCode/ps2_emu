#pragma once
#include <cstdint>

namespace gs
{
	union RGBAQ
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

	struct GSImpl
	{
		RGBAQ rbgaq;
		uint64_t st, uv;
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
		uint64_t frame[2], zbuf[2], bitbltbuf;
		uint64_t trxpos, trxreg, trxdir;
	};
}