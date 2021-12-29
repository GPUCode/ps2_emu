#include <cpu/ee/intc.h>
#include <cpu/ee/ee.h>

static const char* REGS[2] =
{
	"INTC_STAT",
	"INTC_STAT"
};

namespace ee
{
	INTC::INTC(EmotionEngine* ee) :
		cpu(ee)
	{
		/* Clear registers */
		regs = {};
	}

	uint64_t INTC::read(uint32_t addr)
	{
		auto offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&regs + offset;

		fmt::print("[INTC] Reading {:#x} from {}\n", *ptr, REGS[offset]);
		return *ptr;
	}

	void INTC::write(uint32_t addr, uint64_t data)
	{
		auto offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&regs + offset;

		/* Writing 1 to the nth bit of INTC_STAT (offset 0) clears it,
		   while doing that to INTC_MASK (offset 1) reverses it */
		*ptr = (offset ? *ptr ^ data : *ptr & ~data);

		/* Set appropriate COP0 status bits */
		cpu->cop0.cause.ip0_pending = (regs.intc_mask & regs.intc_stat);

		fmt::print("[INTC] Writing {:#x} to {}\n", data, REGS[offset]);
	}

	void INTC::trigger(uint32_t intr)
	{
		fmt::print("[INTC] Triggering interrupt {:d}\n", intr);
		regs.intc_stat |= (1 << intr);

		/* Set appropriate COP0 status bits */
		cpu->cop0.cause.ip0_pending = (regs.intc_mask & regs.intc_stat);
	}

	bool INTC::int_pending()
	{
		auto& cop0 = cpu->cop0;

		/* Only when both The EIE and IE bits are set to 1
		   (in addition, when the ERL and EXL bits are 0),
		   interrupts are enabled. EE Core Users Manual [74]  */
		bool int_enabled = cop0.status.eie && cop0.status.ie && !cop0.status.erl && !cop0.status.exl;

		/* The interrupt exception occurs if one of the three interrupt signals is asserted.
		   EE Core Users Manual [99] */
		bool pending = (cop0.cause.ip0_pending && cop0.status.im0) ||
			(cop0.cause.ip1_pending && cop0.status.im1) ||
			(cop0.cause.timer_ip_pending && cop0.status.im7);

		/* The processor recognizes an interrupt only when the corresponding IM bits,
		   the IP bit of the Cause register, and the IE field of the Status register are set to 1 */
		return int_enabled && pending;
	}
}