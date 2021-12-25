#pragma once
#include <cstdint>

namespace ee
{
    /* All COP0 instructions have this */
    constexpr uint8_t COP0_OPCODE = 0b010000;

    /* Instruction types */
    constexpr uint8_t COP0_MF0 = 0b00000;
    constexpr uint8_t COP0_MT0 = 0b00100;
    constexpr uint8_t COP0_C0 = 0b10000;
    constexpr uint8_t COP0_TLB = 0b10000;

    /* The status register fields */
    union COP0Status
    {
        uint32_t value = 0x400004; /* BEV, ERL = 1 by default */
        struct
        {
            uint32_t ie : 1; /* Interrupt Enable */
            uint32_t exl : 1; /* Exception Level */
            uint32_t erl : 1; /* Error Level */
            uint32_t ksu : 2; /* Kernel/Supervisor/User Mode bits */
            uint32_t : 5;
            uint32_t im0 : 1; /* Int[1:0] signals */
            uint32_t im1 : 1;
            uint32_t bem : 1; /* Bus Error Mask */
            uint32_t : 2;
            uint32_t im7 : 1; /* Internal timer interrupt  */
            uint32_t eie : 1; /* Enable IE */
            uint32_t edi : 1; /* EI/DI instruction Enable */
            uint32_t ch : 1; /* Cache Hit */
            uint32_t : 3;
            uint32_t bev : 1; /* Location of TLB refill */
            uint32_t dev : 1; /* Location of Performance counter */
            uint32_t : 2;
            uint32_t fr : 1; /* Additional floating point registers */
            uint32_t : 1;
            uint32_t cu : 4; /* Usability of each of the four coprocessors */
        };
    };

    union COP0Cause
    {
        uint32_t value = 0;
        struct
        {
            uint32_t : 2;
            uint32_t exccode : 5;
            uint32_t : 3;
            uint32_t ip1_pending : 1;
            uint32_t ip0_pending : 1;
            uint32_t siop : 1;
            uint32_t : 2;
            uint32_t timer_ip_pending : 1;
            uint32_t exc2 : 3;
            uint32_t : 9;
            uint32_t ce : 2;
            uint32_t bd2 : 1;
            uint32_t bd : 1;
        };
    };

    enum OperatingMode 
    {
        USER_MODE = 0b10,
        SUPERVISOR_MODE = 0b01,
        KERNEL_MODE = 0b00
    };

    /* The COP0 registers */
    union COP0
    {
        uint32_t regs[32] = {};
        struct
        {
            uint32_t index;
            uint32_t random;
            uint32_t entry_lo0;
            uint32_t entry_lo1;
            uint32_t context;
            uint32_t page_mask;
            uint32_t wired;
            uint32_t reserved0[1];
            uint32_t bad_vaddr;
            uint32_t count;
            uint32_t entryhi;
            uint32_t compare;
            COP0Status status;
            COP0Cause cause;
            uint32_t epc;
            uint32_t prid;
            uint32_t config;
            uint32_t reserved1[6];
            uint32_t bad_paddr;
            uint32_t debug;
            uint32_t perf;
            uint32_t reserved2[2];
            uint32_t tag_lo;
            uint32_t tag_hi;
            uint32_t error_epc;
            uint32_t reserved3[1];
        };

        OperatingMode get_operating_mode()
        {
            if (status.exl || status.erl) /* Setting one of these enforces kernel mode */
                return OperatingMode::KERNEL_MODE;
            else
                return (OperatingMode)status.ksu;
        }
    };
};