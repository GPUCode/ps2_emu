#pragma once
#include <common/component.h>

namespace gs
{
	/* These can be accessed from the EE */
	struct PrivRegs
	{
		uint64_t PMODE;
		uint64_t SMODE1;
		uint64_t SMODE2;
		uint64_t SRFSH;
		uint64_t SYNCH1;
		uint64_t SYNCH2;
		uint64_t SYNCV;
		uint64_t DISPFB1;
		uint64_t DISPLAY1;
		uint64_t DISPFB2;
		uint64_t DISPLAY2;
		uint64_t EXTBUF;
		uint64_t EXTDATA;
		uint64_t EXTWRITE;
		uint64_t BGCOLOR;
		uint64_t GS_CSR;
		uint64_t GS_IMR;
		uint64_t BUSDIR;
		uint64_t SIGLBLID;
	};

	class GS : public common::Component
	{

	};
}