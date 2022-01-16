#include <media/gamepad.h>
#include <cstring>
#include <cstdio>
#include <fmt/core.h>

namespace media
{
    constexpr const char* MODES[] = { "digital", "analog" };

    /* Response buffers based on the command. Can be modified by commands
   on demand. */
    uint8_t Gamepad::responses[16][18] =
    {
        {}, {}, {}, {}, {}, { 0x03, 0x02, 0x00, 0x02, 0x01, 0x00 }, {},
        { 0x00, 0x00, 0x02, 0x00, 0x01, 0x00 }
    };

    Gamepad::Gamepad()
    {

    }

    void Gamepad::press_button(PadButton button)
    {
        buttons &= ~(1 << (uint32_t)button);
    }

    void Gamepad::release_button(PadButton button)
    {
        buttons |= 1 << (uint32_t)button;
    }

    void Gamepad::switch_mode(uint8_t m)
    {
        fmt::print("[PAD] Switching to {} mode\n", MODES[m]);
        mode = (PadMode)m;
    }

    void Gamepad::read_buttons(uint8_t cmd)
    {
        fmt::print("[PAD] Reading button state: {:#b}\n", buttons);
        uint8_t offset = written - 4;
        responses[command & 0xf][offset] = (buttons >> offset) & 0xff;
        
        /* Chain response */
        set_response(4, &Gamepad::read_buttons);
    }

    void Gamepad::set_config(uint8_t value)
    {
        fmt::print("[PAD] Setting config mode to {:#x}\n", value);

        /* When not in config mode, 0x43 behaves like 0x42 */
        if (!config_mode)
            read_buttons(value);
        else /* Otherwise return zero */
            std::memset(responses[command & 0xf], 0, 2);

        config_mode = value;
    }

    void Gamepad::query(uint8_t half)
    {
        static uint8_t constants[2][6] = { { 0x0, 0x0, 0x0, 0x2, 0x0, 0xa }, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x14 } };
        fmt::print("[PAD] Query act {:d}\n", half);
        std::memcpy(responses[command & 0xf], constants[half], 6);
    }

    inline void Gamepad::set_response(uint16_t byte_id, Response resp)
    {
        custom_response = byte_id + 1;
        response = resp;
    }

    uint8_t Gamepad::process_command(uint8_t cmd)
    {
        if (!config_mode &&
            cmd != PadCommand::CONFIG_MODE &&
            cmd != PadCommand::READ_DATA)
        {
            fmt::print("[PAD] This command needs to be called in config mode!\n");
            std::abort(); /* For debugging */
            return 0xf3;
        }

        command = cmd;
        uint8_t reply = 0xf3;
        switch (cmd)
        {
        case PadCommand::READ_DATA:
        {
            set_response(3, &Gamepad::read_buttons);
            reply = (mode == PadMode::Digital ? 0x41 : 0x73);
            break;
        }
        case PadCommand::CONFIG_MODE:
        {
            set_response(3, &Gamepad::set_config);
            reply = (mode == PadMode::Digital ? 0x41 : 0x73);
            break;
        }
        case PadCommand::SET_MAIN_MODE:
        {
            set_response(3, &Gamepad::switch_mode);
            break;
        }
        case PadCommand::QUERY_MODEL:
        {
            responses[5][2] = (uint8_t)mode;
            break;
        }
        case PadCommand::QUERY_ACT:
        {
            set_response(3, &Gamepad::query);
            break;
        }
        case PadCommand::QUERY_COMB:
        {
            fmt::print("[PAD] Query comb\n");
            break;
        }
        default:
            fmt::print("[PAD] Unknown command {:#x}\n", cmd);
            std::abort();
        }

        /* All commands return this except 0x41 - 0x43 */
        return reply;
    }

    uint8_t Gamepad::write_byte(uint8_t byte)
    {
        written++;
        if (written == custom_response)
        {
            /* Some commands require custom responses for certain bytes in the command chain that
               depend on the input provided. The response function can also modify the response
               byte to chain custom reponses. */
            custom_response = -1;
            (this->*response)(byte);
        }
        else
        {
            /* This is for the command header */
            switch (written)
            {
            case 1: return 0xff;
            case 2: return process_command(byte);
            case 3: return 0x5a;
            }
        }

        uint8_t offset = written - 4;
        return responses[command & 0xf][offset];
    }
}