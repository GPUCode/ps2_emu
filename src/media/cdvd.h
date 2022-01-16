#pragma once
#include <common/component.h>
#include <queue>

namespace common
{
	class Emulator;
}

namespace media
{
	/* Structure for managing N/S commands */
	struct CommandDesc
	{
		uint8_t command;
		uint8_t status;
		std::queue<uint8_t> params;
		std::queue<uint8_t> result; /* Some commands have multiple outputs */
	};

	struct CDVD : public common::Component
	{
		CDVD(common::Emulator* parent);
		~CDVD() = default;

		uint8_t read(uint32_t address);
		void write(uint32_t address, uint8_t data);

	private:
		void process_S_command(uint8_t cmd);
		uint8_t read_S_result();

	private:
		common::Emulator* emulator = nullptr;
		CommandDesc S{}, N{};
	};
}