#include <gs/gs.h>
#include <common/emulator.h>
#include <cassert>

static const char* PRIV_REGS[] =
{
	"PMODE", "SMODE1", "SMODE2",
	"SRFSH", "SYNCH1", "SYNCH2",
	"SYNCV", "DISPFB1", "DISPLAY1",
	"DISPFB2", "DISPLAY2", "EXTBUF",
	"EXTDATA", "EXTWRITE", "BGCOLOR",
	"GS_CSR", "GS_IMR", "BUSDIR",
	"SIGLBLID"
};

namespace gs
{
	GraphicsSynthesizer::GraphicsSynthesizer(common::Emulator* parent) :
		emulator(parent)
	{
		uint32_t addresses[3] = { 0x12000000, 0x12000080, 0x12001000 };
		auto reader = &GraphicsSynthesizer::read_priv;
		auto writer = &GraphicsSynthesizer::write_priv;

		for (auto addr : addresses)
			emulator->add_handler<uint64_t>(addr, this, reader, writer);

		/* Allocate VRAM */
		vram = new Page[512];
	}

	GraphicsSynthesizer::~GraphicsSynthesizer()
	{
		delete[] vram;
	}

	uint64_t GraphicsSynthesizer::read_priv(uint32_t addr)
	{
		bool group = addr & 0xf000;
		uint32_t offset = ((addr >> 4) & 0xf) + 15 * group;
		auto ptr = (uint64_t*)&priv_regs + offset;

		fmt::print("[GS] Reading {:#x} from {}\n", *ptr, PRIV_REGS[offset]);
		/* Only CSR and SIGLBLID are readable! */
		assert(offset == 15 || offset == 18);

		return *ptr;
	}

	void GraphicsSynthesizer::write_priv(uint32_t addr, uint64_t data)
	{
		bool group = addr & 0xf000;
		uint32_t offset = ((addr >> 4) & 0xf) + 15 * group;
		auto ptr = (uint64_t*)&priv_regs + offset;

		fmt::print("[GS] Writing {:#x} to {}\n", data, PRIV_REGS[offset]);

		*ptr = data;
	}

	void GraphicsSynthesizer::write(uint16_t addr, uint64_t data)
	{
		switch (addr)
		{
		case 0x1:
			regs.rgbaq.value = data;
			break;
		case 0x2:
			regs.st = data;
			break;
		case 0x3:
			regs.uv = data;
			break;
		case 0x6:
		case 0x7:
			regs.tex0[addr & 1] = data;
			break;
		case 0x8:
		case 0x9:
			regs.clamp[addr & 1] = data;
			break;
		case 0xa:
			regs.fog = data;
			break;
		case 0x14:
		case 0x15:
			regs.tex1[addr & 1] = data;
			break;
		case 0x16:
		case 0x17:
			regs.tex2[addr & 1] = data;
			break;
		case 0x18:
		case 0x19:
			regs.xyoffset[addr & 1] = data;
			break;
		case 0x1a:
			regs.prmodecont = data;
			break;
		case 0x1b:
			regs.prmode = data;
			break;
		case 0x1c:
			regs.texclut = data;
			break;
		case 0x22:
			regs.scanmsk = data;
			break;
		case 0x34:
		case 0x35:
			regs.miptbp1[addr & 1] = data;
			break;
		case 0x36:
		case 0x37:
			regs.miptbp2[addr & 1] = data;
			break;
		case 0x3b:
			regs.texa = data;
			break;
		case 0x3d:
			regs.fogcol = data;
			break;
		case 0x3f:
			regs.texflush = data;
			break;
		case 0x40:
		case 0x41:
			regs.scissor[addr & 1] = data;
			break;
		case 0x42:
		case 0x43:
			regs.alpha[addr & 1] = data;
			break;
		case 0x44:
			regs.dimx = data;
			break;
		case 0x45:
			regs.dthe = data;
			break;
		case 0x46:
			regs.colclamp = data;
			break;
		case 0x47:
		case 0x48:
			regs.test[addr & 1] = data;
			break;
		case 0x49:
			regs.pabe = data;
			break;
		case 0x4a:
		case 0x4b:
			regs.fba[addr & 1] = data;
			break;
		case 0x4c:
		case 0x4d:
			regs.frame[addr & 1] = data;
			break;
		case 0x4e:
		case 0x4f:
			regs.zbuf[addr & 1] = data;
			break;
		case 0x50:
			regs.bitbltbuf.value = data;
			break;
		case 0x51:
			regs.trxpos.value = data;
			break;
		case 0x52:
			regs.trxreg.value = data;
			break;
		case 0x53:
			regs.trxdir = data;
			break;
		default:
			fmt::print("[GS] Writting {:#x} to unknown address {:#x}\n", data, addr);
			std::abort();
		}
	}

	void GraphicsSynthesizer::write_hwreg(uint64_t data)
	{
		/* HWREG is only used for GIF -> VRAM transfers */
		if (regs.trxdir != TRXDir::HostLocal)
		{
			fmt::print("[GS] Write to HWREG with invalid transfer dir!\n");
			std::abort();
			return;
		}

		auto& bitbltbuf = regs.bitbltbuf;
		auto& trxpos = regs.trxpos;
		auto& trxreg = regs.trxreg;

		uint16_t format = regs.bitbltbuf.dest_pixel_format;
		switch (format)
		{
		/* Page layout of PSMCT32
		  <------- 8 blocks/64 pixels ------->
		| 0| | 1| | 4| | 5| |16| |17| |20| |21| ^
		| 2| | 3| | 6| | 7| |18| |19| |22| |23| | 4 blocks/32 pixels
		| 8| | 9| |12| |13| |24| |25| |28| |29| |
		|10| |11| |14| |15| |26| |27| |30| |31| | */
		case PixelFormat::PSMCT32:
		{
			/* Calculate which page we are refering to */
			uint32_t page = bitbltbuf.dest_base / BLOCKS_PER_PAGE;
			uint32_t width_in_pages = bitbltbuf.dest_width;
			uint32_t width_in_pixels = trxreg.width;
			
			/* HWREG values are 64bit so they pack 2 pixels together */
			for (int i = 0; i < 2; i++)
			{
				uint16_t x = data_written % width_in_pixels;
				uint16_t y = data_written / width_in_pixels;

				fmt::print("[GS] Writing to PSMCT32 buffer at ({}, {})\n", x, y);

				/* NOTE: x, y can refer to outside of the selected page (if their values are bigger than the page dimentions) */
				/* Update the page accordingly. Loop back if the page is over the page width */
				page += (x / Page::PIXEL_WIDTH) % width_in_pages + (y / Page::PIXEL_HEIGHT) * width_in_pages;
				uint32_t* pixels = (uint32_t*)&data;
			
				vram[page].write_psmct32(x, y, pixels[i]);
				data_written++;
			}
			
			break;
		}
		default:
			fmt::print("[GS] Unknown texture format {:#x}\n", format);
			std::abort();
		}

		if (data_written >= trxreg.width * trxreg.height)
		{
			fmt::print("[GS] HWREG transfer complete!\n");
			data_written = 0;

			/* Deactivate TRXDIR */
			regs.trxdir = TRXDir::None;
		}
	}
}