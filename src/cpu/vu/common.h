#pragma once
#include <common/component.h>

namespace vu
{
	union Vector
	{
		/* The unsigned part is used to preserve bit data */
		uint128_t qword;
		uint32_t word[4];
		/* Float part is used for float operations */
		float fword[4];
		struct { float x, y, z, w; };
	};

	struct Registers
	{
		uint32_t vi[16];
		uint32_t control[16];
		Vector vf[32];
	};

	union VUInstr
	{
		uint32_t value;
		struct
		{
			uint32_t : 6;
			uint32_t id : 5;
			uint32_t is : 5;
			uint32_t it : 5;
			/* This acts as a bit mask that tells the CPU which fields
			   to write (X, Y, Z, W) */
			uint32_t dest : 4;
			uint32_t : 7;
		};
		struct
		{
			uint32_t : 6;
			uint32_t fd : 5;
			uint32_t fs : 5;
			uint32_t ft : 5;
			uint32_t : 11;
		};
	};
}