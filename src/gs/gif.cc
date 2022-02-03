#include <gs/gif.h>
#include <gs/gs.h>
#include <common/emulator.h>
#include <cassert>
#include <fmt/core.h>

static const char* REGS[10] =
{
	"GIF_CTRL",
	"GIF_MODE",
	"GIF_STAT",
	"GIF_TAG0",
	"GIF_TAG1",
	"GIF_TAG2",
	"GIF_TAG3",
	"GIF_CNT",
	"GIF_P3CNT",
	"GIF_P3TAG"
};

namespace gs
{
	GIF::GIF(common::Emulator* parent) :
		emulator(parent)
	{
		emulator->add_handler(0x10003000, this, &GIF::read, &GIF::write);
		emulator->add_handler<uint128_t>(0x10006000, this, nullptr, &GIF::write_path3);
	}
	
	uint32_t GIF::read(uint32_t addr)
	{
		uint32_t data;
		auto offset = (addr & 0xf0) >> 4;
		switch (offset)
		{
		case 2:
			/* TODO: repoth PATH status */
			status.fifo_count = fifo.size<uint128_t>();
			data = status.value;
			break;
		default:
			fmt::print("[GIF] Read from unknown register {}\n", REGS[offset]);
			std::abort();
		}

		return data;
	}
	
	void GIF::write(uint32_t addr, uint32_t data)
	{
		auto offset = (addr & 0xf0) >> 4;
		switch (offset)
		{
		case 0:
			control.value = data;
			
			/* Reset the GIF when the RST bit is set */
			if (control.reset)
				reset();
			
			break;
		default:
			fmt::print("[GIF] Write to unknown register {}\n", REGS[offset]);
			std::abort();
		}

		fmt::print("[GIF] Writing {:#x} to {}\n", data, REGS[offset]);
	}

	void GIF::tick(uint32_t cycles)
	{
		while (!fifo.empty() && cycles--)
		{
			if (!data_count)
				process_tag();
			else
				execute_command();
		}
	}

	void GIF::reset()
	{
		/* Reset all class members */
		control = {};
		mode = 0;
		status = {};
		fifo = {};
		tag = {};
		data_count = reg_count = 0;
		interal_Q = 0;
	}
	
	bool GIF::write_path3(uint32_t, uint128_t qword)
	{
		return fifo.push<uint128_t>(qword);
	}
	
	void GIF::process_tag()
	{
		if (fifo.read(&tag.value))
		{
			data_count = tag.nloop;
			reg_count = tag.nreg;

			auto& gs = emulator->gs;
			fmt::print("[GIF] Received new GS primitive!\n");

			/* Set the GS PRIM register to the PRIM field of GIFTag
			   NOTE: This only happens when PRE == 1 */
			gs->prim = tag.pre ? tag.prim : gs->prim;

			/* The initial value of Q is 1.0f and is set
			   when the GIFTag is read */
			gs->rgbaq.q = 1.0f;

			/* Remove tag from fifo */
			fifo.pop<uint128_t>();
		}
	}

	void GIF::execute_command()
	{
		uint128_t qword;
		if (fifo.read(&qword))
		{
			uint16_t format = tag.flg;
			switch (format)
			{
			case Format::Packed:
			{
				process_packed(qword);
				if (!reg_count)
				{
					data_count--;
					reg_count = tag.nreg;
				}
				break;
			}
			case Format::Image:
			{
				emulator->gs->write_hwreg(qword);
				emulator->gs->write_hwreg(qword >> 64);
				data_count--;
				break;
			}
			default:
				fmt::print("[GIF] Unknown format {:d}\n", format);
				std::abort();
			}

			fifo.pop<uint128_t>();
		}
	}

	void GIF::process_packed(uint128_t qword)
	{
		int curr_reg = tag.nreg - reg_count;
		uint64_t regs = tag.regs;
		uint32_t desc = (regs >> 4 * curr_reg) & 0xf;

		/* Process the qword based on the descriptor */
		auto& gs = emulator->gs;
		switch (desc)
		{
		case REGDesc::PRIM:
		{
			gs->write(0x0, qword & 0x7ff);
			break;
		}
		case REGDesc::RGBAQ:
		{
			RGBAQReg target;
			target.r = qword & 0xff;
			target.g = (qword >> 32) & 0xff;
			target.b = (qword >> 64) & 0xff;
			target.a = (qword >> 96) & 0xff;

			gs->write(0x1, target.value);
			break;
		}
		case REGDesc::ST:
		{
			gs->write(0x2, qword);
			interal_Q = qword >> 64;
			break;
		}
		case REGDesc::XYZF2:
		{
			bool disable_draw = (qword >> 111) & 1;
			auto address = disable_draw ? 0xc : 0x4;

			/* Form the target register */
			XYZF target;
			target.x = qword & 0xffff;
			target.y = (qword >> 32) & 0xffff;
			target.z = (qword >> 68) & 0xffffff;
			target.f = (qword >> 100) & 0xff;

			/* Let the GS handle the rest */
			gs->write(address, target.value);
			break;
		}
		case REGDesc::XYZ2:
		{
			bool disable_draw = (qword >> 111) & 1;
			auto address = disable_draw ? 0xd : 0x5;
			
			/* Form the target register */
			XYZ target;
			target.x = qword & 0xffff;
			target.y = (qword >> 32) & 0xffff;
			target.z = (qword >> 64) & 0xffffffff;

			/* Let the GS handle the rest */
			gs->write(address, target.value);
			break;
		}
		case REGDesc::A_D:
		{
			uint64_t data = qword;
			uint16_t addr = (qword >> 64) & 0xff;
			emulator->gs->write(addr, data);
			break;
		}
		default:
			fmt::print("[GIF] Unknown reg descritptor {:#x}!\n", desc);
			std::abort();
		}

		reg_count--;
	}
}