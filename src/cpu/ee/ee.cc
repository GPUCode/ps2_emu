#include <cpu/ee/ee.h>
#include <cpu/vu/vu.h>
#include <common/emulator.h>
#include <cpu/ee/opcode.h>
#include <cpu/ee/jit/jit.h>
#include <fmt/color.h>
#include <unordered_map>

namespace ee
{
    EmotionEngine::EmotionEngine(common::Emulator* parent) :
        emulator(parent), intc(this), timers(parent, &intc)
    {
        /* Allocate the 32MB of EE memory */
        ram = new uint8_t[32 * 1024 * 1024];
        compiler = new jit::JITCompiler(this);

        /* Reset CPU state. */
        reset();
    }

    EmotionEngine::~EmotionEngine()
    {
        delete[] ram;
        delete compiler;
    }

    void EmotionEngine::reset()
    {
        /* Reset PC. */
        pc = 0xbfc00000;

        /* Build the JIT prologue block */
        compiler->reset();

        /* Reset instruction holders */
        next_instr = {};
        next_instr.value = read<uint32_t>(pc);
        next_instr.pc = pc;

        /* Set this to zero */
        gpr[0].qword = 0;

        fmt::print("{}\n", sizeof(Instruction));

        /* Set EE pRId */
        cop0.prid = 0x2E20;
    }

    void EmotionEngine::tick(uint32_t cycles)
    {
        cycles_to_execute = cycles;

        compiler->run();

        /* Tick the cpu for the amount of cycles requested */
        /*for (int cycle = cycles; cycle > 0; cycle--)
        {
            instr = next_instr;
            skip_branch_delay = false;

            direct_jump();

            switch (instr.opcode)
            {
            case COP0_OPCODE: op_cop0(this); break;
            case SPECIAL_OPCODE: op_special(this); break;
            case COP1_OPCODE: op_cop1(this); break;
            case 0b010010: op_cop2(this); break;
            case 0b101011: op_sw(this); break;
            case 0b001010: op_slti(this); break;
            case 0b000101: op_bne(this); break;
            case 0b001101: op_ori(this); break;
            case 0b001000: op_addi(this); break;
            case 0b011110: op_lq(this); break;
            case 0b001111: op_lui(this); break;
            case 0b001001: op_addiu(this); break;
            case 0b011100: op_mmi(this); break;
            case 0b111111: op_sd(this); break;
            case 0b000011: op_jal(this); break;
            case 0b000001: op_regimm(this); break;
            case 0b001100: op_andi(this); break;
            case 0b000100: op_beq(this); break;
            case 0b010100: op_beql(this); break;
            case 0b001011: op_sltiu(this); break;
            case 0b010101: op_bnel(this); break;
            case 0b100000: op_lb(this); break;
            case 0b111001: op_swc1(this); break;
            case 0b100100: op_lbu(this); break;
            case 0b110111: op_ld(this); break;
            case 0b000010: op_j(this); break;
            case 0b100011: op_lw(this); break;
            case 0b101000: op_sb(this); break;
            case 0b000110: op_blez(this); break;
            case 0b000111: op_bgtz(this); break;
            case 0b100101: op_lhu(this); break;
            case 0b101001: op_sh(this); break;
            case 0b001110: op_xori(this); break;
            case 0b011001: op_daddiu(this); break;
            case 0b011111: op_sq(this); break;
            case 0b100001: op_lh(this); break;
            case 0b101111: op_cache(this); break;
            case 0b100111: op_lwu(this); break;
            case 0b011010: op_ldl(this); break;
            case 0b011011: op_ldr(this); break;
            case 0b101100: op_sdl(this); break;
            case 0b101101: op_sdr(this); break;
            case 0b110001: op_lwc1(this); break;
            case 0b010110: op_blezl(this); break;
            case 0b100010: op_lwl(this); break;
            case 0b100110: op_lwr(this); break;
            case 0b101010: op_swl(this); break;
            case 0b101110: op_swr(this); break;
            case 0b111110: op_sqc2(this); break;
            default:
                common::Emulator::terminate("[ERROR] Unimplemented opcode: {:#06b}\n", instr.opcode & 0x3F);
            }

            if (gpr[3].word[0] == 0x746a0)
            {
                fmt::print("");
            }

            gpr[0].qword = 0;
            cycles_to_execute--;
        }*/

        /* Increment COP0 counter */
        cop0.count += cycles + std::abs(cycles_to_execute);

        /* Tick the timers for BUSCLK cycles */
        timers.tick(cycles / 2);

        /* Check for interrupts */
        if (intc.int_pending())
        {
            fmt::print("[EE] Interrupt!\n");
            exception(Exception::Interrupt);
        }
    }

    void EmotionEngine::exception(Exception exception, bool log)
    {
        if (log) [[likely]]
        {
            fmt::print("[INFO] Exception occured of type {:d}!\n", (uint32_t)exception);
        }

        ExceptionVector vector = ExceptionVector::V_COMMON;
        cop0.cause.exccode = (uint32_t)exception;
        if (!cop0.status.exl)
        {
            cop0.epc = instr.pc - 4 * instr.is_delay_slot;
            cop0.cause.bd = instr.is_delay_slot;

            /* Select appropriate exception vector */
            switch (exception)
            {
            case Exception::TLBLoad:
            case Exception::TLBStore:
                vector = ExceptionVector::V_TLB_REFILL;
                break;
            case Exception::Interrupt:
                vector = ExceptionVector::V_INTERRUPT;
                break;
            default:
                vector = ExceptionVector::V_COMMON;
                break;
            }

            /* Enter kernel mode */
            cop0.status.exl = true;
        }

        /* We do this to avoid branches */
        pc = exception_addr[cop0.status.bev] + vector;

        /* Insert the instruction in our pipeline */
        direct_jump();
    }

    void EmotionEngine::direct_jump()
    {
        next_instr = {};
        next_instr.value = read<uint32_t>(pc);
        next_instr.pc = pc;
        pc += 4;
    }
};
