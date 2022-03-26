#include <cpu/ee/opcode.h>
#include <fmt/color.h>
#include <common/emulator.h>
#include <fstream>

/* Open disassembly file */
std::ofstream disassembly = std::ofstream("disassembly_ee.log");

#ifdef NDEBUG
#define log(...) (void)0
#else
constexpr fmt::v8::text_style BOLD = fg(fmt::color::forest_green) | fmt::emphasis::bold;
#define log(...) \
    if (ee->print_pc) \
    { \
        auto message = fmt::format(__VA_ARGS__); \
        auto output = fmt::format("PC: {:#x} {}", ee->instr.pc, message); \
        disassembly << output; \
        disassembly.flush(); \
    }

#endif

constexpr const char* TEST_ELF = "3stars.elf";
bool load_elf = true;

namespace ee
{
    void op_cop0(EmotionEngine* ee)
    {
        uint16_t type = ee->instr.value >> 21 & 0x1F;

        switch (type)
        {
        case COP0_MF0:
        {
            switch (ee->instr.value & 0x7)
            {
            case 0b000: op_mfc0(ee); break;
            default:
                common::Emulator::terminate("[ERROR] Unimplemented COP0 MF0 instruction: {:#03b}\n", ee->instr.value & 0x7);
            }
            break;
        }
        case COP0_MT0:
        {
            switch (ee->instr.value & 0x7)
            {
            case 0b000: op_mtc0(ee); break;
            default:
                common::Emulator::terminate("[ERROR] Unimplemented COP0 MT0 instruction: {:#03b}\n", ee->instr.value & 0x7);
            }
            break;
        }
        case COP0_TLB:
        {
            auto fmt = ee->instr.value & 0x3f;
            switch (fmt)
            {
            case 0b000010: op_tlbwi(ee); break;
            case 0b111001: op_di(ee); break;
            case 0b011000: op_eret(ee); break;
            case 0b111000: op_ei(ee); break;
            default:
                fmt::print("[ERROR] Unimplemented COP0 TLB instruction {:#08b}\n", fmt);
            }
            break;
        }
        default:
            common::Emulator::terminate("[ERROR] Unimplemented COP0 instruction: {:#05b}\n", type);
        }
    }

    void op_mfc0(EmotionEngine* ee)
    {
        uint16_t rd = (ee->instr.value >> 11) & 0x1F;
        uint16_t rt = (ee->instr.value >> 16) & 0x1F;

        ee->gpr[rt].dword[0] = ee->cop0.regs[rd];
        log("MFC0: ee->gpr[{:d}] = COP0_REG[{:d}] ({:#x})\n", rt, rd, ee->cop0.regs[rd]);
    }

    void op_special(EmotionEngine* ee)
    {
        switch(ee->instr.r_type.funct)
        {
        case 0b000000: op_sll(ee); break;
        case 0b001000: op_jr(ee); break;
        case 0b001111: op_sync(ee); break;
        case 0b001001: op_jalr(ee); break;
        case 0b000011: op_sra(ee); break;
        case 0b100001: op_addu(ee); break;
        case 0b101101: op_daddu(ee); break;
        case 0b100101: op_or(ee); break;
        case 0b011000: op_mult(ee); break;
        case 0b011011: op_divu(ee); break;
        case 0b010010: op_mflo(ee); break;
        case 0b011010: op_div(ee); break;
        case 0b010000: op_mfhi(ee); break;
        case 0b101011: op_sltu(ee); break;
        case 0b100011: op_subu(ee); break;
        case 0b001011: op_movn(ee); break;
        case 0b101010: op_slt(ee); break;
        case 0b100100: op_and(ee); break;
        case 0b000010: op_srl(ee); break;
        case 0b111100: op_dsll32(ee); break;
        case 0b111111: op_dsra32(ee); break;
        case 0b111000: op_dsll(ee); break;
        case 0b010111: op_dsrav(ee); break;
        case 0b001010: op_movz(ee); break;
        case 0b010100: op_dsllv(ee); break;
        case 0b000100: op_sllv(ee); break;
        case 0b000111: op_srav(ee); break;
        case 0b100111: op_nor(ee); break;
        case 0b111010: op_dsrl(ee); break;
        case 0b000110: op_srlv(ee); break;
        case 0b111110: op_dsrl32(ee); break;
        case 0b001100: op_syscall(ee); break;
        case 0b101000: op_mfsa(ee); break;
        case 0b010001: op_mthi(ee); break;
        case 0b010011: op_mtlo(ee); break;
        case 0b101001: op_mtsa(ee); break;
        case 0b101111: op_dsubu(ee); break;
        case 0b100110: op_xor(ee); break;
        case 0b011001: op_multu(ee); break;
        case 0b111011: op_dsra(ee); break;
        case 0b100010: op_sub(ee); break;
        case 0b100000: op_add(ee); break;
        default:
            common::Emulator::terminate("[ERROR] Unimplemented SPECIAL instruction: {:#06b}\n", (uint16_t)ee->instr.r_type.funct);
        }
    }

    void op_regimm(EmotionEngine* ee)
    {
        uint16_t type = (ee->instr.value >> 16) & 0x1F;
        switch(type)
        {
        case 0b00001: op_bgez(ee); break;
        case 0b00000: op_bltz(ee); break;
        case 0b00010: op_bltzl(ee); break;
        case 0b00011: op_bgezl(ee); break;
        default:
            common::Emulator::terminate("[ERROR] Unimplemented REGIMM instruction: {:#07b}\n", type);
        }
    }

    void op_sw(EmotionEngine* ee)
    {
        uint16_t base = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint32_t data = ee->gpr[rt].word[0];

        log("SW: Writing ee->gpr[{:d}] ({:#x}) to address {:#x} = ee->gpr[{:d}] ({:#x}) + {:d}\n", rt, data, vaddr, base, ee->gpr[base].word[0], offset);
        if (vaddr & 0x3) [[unlikely]]
        {
            log("[ERROR] SW: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorStore);
        }
        else
            ee->write<uint32_t>(vaddr, data);
    }

    void op_sll(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t sa = ee->instr.r_type.sa;

        ee->gpr[rd].dword[0] = (uint64_t)(int32_t)(ee->gpr[rt].word[0] << sa);

        if (ee->instr.value == 0)
        {
            log("NOP\n");
        }
        else
        {
            log("SLL: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) << {:d}\n", rd, rt, ee->gpr[rt].dword[0], sa);
        }
    }

    void op_slti(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        int64_t imm = (int16_t)ee->instr.i_type.immediate;

        int64_t reg = ee->gpr[rs].dword[0];
        ee->gpr[rt].dword[0] = reg < imm;

        log("SLTI: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) < {:#x}\n", rt, rs, reg, imm);
    }

    void op_bne(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;
        int32_t imm = (int16_t)ee->instr.i_type.immediate;

        int32_t offset = imm << 2;
        if (ee->gpr[rs].dword[0] != ee->gpr[rt].dword[0])
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }

        ee->next_instr.is_delay_slot = true;
        log("BNE: IF ee->gpr[{:d}] ({:#x}) != ee->gpr[{:d}] ({:#x}) THEN PC += {:#x}\n", rt, ee->gpr[rt].dword[0], rs, ee->gpr[rs].dword[0], offset);
    }

    void op_ori(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t imm = ee->instr.i_type.immediate;

        log("ORI: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) | {:#x}\n", rt, rs, ee->gpr[rs].dword[0], imm);

        ee->gpr[rt].dword[0] = ee->gpr[rs].dword[0] | (uint64_t)imm;
    }

    void op_addi(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        int16_t imm = (int16_t)ee->instr.i_type.immediate;

        /* TODO: Overflow detection */
        int64_t reg = ee->gpr[rs].dword[0];
        ee->gpr[rt].dword[0] = reg + imm;

        log("ADDI: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, rs, reg, imm);
    }

    void op_lq(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t imm = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = ee->gpr[base].word[0] + imm;
        ee->gpr[rt].qword = ee->read<uint128_t>(vaddr);

        log("LQ: ee->gpr[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x} + {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], imm);
    }

    void op_lui(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint32_t imm = ee->instr.i_type.immediate;

        ee->gpr[rt].dword[0] = (int32_t)(imm << 16);

        log("LUI: ee->gpr[{:d}] = {:#x}\n", rt, ee->gpr[rt].dword[0]);
    }

    void op_jr(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        ee->pc = ee->gpr[rs].word[0];

        ee->next_instr.is_delay_slot = true;
        ee->branch_taken = true;
        log("JR: Jumped to ee->gpr[{:d}] = {:#x}\n", rs, ee->pc);
    }

    void op_sync(EmotionEngine* ee)
    {
        log("SYNC\n");
    }

    void op_lb(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        ee->gpr[rt].dword[0] = (int8_t)ee->read<uint8_t>(vaddr);

        log("LB: ee->gpr[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], offset);
    }

    void op_swc1(EmotionEngine* ee)
    {
        uint16_t base = ee->instr.i_type.rs;
        uint16_t ft = ee->instr.i_type.rt;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint32_t data = ee->cop1.fpr[ft].uint;

        log("SWC1: Writing FPR[{:d}] ({:#x}) to address {:#x} = ee->gpr[{:d}] ({:#x}) + {:d}\n", ft, data, vaddr, base, ee->gpr[base].word[0], offset);
        if ((vaddr & 0b11) != 0) [[unlikely]]
        {
            common::Emulator::terminate("[ERROR] SW: Address {:#x} is not aligned\n", vaddr);
        }
        else
            ee->write<uint32_t>(vaddr, data);
    }

    void op_lbu(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        ee->gpr[rt].dword[0] = ee->read<uint8_t>(vaddr);

        log("LBU: ee->gpr[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], offset);
    }

    void op_ld(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];

        if (vaddr & 0x7) [[unlikely]]
        {
            log("[ERROR] LD: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorLoad);
        }
        else
        {
            ee->gpr[rt].dword[0] = ee->read<uint64_t>(vaddr);
            log("LD: ee->gpr[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], offset);
        }

    }

    void op_j(EmotionEngine* ee)
    {
        uint32_t instr_index = ee->instr.j_type.target;
        ee->pc = ((ee->instr.pc + 4) & 0xF0000000) | (instr_index << 2);

        ee->next_instr.is_delay_slot = true;
        ee->branch_taken = true;
        log("J: Jumping to PC = {:#x}\n", ee->pc);
    }

    void op_sb(EmotionEngine* ee)
    {
        uint16_t base = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint16_t data = ee->gpr[rt].word[0] & 0xFF;

        log("SB: Writing ee->gpr[{:d}] ({:#x}) to address {:#x} = ee->gpr[{:d}] ({:#x}) + {:d}\n", rt, data, vaddr, base, ee->gpr[base].word[0], offset);
        ee->write<uint8_t>(vaddr, data);
    }

    void op_div(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;

        int32_t reg1 = ee->gpr[rs].word[0];
        int32_t reg2 = ee->gpr[rt].word[0];

        if (reg2 == 0)
        {
            ee->hi0 = reg1;
            ee->lo0 = reg1 >= 0 ? (int32_t)0xffffffff : 1;
        }
        else if (reg1 == 0x80000000 && reg2 == -1)
        {
            ee->hi0 = 0;
            ee->lo0 = (int32_t)0x80000000;
        }
        else
        {
            ee->hi0 = (int32_t)(reg1 % reg2);
            ee->lo0 = (int32_t)(reg1 / reg2);
        }

        log("DIV: ee->lo0 = ee->gpr[{:d}] ({:#x}) / ee->gpr[{:d}] ({:#x})\n", rs, reg1, rt, reg2);
    }

    void op_mfhi(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;

        ee->gpr[rd].dword[0] = ee->hi0;

        log("MFHI: ee->gpr[{:d}] = ee->hi0 ({:#x})\n", rd, ee->hi0);
    }

    void op_sltu(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rt = ee->instr.r_type.rt;

        log("SLTU: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) < ee->gpr[{:d}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);
        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0] < ee->gpr[rt].dword[0];
    }

    void op_blez(EmotionEngine* ee)
    {
        int32_t imm = (int16_t)ee->instr.i_type.immediate;
        uint16_t rs = ee->instr.i_type.rs;

        int32_t offset = imm << 2;
        int64_t reg = ee->gpr[rs].dword[0];
        if (reg <= 0)
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }

        ee->next_instr.is_delay_slot = true;
        log("BLEZ: IF ee->gpr[{:d}] ({:#x}) <= 0 THEN PC += {:#x}\n", rs, ee->gpr[rs].dword[0], offset);
    }

    void op_subu(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rt = ee->instr.r_type.rt;

        int32_t reg1 = ee->gpr[rs].dword[0];
        int32_t reg2 = ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[0] = reg1 - reg2;

        log("SUBU: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) - ee->gpr[{:d}] ({:#x})\n", rd, rs, reg1, rt, reg2);
    }

    void op_bgtz(EmotionEngine* ee)
    {
        int32_t imm = (int16_t)ee->instr.i_type.immediate;
        uint16_t rs = ee->instr.i_type.rs;

        int32_t offset = imm << 2;
        int64_t reg = ee->gpr[rs].dword[0];
        if (reg > 0)
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }

        ee->next_instr.is_delay_slot = true;
        log("BGTZ: IF ee->gpr[{:d}] ({:#x}) > 0 THEN PC += {:#x}\n", rs, ee->gpr[rs].dword[0], offset);
    }

    void op_movn(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rt = ee->instr.r_type.rt;

        if (ee->gpr[rt].dword[0] != 0) ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0];

        log("MOVN: IF ee->gpr[{:d}] ({:#x}) != 0 THEN ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x})\n", rt, ee->gpr[rt].dword[0], rd, rs, ee->gpr[rs].dword[0]);
    }

    void op_slt(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rt = ee->instr.r_type.rt;

        int64_t reg1 = ee->gpr[rs].dword[0];
        int64_t reg2 = ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[0] = reg1 < reg2;

        log("SLT: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) < ee->gpr[{:d}] ({:#x})\n", rd, rs, reg1, rt, reg2);
    }

    void op_and(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0] & ee->gpr[rt].dword[0];

        log("AND: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) & ee->gpr[{:d}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);
    }

    void op_srl(EmotionEngine* ee)
    {
        uint16_t sa = ee->instr.r_type.sa;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = (int32_t)(ee->gpr[rt].word[0] >> sa);

        log("SRL: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> {:d}\n", rd, rt, ee->gpr[rt].word[0], sa);
    }

    void op_dsll(EmotionEngine* ee)
    {
        uint16_t sa = ee->instr.r_type.sa;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        log("DSLL: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) << {:d} + 32\n", rd, rt, ee->gpr[rt].dword[0], sa);
        ee->gpr[rd].dword[0] = ee->gpr[rt].dword[0] << sa;
    }

    void op_bltz(EmotionEngine* ee)
    {
        int32_t imm = (int16_t)ee->instr.i_type.immediate;
        uint16_t rs = ee->instr.i_type.rs;

        int32_t offset = imm << 2;
        int64_t reg = ee->gpr[rs].dword[0];
        if (reg < 0)
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }

        ee->next_instr.is_delay_slot = true;
        log("BLTZ: IF ee->gpr[{:d}] ({:#x}) > 0 THEN PC += {:#x}\n", rs, ee->gpr[rs].dword[0], offset);
    }

    void op_sh(EmotionEngine* ee)
    {
        uint16_t base = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint16_t data = ee->gpr[rt].word[0] & 0xFFFF;

        log("SH: Writing ee->gpr[{:d}] ({:#x}) to address {:#x} = ee->gpr[{:d}] ({:#x}) + {:d}\n", rt, data, vaddr, base, ee->gpr[base].word[0], offset);
        if (vaddr & 0x1) [[unlikely]]
        {
            log("[ERROR] SH: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorStore);
        }
        else
            ee->write<uint16_t>(vaddr, data);
    }

    void op_madd(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rt = ee->instr.r_type.rt;

        uint64_t lo = ee->lo0 & 0xFFFFFFFF;
        uint64_t hi = ee->hi0 & 0xFFFFFFFF;
        int64_t result = (hi << 32 | lo) + (int64_t)ee->gpr[rs].word[0] * (int64_t)ee->gpr[rt].word[0];

        ee->lo0 = (int64_t)(int32_t)(result & 0xFFFFFFFF);
        ee->hi0 = (int64_t)(int32_t)(result >> 32);
        ee->gpr[rd].dword[0] = (int64_t)ee->lo0;

        log("MADD: ee->gpr[{:d}] = ee->lo0 = {:#x} and ee->hi0 = {:#x}\n", rd, ee->lo0, ee->hi0);
    }

    void op_divu1(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;

        if (ee->gpr[rt].word[0] != 0)
        {
            ee->lo1 = (int64_t)(int32_t)(ee->gpr[rs].word[0] / ee->gpr[rt].word[0]);
            ee->hi1 = (int64_t)(int32_t)(ee->gpr[rs].word[0] % ee->gpr[rt].word[0]);

            log("DIVU1: ee->gpr[{:d}] ({:#x}) / ee->gpr[{:d}] ({:#x}) OUTPUT ee->lo1 = {:#x} and ee->hi1 = {:#x}\n", rs, ee->gpr[rs].word[0], rt, ee->gpr[rt].word[0], ee->lo1, ee->hi1);
        }
        else [[unlikely]]
        {
            log("[ERROR] DIVU1: Division by zero!\n");
        }
    }

    void op_mflo1(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;

        ee->gpr[rd].dword[0] = ee->lo1;

        log("mflo1: ee->gpr[{:d}] = {:#x}\n", rd, ee->lo1);
    }

    void op_xori(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        uint64_t imm = ee->instr.i_type.immediate;

        log("XORI: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) ^ {:d}\n", rt, rs, ee->gpr[rs].dword[0], imm);

        ee->gpr[rt].dword[0] = ee->gpr[rs].dword[0] ^ imm;
    }

    void op_mult1(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        int64_t reg1 = (int64_t)ee->gpr[rs].dword[0];
        int64_t reg2 = (int64_t)ee->gpr[rt].dword[0];
        int64_t result = reg1 * reg2;
        ee->gpr[rd].dword[0] = ee->lo1 = (int32_t)(result & 0xFFFFFFFF);
        ee->hi1 = (int32_t)(result >> 32);

        log("MULT1: ee->gpr[{:d}] ({:#x}) * ee->gpr[{:d}] ({:#x}) = {:#x} OUTPUT ee->gpr[{:d}] = ee->lo0 = {:#x} and ee->hi0 = {:#x}\n", rs, reg1, rt, reg2, result, rd, ee->lo1, ee->hi1);
    }

    void op_movz(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        if (ee->gpr[rt].dword[0] == 0)
            ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0];

        log("MOVZ: IF ee->gpr[{:d}] == 0 THEN ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x})\n", rt, rd, rs, ee->gpr[rs].dword[0]);
    }

    void op_dsllv(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        uint64_t reg = ee->gpr[rt].dword[0];
        uint16_t sa = ee->gpr[rs].word[0] & 0x3F;
        ee->gpr[rd].dword[0] = reg << sa;

        log("DSLLV: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) << ee->gpr[{:d}] ({:d})\n", rd, rt, reg, rs, sa);
    }

    void op_daddiu(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        log("DADDIU: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, rs, ee->gpr[rs].dword[0], offset);

        int64_t reg = ee->gpr[rs].dword[0];
        ee->gpr[rt].dword[0] = reg + offset;
    }

    void op_sq(EmotionEngine* ee)
    {
        uint16_t base = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];

        log("SQ: Writing ee->gpr[{:d}] ({:#x}) to address {:#x} = ee->gpr[{:d}] ({:#x}) + {:d}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], offset);
        if ((vaddr & 0xF) != 0)
        {
            common::Emulator::terminate("[ERROR] SQ: Address {:#x} is not aligned\n", vaddr);
        }
        else
        {
            ee->write<uint128_t>(vaddr, ee->gpr[rt].qword);
        }
    }

    void op_lh(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];

        log("LH: ee->gpr[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], offset);
        if (vaddr & 0x1) [[unlikely]]
        {
            log("[ERROR] LH: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorLoad);
        }
        else
            ee->gpr[rt].dword[0] = (int16_t)ee->read<uint16_t>(vaddr);
    }

    void op_cache(EmotionEngine* ee)
    {
        log("\n");
    }

    void op_sllv(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        uint32_t reg = ee->gpr[rt].word[0];
        uint16_t sa = ee->gpr[rs].word[0] & 0x3F;
        ee->gpr[rd].dword[0] = (int32_t)(reg << sa);

        log("SLLV: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) << ee->gpr[{:d}] ({:d})\n", rd, rt, reg, rs, sa);
    }

    void op_srav(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        int32_t reg = (int32_t)ee->gpr[rt].word[0];
        uint16_t sa = ee->gpr[rs].word[0] & 0x3F;
        ee->gpr[rd].dword[0] = reg >> sa;

        log("SRAV: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> ee->gpr[{:d}] ({:d})\n", rd, rt, reg, rs, sa);
    }

    void op_nor(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ~(ee->gpr[rs].dword[0] | ee->gpr[rt].dword[0]);

        log("NOR: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) NOR ee->gpr[{:d}] ({:d})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);
    }

    void op_lwu(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];

        log("LWU: ee->gpr[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], offset);
        if (vaddr & 0x3) [[unlikely]]
        {
            log("[ERROR] LWU: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorLoad);
        }
        else
            ee->gpr[rt].dword[0] = ee->read<uint32_t>(vaddr);
    }

    void op_ldl(EmotionEngine* ee)
    {
        static const uint64_t LDL_MASK[8] =
        {
            0x00ffffffffffffffULL, 0x0000ffffffffffffULL, 0x000000ffffffffffULL, 0x00000000ffffffffULL,
            0x0000000000ffffffULL, 0x000000000000ffffULL, 0x00000000000000ffULL, 0x0000000000000000ULL
        };
        static const uint8_t LDL_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        /* The address given is unaligned, so let's align it first */
        uint32_t addr = offset + ee->gpr[base].word[0];
        uint32_t aligned_addr = addr & ~0x7;
        uint32_t shift = addr & 0x7;

        auto dword = ee->read<uint64_t>(aligned_addr);
        uint64_t result = (ee->gpr[rt].dword[0] & LDL_MASK[shift]) | (dword << LDL_SHIFT[shift]);
        ee->gpr[rt].dword[0] = result;

        log("LDL: ee->gpr[{}] = {:#x} with data from {:#x}\n", rt, ee->gpr[rt].dword[0], addr);
    }

    void op_ldr(EmotionEngine* ee)
    {
        static const uint64_t LDR_MASK[8] =
        {
            0x0000000000000000ULL, 0xff00000000000000ULL, 0xffff000000000000ULL, 0xffffff0000000000ULL,
            0xffffffff00000000ULL, 0xffffffffff000000ULL, 0xffffffffffff0000ULL, 0xffffffffffffff00ULL
        };
        static const uint8_t LDR_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        /* The address given is unaligned, so let's align it first */
        uint32_t addr = offset + ee->gpr[base].word[0];
        uint32_t aligned_addr = addr & ~0x7;
        uint16_t shift = addr & 0x7;

        auto dword = ee->read<uint64_t>(aligned_addr);
        uint64_t result = (ee->gpr[rt].dword[0] & LDR_MASK[shift]) | (dword >> LDR_SHIFT[shift]);
        ee->gpr[rt].dword[0] = result;

        log("LDR: ee->gpr[{}] = {:#x} with data from {:#x}\n", rt, ee->gpr[rt].dword[0], addr);
    }

    void op_sdl(EmotionEngine* ee)
    {
        static const uint64_t SDL_MASK[8] =
        {
            0xffffffffffffff00ULL, 0xffffffffffff0000ULL, 0xffffffffff000000ULL, 0xffffffff00000000ULL,
            0xffffff0000000000ULL, 0xffff000000000000ULL, 0xff00000000000000ULL, 0x0000000000000000ULL
        };
        static const uint8_t SDL_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        /* The address given is unaligned, so let's align it first */
        uint32_t addr = offset + ee->gpr[base].word[0];
        uint32_t aligned_addr = addr & ~0x7;
        uint32_t shift = addr & 0x7;

        auto dword = ee->read<uint64_t>(aligned_addr);
        dword = (ee->gpr[rt].dword[0] >> SDL_SHIFT[shift]) | (dword & SDL_MASK[shift]);
        ee->write<uint64_t>(aligned_addr, dword);

        log("SDL: Writing {:#x} to address {:#x}\n", dword, aligned_addr);
    }

    void op_sdr(EmotionEngine* ee)
    {
        static const uint64_t SDR_MASK[8] =
        {
            0x0000000000000000ULL, 0x00000000000000ffULL, 0x000000000000ffffULL, 0x0000000000ffffffULL,
            0x00000000ffffffffULL, 0x000000ffffffffffULL, 0x0000ffffffffffffULL, 0x00ffffffffffffffULL
        };
        static const uint8_t SDR_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        /* The address given is unaligned, so let's align it first */
        uint32_t addr = offset + ee->gpr[base].word[0];
        uint32_t aligned_addr = addr & ~0x7;
        uint32_t shift = addr & 0x7;

        auto dword = ee->read<uint64_t>(aligned_addr);
        dword = (ee->gpr[rt].dword[0] << SDR_SHIFT[shift]) | (dword & SDR_MASK[shift]);
        ee->write<uint64_t>(aligned_addr, dword);

        log("SDR: Writing {:#x} to address {:#x}\n", dword, addr);
    }

    void op_dsrl(EmotionEngine* ee)
    {
        uint16_t sa = ee->instr.r_type.sa;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rt].dword[0] >> sa;

        log("DSRL: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> {:d}\n", rd, rt, ee->gpr[rt].dword[0], sa);
    }

    void op_srlv(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        uint16_t sa = ee->gpr[rs].word[0] & 0x3F;
        ee->gpr[rd].dword[0] = (int32_t)(ee->gpr[rt].word[0] >> sa);

        log("SRLV: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> ee->gpr[{:d}] ({:d})\n", rd, rt, ee->gpr[rt].word[0], rs, sa);
    }

    void op_dsrl32(EmotionEngine* ee)
    {
        uint16_t sa = ee->instr.r_type.sa;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rt].dword[0] >> (sa + 32);

        log("DSRL32: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> {:d}\n", rd, rt, ee->gpr[rt].dword[0], sa);
    }

    /* Syscall names per id. Info from ps2devwiki */
    std::unordered_map<uint16_t, std::string> syscalls =
    {
        { 0x5, "_ExceptionEpilogue" },
        { 0x12, "AddDmacHandler" },
        { 0x16, "_EnableDmac" },
        { 0x3c, "InitMainThread" },
        { 0x3d, "InitHeap" },
        { 0x40, "CreateSema" },
        { 0x43, "iSignalSema" },
        { 0x44, "WaitSema" },
        { 0x64, "FlushCache" },
        { 0x77, "SifSetDma" },
        { 0x78, "sceSifSetDChain" },
        { 0x79, "sceSifSetReg" },
        { 0x7a, "sceSifGetReg" },
        { 0x7c, "Deci2Call" }
    };

    void op_syscall(EmotionEngine* ee)
    {
        int8_t code = ee->read<uint8_t>(ee->instr.pc - 4);
        /* Negative syscalls also exist! */
        uint16_t id = std::abs(code);
        fmt::print("[EE] Executing {} with id {:#x}\n", syscalls[id], id);

        log("SYSCALL: {:#x}\n", id);
        ee->exception(Exception::Syscall, false);
    }

    void op_bltzl(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        int32_t imm = (int16_t)ee->instr.i_type.immediate;

        int32_t offset = imm << 2;
        int64_t reg = ee->gpr[rs].dword[0];
        if (reg < 0)
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }
        else
        {
            ee->fetch_next();
            ee->skip_branch_delay = true;
        }

        log("BLTZL: IF ee->gpr[{:d}] ({:#x}) < 0 THEN PC += {:#x}\n", rs, reg, offset);
    }

    void op_bgezl(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        int32_t imm = (int16_t)ee->instr.i_type.immediate;

        int32_t offset = imm << 2;
        int64_t reg = ee->gpr[rs].dword[0];

        if (reg >= 0)
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }
        else
        {
            ee->fetch_next();
            ee->skip_branch_delay = true;
        }

        log("BGEZL: IF ee->gpr[{:d}] ({:#x}) >= 0 THEN PC += {:#x}\n", rs, reg, offset);
    }

    void op_mfsa(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        ee->gpr[rd].dword[0] = ee->sa;

        log("MFSA\n");
    }

    void op_mthi(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        ee->hi0 = ee->gpr[rs].dword[0];

        log("MTHI: ee->hi0 = ee->gpr[{}] = {:#x}\n", rs, ee->hi0);
    }

    void op_mtlo(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        ee->lo0 = ee->gpr[rs].dword[0];

        log("MTLO: ee->lo0 = ee->gpr[{}] = {:#x}\n", rs, ee->lo0);
    }

    void op_mtsa(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.i_type.rs;
        ee->sa = ee->gpr[rs].dword[0];

        log("MTSA: SA = ee->gpr[{}] = {:#x}\n", rs, ee->sa);
    }

    void op_lwc1(EmotionEngine* ee)
    {
        uint16_t base = ee->instr.i_type.rs;
        uint16_t ft = ee->instr.i_type.rt;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        if (vaddr & 0x3) [[unlikely]]
        {
            log("[ERROR] LWC1: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorLoad);
        }
        else
        {
            ee->cop1.fpr[ft].uint = ee->read<uint32_t>(vaddr);
            log("LWC1: FPR[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", ft, ee->cop1.fpr[ft].uint, vaddr, base, ee->gpr[base].word[0], offset);
        }
    }

    void op_dsubu(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0] - ee->gpr[rt].dword[0];
        log("DSUBU: ee->gpr[{}] = ee->gpr[{}] ({:#x}) - ee->gpr[{}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);
    }

    void op_blezl(EmotionEngine* ee)
    {
        int32_t imm = (int16_t)ee->instr.i_type.immediate;
        uint16_t rs = ee->instr.i_type.rs;

        int32_t offset = imm << 2;
        int64_t reg = ee->gpr[rs].dword[0];
        if (reg <= 0)
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }
        else
        {
            ee->fetch_next();
            ee->skip_branch_delay = true;
        }

        log("BLEZL: IF ee->gpr[{:d}] ({:#x}) <= 0 THEN PC += {:#x}\n", rs, ee->gpr[rs].dword[0], offset);
    }

    void op_xor(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0] ^ ee->gpr[rt].dword[0];
        log("XOR: ee->gpr[{}] = ee->gpr[{}] ({:#x}) ^ ee->gpr[{}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);
    }

    void op_multu(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        uint64_t result = (uint64_t)ee->gpr[rs].word[0] * ee->gpr[rt].word[0];
        ee->gpr[rd].dword[0] = ee->lo0 = result & 0xFFFFFFFF;
        ee->hi0 = result >> 32;

        log("MULTU\n");
    }

    void op_lwl(EmotionEngine* ee)
    {
        static const uint32_t LWL_MASK[4] = { 0xffffff, 0x0000ffff, 0x000000ff, 0x00000000 };
        static const uint8_t LWL_SHIFT[4] = { 24, 16, 8, 0 };

        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint32_t aligned_addr = vaddr & ~0x3;
        int shift = vaddr & 0x3;

        uint32_t data = ee->read<uint32_t>(aligned_addr);
        uint32_t result = (ee->gpr[rt].word[0] & LWL_MASK[shift]) | (data << LWL_SHIFT[shift]);
        ee->gpr[rt].dword[0] = (int32_t)result;

        log("LWL: ee->gpr[{}] = {:#x} from address {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr);
    }

    void op_lwr(EmotionEngine* ee)
    {
        static const uint32_t LWR_MASK[4] = { 0x000000, 0xff000000, 0xffff0000, 0xffffff00 };
        static const uint8_t LWR_SHIFT[4] = { 0, 8, 16, 24 };

        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint32_t aligned_addr = vaddr & ~0x3;
        int shift = vaddr & 0x3;

        uint32_t data = ee->read<uint32_t>(aligned_addr);
        data = (ee->gpr[rt].word[0] & LWR_MASK[shift]) | (data >> LWR_SHIFT[shift]);

        /* NOTE: Only if LWR loads the sign bit, do the upper 64bits of the register get written! */
        if (!shift)
            ee->gpr[rt].dword[0] = (int32_t)data;
        else
            ee->gpr[rt].word[0] = data;

        log("LWR: ee->gpr[{}] = {:#x} from address {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr);
    }

    void op_swl(EmotionEngine* ee)
    {
        static const uint32_t SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0x00000000 };
        static const uint8_t SWL_SHIFT[4] = { 24, 16, 8, 0 };

        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint32_t aligned_addr = vaddr & ~0x3;
        int shift = vaddr & 0x3;

        uint32_t data = ee->read<uint32_t>(aligned_addr);
        data = (ee->gpr[rt].word[0] >> SWL_SHIFT[shift]) | (data & SWL_MASK[shift]);
        ee->write<uint32_t>(aligned_addr, data);

        log("SWL: Writing {:#x} to address {:#x}\n", data, aligned_addr);
    }

    void op_swr(EmotionEngine* ee)
    {
        static const uint32_t SWR_MASK[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
        static const uint8_t SWR_SHIFT[4] = { 0, 8, 16, 24 };

        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint32_t aligned_addr = vaddr & ~0x3;
        int shift = vaddr & 0x3;

        uint32_t data = ee->read<uint32_t>(aligned_addr);
        data = (ee->gpr[rt].word[0] << SWR_SHIFT[shift]) | (data & SWR_MASK[shift]);
        ee->write<uint32_t>(aligned_addr, data);

        log("SWR: Writing {:#x} to address {:#x}\n", data, aligned_addr);
    }

    void op_sqc2(EmotionEngine* ee)
    {
        uint16_t ft = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        ee->write<uint128_t>(vaddr, ee->emulator->vu[0]->regs.vf[ft].qword);

        log("SQC2: Writing VF[{}] to address {:#x}\n", ft, vaddr);
    }

    void op_dsra(EmotionEngine* ee)
    {
        uint16_t sa = ee->instr.r_type.sa;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        int64_t reg = ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[0] = reg >> sa;

        log("DSRA: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> {:d}\n", rd, rt, ee->gpr[rt].dword[0], sa);
    }

    void op_sub(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rt = ee->instr.r_type.rt;

        int32_t reg1 = ee->gpr[rs].dword[0];
        int32_t reg2 = ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[0] = reg1 - reg2;

        log("SUB: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) - ee->gpr[{:d}] ({:#x})\n", rd, rs, reg1, rt, reg2);
    }

    void op_add(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        int32_t result = ee->gpr[rs].dword[0] + ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[0] = result;

        log("ADD: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) + ee->gpr[{:d}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);
    }

    void op_di(EmotionEngine* ee)
    {
        auto& status = ee->cop0.status;
        if (status.edi || status.exl || status.erl || !status.ksu)
        {
            status.eie = 0;
        }

        log("DI: STATUS.EIE = {:d}\n", (uint16_t)status.eie);
    }

    bool done = false;

    void op_eret(EmotionEngine* ee)
    {
        log("ERET!\n");
        auto& status = ee->cop0.status;
        if (status.erl)
        {
            ee->pc = ee->cop0.error_epc;
            status.erl = 0;
        }
        else
        {
            ee->pc = ee->cop0.epc;
            status.exl = 0;
        }

        /* Skip branch delay slot */
        ee->branch_taken = true;
        ee->fetch_next();

        if (!done)
        {
            ee->emulator->load_elf(TEST_ELF);
            done = true;
        }
    }

    void op_ei(EmotionEngine* ee)
    {
        auto& status = ee->cop0.status;
        if (status.edi || status.exl || status.erl || !status.ksu)
        {
            status.eie = 1;
        }

        log("EI: STATUS.EIE = {:d}\n", (uint16_t)status.eie);
    }

    void op_cop1(EmotionEngine* ee)
    {
        uint32_t fmt = (ee->instr.value >> 21) & 0x1f;
        switch (fmt)
        {
        case 0b00100: op_mtc1(ee); break;
        case 0b00110: op_ctc1(ee); break;
        case 0b00010: op_cfc1(ee); break;
        case 0b10000: ee->cop1.execute(ee->instr); break;
        default:
            common::Emulator::terminate("[ERROR] Unimplemented COP1 instruction {:#07b}\n", fmt);
        }
    }

    void op_mtc1(EmotionEngine* ee)
    {
        uint16_t fs = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        log("MTC1: FPR[{}] = ee->gpr[{}] ({:#x})\n", fs, rt, ee->gpr[rt].word[0]);
        ee->cop1.fpr[fs].uint = ee->gpr[rt].word[0];
    }

    void op_ctc1(EmotionEngine* ee)
    {
        uint16_t fs = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        log("[COP1] CTC1: FCR{} = ee->gpr[{}] = ({:#x})\n", fs, rt, ee->gpr[rt].word[0]);
        switch (fs)
        {
        case 0: ee->cop1.fcr0.value = ee->gpr[rt].word[0]; break;
        case 31: ee->cop1.fcr31.value = ee->gpr[rt].word[0]; break;
        }
    }

    void op_cfc1(EmotionEngine* ee)
    {
        uint16_t fs = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        log("[COP1] CFC1: ee->gpr[{}] = FCR{}\n", rt, fs);
        switch (fs)
        {
        case 0: ee->gpr[rt].dword[0] = (int32_t)ee->cop1.fcr0.value; break;
        case 31: ee->gpr[rt].dword[0] = (int32_t)ee->cop1.fcr31.value; break;
        }
    }

    void op_cop2(EmotionEngine* ee)
    {
        auto& vu0 = ee->emulator->vu[0];

        uint32_t fmt = (ee->instr.value >> 21) & 0x1f;
        switch (fmt)
        {
        case 0b00010:
            vu0->op_cfc2(ee->instr);
            break;
        case 0b00110:
            vu0->op_ctc2(ee->instr);
            break;
        case 0b00001:
            vu0->op_qmfc2(ee->instr);
            break;
        case 0b00101:
            vu0->op_qmtc2(ee->instr);
            break;
        case 0b10000 ... 0b11111:
            vu0->special1(ee->instr);
            break;
        default:
            common::Emulator::terminate("[ERROR] Unimplemented COP2 instruction {:#07b}\n", fmt);
        }
    }

    void op_por(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        log("POR: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) | ee->gpr[{:d}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);

        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0] | ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[1] = ee->gpr[rs].dword[1] | ee->gpr[rt].dword[1];
    }

    void op_padduw(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        /* SIMD needed here! */
        for (int i = 0; i < 4; i++)
        {
            uint64_t result = (uint64_t)ee->gpr[rt].word[i] + (uint64_t)ee->gpr[rs].word[i];
            ee->gpr[rd].word[i] = (result > 0xffffffff ? 0xffffffff : result);
        }

        log("PADDUW: ee->gpr[{}] = ee->gpr[{}] + ee->gpr[{}]\n", rd, rs, rt);
    }

    void op_plzcw(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint64_t rs = ee->instr.r_type.rs;

        for (int i = 0; i < 2; i++)
        {
            uint32_t word = ee->gpr[rs].word[i];
            bool msb = word & (1 << 31);
            /* To count the leading ones, invert it so
               we can count zeros */
            word = (msb ? ~word : word);
            /* Passing zero to __builtin_clz produces undefined results.
               Thankfully when the number is zero the answer is always 0x1f */
            ee->gpr[rd].word[i] = (word != 0 ? __builtin_clz(word) - 1 : 0x1f);
        }

        log("PLZCW\n");
    }

    void op_mfhi1(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        ee->gpr[rd].dword[0] = ee->hi1;

        log("mfhi1: ee->gpr[{}] = ee->hi1 = {:#x}\n", rd, ee->hi1);
    }

    void op_mthi1(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        ee->hi1 = ee->gpr[rs].dword[0];

        log("MTee->hi1: ee->hi1 = ee->gpr[{}] = {:#x}\n", rs, ee->hi1);
    }

    void op_mtlo1(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        ee->lo1 = ee->gpr[rs].dword[0];

        log("MTee->lo1: ee->lo1 = ee->gpr[{}] = {:#x}\n", rs, ee->hi1);
    }

    void op_pcpyh(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        for (int i = 0; i < 4; i++)
        {
            ee->gpr[rd].hword[i] = ee->gpr[rt].hword[0];
            ee->gpr[rd].hword[i + 4] = ee->gpr[rt].hword[4];
        }

        log("PCPYH: ee->gpr[{}] <- ee->gpr[{}]\n", rd, rt);
    }

    void op_pcpyld(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[1] = ee->gpr[rs].dword[0];

        log("PCPYLD: ee->gpr[{}] = {:#x}{:016x} (ee->gpr[{}], ee->gpr[{}])\n", rd, ee->gpr[rd].dword[0], ee->gpr[rd].dword[1], rt, rs);
    }

    void op_pnor(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        log("PNOR: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) NOR ee->gpr[{:d}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);

        ee->gpr[rd].dword[0] = ~(ee->gpr[rs].dword[0] | ee->gpr[rt].dword[0]);
        ee->gpr[rd].dword[1] = ~(ee->gpr[rs].dword[1] | ee->gpr[rt].dword[1]);
    }

    void op_psubb(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        for (int i = 0; i < 16; i++)
        {
            ee->gpr[rd].byte[i] = (int8_t)ee->gpr[rs].byte[i] - (int8_t)ee->gpr[rt].byte[i];
        }

        log("PSUBB\n");
    }

    void op_pand(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0] & ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[1] = ee->gpr[rs].dword[1] & ee->gpr[rt].dword[1];

        log("PAND\n");
    }

    void op_pcpyud(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[1];
        ee->gpr[rd].dword[1] = ee->gpr[rt].dword[1];

        log("PCPYUD\n");
    }

    void op_pxor(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0] ^ ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[1] = ee->gpr[rs].dword[1] ^ ee->gpr[rt].dword[1];

        log("PXOR\n");
    }

    void op_psubw(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        for (int i = 0; i < 4; i++)
        {
            ee->gpr[rd].word[i] = (int32_t)ee->gpr[rs].word[i] - (int32_t)ee->gpr[rt].word[i];
        }

        log("PSUBW\n");
    }

    void op_dsrav(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        int64_t reg = (int64_t)ee->gpr[rt].dword[0];
        uint16_t sa = ee->gpr[rs].word[0] & 0x3F;
        ee->gpr[rd].dword[0] = reg >> sa;

        log("DSRAV: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> ee->gpr[{:d}] ({:d})\n", rd, rt, reg, rs, sa);
    }

    void op_lhu(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];

        log("LHU: ee->gpr[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], offset);
        if (vaddr & 0x1) [[unlikely]]
        {
            log("[ERROR] LHU: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorLoad);
        }
        else
            ee->gpr[rt].dword[0] = ee->read<uint16_t>(vaddr);
    }

    void op_dsll32(EmotionEngine* ee)
    {
        uint16_t sa = ee->instr.r_type.sa;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        log("DSLL32: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) << {:d} + 32\n", rd, rt, ee->gpr[rt].dword[0], sa);
        ee->gpr[rd].dword[0] = ee->gpr[rt].dword[0] << (sa + 32);
    }

    void op_dsra32(EmotionEngine* ee)
    {
        uint16_t sa = ee->instr.r_type.sa;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        int64_t reg = (int64_t)ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[0] = reg >> (sa + 32);

        log("DSRA32: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> {:d} + 32\n", rd, rt, reg, sa);
    }

    void op_lw(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t base = ee->instr.i_type.rs;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];

        if (vaddr & 0x3) [[unlikely]]
        {
            log("[ERROR] LW: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorLoad);
        }
        else
        {
            ee->gpr[rt].dword[0] = (int32_t)ee->read<uint32_t>(vaddr);
            log("LW: ee->gpr[{:d}] = {:#x} from address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, ee->gpr[rt].dword[0], vaddr, base, ee->gpr[base].word[0], offset);
        }
    }

    void op_addiu(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;
        int16_t imm = (int16_t)ee->instr.i_type.immediate;

        int32_t result = ee->gpr[rs].dword[0] + imm;
        ee->gpr[rt].dword[0] = result;

        log("ADDIU: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, rs, ee->gpr[rs].dword[0], imm);
    }

    void op_tlbwi(EmotionEngine* ee)
    {
        log("TLBWI\n");
    }

    void op_mtc0(EmotionEngine* ee)
    {
        uint16_t rt = (ee->instr.value >> 16) & 0x1F;
        uint16_t rd = (ee->instr.value >> 11) & 0x1F;

        ee->cop0.regs[rd] = ee->gpr[rt].word[0];

        log("MTC0: COP0[{:d}] = ee->gpr[{:d}] ({:#x})\n", rd, rt, ee->gpr[rt].word[0]);
    }

    void op_mmi(EmotionEngine* ee)
    {
        switch(ee->instr.r_type.funct)
        {
        case 0b100000: op_madd1(ee); break;
        case 0b000000: op_madd(ee); break;
        case 0b011011: op_divu1(ee); break;
        case 0b010010: op_mflo1(ee); break;
        case 0b011000: op_mult1(ee); break;
        case 0b000100: op_plzcw(ee); break;
        case 0b010000: op_mfhi1(ee); break;
        case 0b010001: op_mthi1(ee); break;
        case 0b010011: op_mtlo1(ee); break;
        case 0b101001:
        {
            switch (ee->instr.r_type.sa)
            {
            case 0b10010: op_por(ee); break;
            case 0b11011: op_pcpyh(ee); break;
            case 0b10011: op_pnor(ee); break;
            case 0b01110: op_pcpyud(ee); break;
            default:
                common::Emulator::terminate("[ERROR] Unimplemented MMI3 instruction: {:#07b}\n", (uint16_t)ee->instr.r_type.sa);
            }
            break;
        }
        case 0b001000:
        {
            switch (ee->instr.r_type.sa)
            {
            case 0b01001: op_psubb(ee); break;
            case 0b00001: op_psubw(ee); break;
            default:
                common::Emulator::terminate("[ERROR] Unimplemented MMI0 instruction: {:#07b}\n", (uint16_t)ee->instr.r_type.sa);
            }
            break;
        }
        case 0b101000:
        {
            switch (ee->instr.r_type.sa)
            {
            case 0b10000: op_padduw(ee); break;
            default:
                common::Emulator::terminate("[ERROR] Unimplemented MMI1 instruction: {:#07b}\n", (uint16_t)ee->instr.r_type.sa);
            }
            break;
        }
        case 0b001001:
        {
            switch (ee->instr.r_type.sa)
            {
            case 0b01110: op_pcpyld(ee); break;
            case 0b10010: op_pand(ee); break;
            case 0b10011: op_pxor(ee); break;
            default:
                common::Emulator::terminate("[ERROR] Unimplemented MMI2 instruction: {:#07b}\n", (uint16_t)ee->instr.r_type.sa);
            }
            break;
        }
        default:
            common::Emulator::terminate("[ERROR] Unimplemented MMI instruction: {:#05b}\n", (uint16_t)ee->instr.r_type.funct);
        }
    }

    void op_madd1(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rt = ee->instr.r_type.rt;

        uint64_t lo = ee->lo1 & 0xFFFFFFFF;
        uint64_t hi = ee->hi1 & 0xFFFFFFFF;
        int64_t result = (hi << 32 | lo) + (int64_t)ee->gpr[rs].word[0] * (int64_t)ee->gpr[rt].word[0];

        ee->lo1 = (int64_t)(int32_t)(result & 0xFFFFFFFF);
        ee->hi1 = (int64_t)(int32_t)(result >> 32);
        ee->gpr[rd].dword[0] = (int64_t)ee->lo1;

        log("MADD1: ee->gpr[{:d}] = ee->lo1 = {:#x} and ee->hi1 = {:#x}\n", rd, ee->lo1, ee->hi1);
    }

    void op_jalr(EmotionEngine* ee)
    {
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        ee->pc = ee->gpr[rs].word[0];
        ee->gpr[rd].dword[0] = ee->instr.pc + 8;

        ee->next_instr.is_delay_slot = true;
        ee->branch_taken = true;
        log("JALR: Jumping to PC = ee->gpr[{:d}] ({:#x}) with link address {:#x}\n", rs, ee->pc, ee->gpr[rd].dword[0]);
    }

    void op_sd(EmotionEngine* ee)
    {
        uint16_t base = ee->instr.i_type.rs;
        uint16_t rt = ee->instr.i_type.rt;
        int16_t offset = (int16_t)ee->instr.i_type.immediate;

        uint32_t vaddr = offset + ee->gpr[base].word[0];
        uint64_t data = ee->gpr[rt].dword[0];

        log("SD: Writing ee->gpr[{:d}] ({:#x}) to address {:#x} = ee->gpr[{:d}] ({:#x}) + {:#x}\n", rt, data, vaddr, base, ee->gpr[base].word[0], offset);
        if (vaddr & 0x7) [[unlikely]]
        {
            log("[ERROR] SD: Address {:#x} is not aligned\n", vaddr);
            ee->exception(Exception::AddrErrorStore);
        }
        else
            ee->write<uint64_t>(vaddr, data);
    }

    void op_jal(EmotionEngine* ee)
    {
        uint32_t instr_index = ee->instr.j_type.target;

        ee->gpr[31].dword[0] = ee->instr.pc + 8;
        ee->pc = ((ee->instr.pc + 4) & 0xF0000000) | (instr_index << 2);

        ee->next_instr.is_delay_slot = true;
        ee->branch_taken = true;
        log("JAL: Jumping to PC = {:#x} with return link address {:#x}\n", ee->pc, ee->gpr[31].dword[0]);
    }

    void op_sra(EmotionEngine* ee)
    {
        uint16_t sa = ee->instr.r_type.sa;
        uint16_t rd = ee->instr.r_type.rd;
        uint16_t rt = ee->instr.r_type.rt;

        int32_t reg = (int32_t)ee->gpr[rt].word[0];
        ee->gpr[rd].dword[0] = reg >> sa;

        log("SRA: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) >> {:d}\n", rd, rt, ee->gpr[rt].word[0], sa);
    }

    void op_bgez(EmotionEngine* ee)
    {
        int32_t imm = (int16_t)ee->instr.i_type.immediate;
        uint16_t rs = ee->instr.i_type.rs;

        int32_t offset = imm << 2;
        int64_t reg = (int64_t)ee->gpr[rs].dword[0];
        if (reg >= 0)
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }

        ee->next_instr.is_delay_slot = true;
        log("BGEZ: IF ee->gpr[{:d}] ({:#x}) >= 0 THEN PC += {:#x}\n", rs, reg, offset);
    }

    void op_addu(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        int32_t result = ee->gpr[rs].dword[0] + ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[0] = result;

        log("ADDU: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) + ee->gpr[{:d}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);
    }

    void op_daddu(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        int64_t reg1 = ee->gpr[rs].dword[0];
        int64_t reg2 = ee->gpr[rt].dword[0];
        ee->gpr[rd].dword[0] = reg1 + reg2;

        log("DADDU: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) + ee->gpr[{:d}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);
    }

    void op_andi(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;
        uint64_t imm = ee->instr.i_type.immediate;

        ee->gpr[rt].dword[0] =  ee->gpr[rs].dword[0] & imm;

        log("ANDI: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) & {:#x}\n", rt, rs, ee->gpr[rs].dword[0], imm);
    }

    void op_beq(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;
        int32_t imm = (int16_t)ee->instr.i_type.immediate;

        int32_t offset = imm << 2;
        if (ee->gpr[rs].dword[0] == ee->gpr[rt].dword[0])
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }

        ee->next_instr.is_delay_slot = true;
        log("BEQ: IF ee->gpr[{:d}] ({:#x}) == ee->gpr[{:d}] ({:#x}) THEN PC += {:#x}\n", rt, ee->gpr[rt].dword[0], rs, ee->gpr[rs].dword[0], offset);
    }

    void op_or(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        log("OR: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) | ee->gpr[{:d}] ({:#x})\n", rd, rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0]);

        ee->gpr[rd].dword[0] = ee->gpr[rs].dword[0] | ee->gpr[rt].dword[0];
    }

    void op_mult(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.r_type.rt;
        uint16_t rs = ee->instr.r_type.rs;
        uint16_t rd = ee->instr.r_type.rd;

        int64_t reg1 = (int64_t)ee->gpr[rs].dword[0];
        int64_t reg2 = (int64_t)ee->gpr[rt].dword[0];
        int64_t result = reg1 * reg2;

        ee->gpr[rd].dword[0] = ee->lo0 = (int32_t)(result & 0xFFFFFFFF);
        ee->hi0 = (int32_t)(result >> 32);

        log("MULT: ee->gpr[{:d}] ({:#x}) * ee->gpr[{:d}] ({:#x}) = {:#x} STORED IN ee->gpr[{:d}] = ee->lo0 = {:#x} and ee->hi0 = {:#x}\n", rs, reg1, rt, reg2, result, rd, ee->lo0, ee->hi0);
    }

    void op_divu(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;

        if (ee->gpr[rt].word[0] == 0)
        {
            ee->hi0 = (int32_t)ee->gpr[rs].word[0];
            ee->lo0 = (int32_t)0xffffffff;
        }
        else
        {
            ee->hi0 = (int32_t)(ee->gpr[rs].word[0] % ee->gpr[rt].word[0]);
            ee->lo0 = (int32_t)(ee->gpr[rs].word[0] / ee->gpr[rt].word[0]);
        }

        log("DIVU: ee->gpr[{:d}] ({:#x}) / ee->gpr[{:d}] ({:#x}) OUTPUT ee->lo0 = {:#x} and ee->hi0 = {:#x}\n", rs, ee->gpr[rs].word[0], rt, ee->gpr[rt].word[0], ee->lo0, ee->hi0);
    }

    void op_beql(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;
        int32_t imm = (int16_t)ee->instr.i_type.immediate;

        int32_t offset = imm << 2;
        if (ee->gpr[rs].dword[0] == ee->gpr[rt].dword[0])
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }
        else
        {
            ee->fetch_next();
            ee->skip_branch_delay = 1;
        }

        log("BEQL: IF ee->gpr[{:d}] ({:#x}) == ee->gpr[{:d}] ({:#x}) THEN PC += {:#x}\n", rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0], offset);
    }

    void op_mflo(EmotionEngine* ee)
    {
        uint16_t rd = ee->instr.r_type.rd;

        ee->gpr[rd].dword[0] = ee->lo0;

        log("MFLO: ee->gpr[{:d}] = ee->lo0 ({:#x})\n", rd, ee->lo0);
    }

    void op_sltiu(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;
        uint64_t imm = (int16_t)ee->instr.i_type.immediate;

        ee->gpr[rt].dword[0] = ee->gpr[rs].dword[0] < imm;

        log("SLTIU: ee->gpr[{:d}] = ee->gpr[{:d}] ({:#x}) < {:#x}\n", rt, rs, ee->gpr[rs].dword[0], imm);
    }

    void op_bnel(EmotionEngine* ee)
    {
        uint16_t rt = ee->instr.i_type.rt;
        uint16_t rs = ee->instr.i_type.rs;
        int32_t imm = (int16_t)ee->instr.i_type.immediate;

        int32_t offset = imm << 2;
        if (ee->gpr[rs].dword[0] != ee->gpr[rt].dword[0])
        {
            ee->pc = ee->instr.pc + 4 + offset;
            ee->branch_taken = true;
        }
        else
        {
            ee->fetch_next();
            ee->skip_branch_delay = 1;
        }

        log("BNEL: IF ee->gpr[{:d}] ({:#x}) != ee->gpr[{:d}] ({:#x}) THEN PC += {:#x}\n", rs, ee->gpr[rs].dword[0], rt, ee->gpr[rt].dword[0], offset);
    }
};
