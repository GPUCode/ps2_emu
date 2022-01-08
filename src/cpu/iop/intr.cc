#include "intr.h"
#include <cpu/iop/intr.h>
#include <cpu/iop/iop.h>

namespace iop
{
	INTR::INTR(IOProcessor* parent) :
		iop(parent)
	{
	}

	uint64_t INTR::read(uint32_t addr)
	{
		uint32_t offset = (addr & 0xf) >> 2;
		auto ptr = (uint32_t*)&regs + offset;
		auto value = *ptr;

		/* Reading I_CTRL returns AND clears it */
		if (offset == 2)
		{
			regs.i_ctrl = 0;
		}

		return value;
	}

	void INTR::write(uint32_t addr, uint64_t data)
	{
		uint32_t offset = (addr & 0xf) >> 2;
		auto ptr = (uint32_t*)&regs + offset;

		/* Writing to I_STAT (offset == 0) is special */
		*ptr = (offset == 0 ? *ptr & data : data);
	}
	
	void INTR::trigger(Interrupt intr)
	{
		/* Set the appropriate interrupt bit */
		regs.i_stat |= (1 << (uint32_t)intr);
	}

	bool INTR::interrupt_pending()
	{
		auto& cop0 = iop->cop0;
		
		/* If I_CTRL && (I_STAT & I_MASK), then COP0.Cause:8 is set */
		/* NOTE: ps2tek says !I_CTRL but apparently that's wrong */
		bool pending = regs.i_ctrl && (regs.i_stat & regs.i_mask);
		cop0.cause.IP = (cop0.cause.IP & ~0x4) | (pending << 2);
		
		bool enabled = cop0.status.IEc && (cop0.status.Im & cop0.cause.IP);
		return pending && enabled;
	}
}