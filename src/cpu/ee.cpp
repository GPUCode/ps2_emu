#include <cpu/ee.hpp>
#include <common/manager.hpp>
#include <bitset>
#include <iostream>
#include <fmt/color.h>

#ifndef NDEBUG
#define log(x) (void)0
#else
#define log(...) fmt::print(__VA_ARGS__)
#endif

constexpr auto BOLD = fg(fmt::color::green_yellow) | fmt::emphasis::bold;

EmotionEngine::EmotionEngine(ComponentManager* parent)
{
    manager = parent;

    /* Reset CPU state. */
    reset_state();
}

void EmotionEngine::tick()
{
    /* Fetch next instruction. */
    fetch_instruction();
}

void EmotionEngine::reset_state()
{
    /* Reset PC. */
    pc = 0xbfc00000;

    /* Reset instruction holders */
    instr.value = instr.pc = 0;
    next_instr.value = next_instr.pc = 0;

    /* Set this to zero */
    gpr[0].dword[0] = gpr[0].dword[1] = 0;

    /* Set EE pRId */
    cop0.prid = 0x00002E20;
}

void EmotionEngine::fetch_instruction()
{
    /* Handle branch delay slots by prefetching the next one */
    instr = next_instr;
    next_instr.value = read<uint32_t>(pc);
    next_instr.pc = pc;
    
    /* Update PC */
    pc += 4;
    log("PC: {:#x} instruction: {:#x} ", instr.pc, instr.value);

    /* Skip the delay slot for any BEQ* instructions */
    if (skip_branch_delay)
    {
        skip_branch_delay = false;
        log("SKIPPED delay slot\n");
        return;
    }

    switch(instr.opcode)
    {
    case COP0_OPCODE: op_cop0(); break;
    case SPECIAL_OPCODE: op_special(); break;
    case 0b101011: op_sw(); break;
    case 0b001010: op_slti(); break;
    case 0b000101: op_bne(); break;
    case 0b001101: op_ori(); break;
    case 0b001000: op_addi(); break;
    case 0b011110: op_lq(); break;
    case 0b001111: op_lui(); break;
    case 0b001001: op_addiu(); break;
    case 0b011100: op_mmi(); break;
    case 0b111111: op_sd(); break;
    case 0b000011: op_jal(); break;
    case 0b000001: op_regimm(); break;
    case 0b001100: op_andi(); break;
    case 0b000100: op_beq(); break;
    case 0b010100: op_beql(); break;
    case 0b001011: op_sltiu(); break;
    case 0b010101: op_bnel(); break;
    case 0b100000: op_lb(); break;
    case 0b111001: op_swc1(); break;
    case 0b100100: op_lbu(); break;
    case 0b110111: op_ld(); break;
    case 0b000010: op_j(); break;
    case 0b100011: op_lw(); break;
    case 0b101000: op_sb(); break;
    case 0b000110: op_blez(); break;
    case 0b000111: op_bgtz(); break;
    case 0b100101: op_lhu(); break;
    default:
        fmt::print("[ERROR] Unimplemented opcode: {:#06b}\n", instr.opcode & 0x3F);
        std::abort();
    }

    cop0.regs[9]++;
}

template <typename T>
T EmotionEngine::read(uint32_t addr)
{
    if (addr >= 0x70000000 && addr < 0x70004000) /* Read from scratchpad */
        return *(T*)&scratchpad[addr & 0x3FFF];
    else
        return manager->read<T>(addr);
}

template <typename T>
void EmotionEngine::write(uint32_t addr, T data)
{
    if (addr >= 0x70000000 && addr < 0x70004000)
        *(T*)&scratchpad[addr & 0x3FFF] = data;
    else
        manager->write<T>(addr, data);

    if (scratchpad[0x70003edd & 0x3FFF] == 0x33) __debugbreak();
}

void EmotionEngine::op_cop0()
{
    /*  The manual says that COP0 is only accessible in kernel mode
        There are exceptions but we will deal with those later.  */
    if (cop0.get_operating_mode() != OperatingMode::KERNEL_MODE)
        return;

    uint16_t type = instr.value >> 21 & 0x1F;
	switch (type)
	{
	case COP0_MF0:
		switch(instr.value & 0x7)
        {
        case 0b000: op_mfc0(); break; 
        default:
            fmt::print("[ERROR] Unimplemented COP0 MF0 instruction: {:#03b}\n", instr.value & 0x7);
            std::exit(1);
        }
        break;
    case COP0_MT0:
        switch (instr.value & 0x7)
        {
        case 0b000: op_mtc0(); break;
        default:
            fmt::print("[ERROR] Unimplemented COP0 MT0 instruction: {:#03b}\n", instr.value & 0x7);
            std::exit(1);
        }
        break;
	case COP0_TLB:
        if ((instr.value & 0x3F) == 0b000010) op_tlbwi();
        break;
	default:
		fmt::print("[ERROR] Unimplemented COP0 instruction: {:#05b}\n", type);
		std::exit(1);
	}
}

void EmotionEngine::op_mfc0()
{
    uint16_t rd = (instr.value >> 11) & 0x1F;
    uint16_t rt = (instr.value >> 16) & 0x1F;

    gpr[rt].dword[0] = cop0.regs[rd];
    log("MFC0: GPR[{:d}] = COP0_REG[{:d}] ({:#x})\n", rt, rd, cop0.regs[rd]);
}

void EmotionEngine::op_special()
{
    switch(instr.r_type.funct)
    {
    case 0b000000: op_sll(); break;
    case 0b001000: op_jr(); break;
    case 0b001111: op_sync(); break;
    case 0b001001: op_jalr(); break;
    case 0b000011: op_sra(); break;
    case 0b100001: op_addu(); break;
    case 0b101101: op_daddu(); break;
    case 0b100101: op_or(); break;
    case 0b011000: op_mult(); break;
    case 0b011011: op_divu(); break;
    case 0b010010: op_mflo(); break;
    case 0b011010: op_div(); break;
    case 0b010000: op_mfhi(); break;
    case 0b101011: op_sltu(); break;
    case 0b100011: op_subu(); break;
    case 0b001011: op_movn(); break;
    case 0b101010: op_slt(); break;
    case 0b100100: op_and(); break;
    case 0b000010: op_srl(); break;
    case 0b111100: op_dsll32(); break;
    case 0b111111: op_dsra32(); break;
    case 0b111000: op_dsll(); break;
    default:
        fmt::print("[ERROR] Unimplemented SPECIAL instruction: {:#06b}\n", (uint16_t)instr.r_type.funct);
		std::exit(1);
    }
}

void EmotionEngine::op_regimm()
{
    uint16_t type = (instr.value >> 16) & 0x1F;
    switch(type)
    {
    case 0b00001: op_bgez(); break;
    case 0b00000: op_bltz(); break;
    default:
        fmt::print("[ERROR] Unimplemented REGIMM instruction: {:#05b}\n", type);
		std::exit(1);
    }
}

void EmotionEngine::op_sw()
{
    uint16_t base = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    uint32_t data = gpr[rt].word[0];

    log(BOLD, "SW: Writing GPR[{:d}] ({:#x}) to address {:#x} = GPR[{:d}] ({:#x}) + {:d}\n", rt, data, vaddr, base, gpr[base].word[0], offset);
    if ((vaddr & 0b11) != 0)
    {
        log("[ERROR] SW: Address {:#x} is not aligned\n", vaddr);
        std::exit(1); /* NOTE: SignalException (AddressError) */
    }
    else
        write<uint32_t>(vaddr, data);
}

void EmotionEngine::op_sll()
{
    uint16_t rt = instr.r_type.rt;
    uint16_t rd = instr.r_type.rd;
    uint16_t sa = instr.r_type.sa;

    gpr[rd].dword[0] = (uint64_t)(int32_t)(gpr[rt].word[0] << sa);

    if (instr.value == 0) log("NOP\n");
    else log("SLL: GPR[{:d}] = GPR[{:d}] ({:#x}) << {:d}\n", rd, rt, gpr[rt].dword[0], sa);
}

void EmotionEngine::op_slti()
{
    uint16_t rs = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int64_t imm = (int16_t)instr.i_type.immediate;

    int64_t reg = gpr[rs].dword[0];
    gpr[rt].dword[0] = reg < imm;
    
    log("SLTI: GPR[{:d}] = GPR[{:d}] ({:#x}) < {:#x}\n", rt, rs, reg, imm);
}

void EmotionEngine::op_bne()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    int32_t imm = (int16_t)instr.i_type.immediate;

    int32_t offset = imm << 2;
    if (gpr[rs].dword[0] != gpr[rt].dword[0])
        pc += offset - 4;
    
    log("BNE: IF GPR[{:d}] ({:#x}) != GPR[{:d}] ({:#x}) THEN PC += {:#x}\n", rt, gpr[rt].dword[0], rs, gpr[rs].dword[0], offset);
}

void EmotionEngine::op_ori()
{
    uint16_t rs = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    uint16_t imm = instr.i_type.immediate;

    log("ORI: GPR[{:d}] = GPR[{:d}] ({:#x}) | {:#x}\n", rt, rs, gpr[rs].dword[0], imm);

    gpr[rt].dword[0] = gpr[rs].dword[0] | (uint64_t)imm;
}

void EmotionEngine::op_addi()
{
    uint16_t rs = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int16_t imm = (int16_t)instr.i_type.immediate;

    /* TODO: Overflow detection */
    int64_t reg = gpr[rs].dword[0];
    gpr[rt].dword[0] = reg + imm;

    log("ADDI: GPR[{:d}] = GPR[{:d}] ({:#x}) + {:#x}\n", rt, rs, reg, imm);
}

void EmotionEngine::op_lq()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t base = instr.i_type.rs;
    int16_t imm = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = (gpr[base].word[0] + imm) & 0b0000;
    gpr[rt].dword[0] = read<uint64_t>(vaddr);
    gpr[rt].dword[1] = read<uint64_t>(vaddr + 8);

    log("LQ: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x} + {:#x}\n", rt, gpr[rt].dword[0], vaddr, base, gpr[base].word[0], imm);
}

void EmotionEngine::op_lui()
{
    uint16_t rt = instr.i_type.rt;
    uint32_t imm = instr.i_type.immediate;

    gpr[rt].dword[0] = (int64_t)(int32_t)(imm << 16);

    log("LUI: GPR[{:d}] = {:#x}\n", rt, gpr[rt].dword[0]);
}

void EmotionEngine::op_jr()
{
    uint16_t rs = instr.i_type.rs;
    pc = gpr[rs].word[0];
    
    log("JR: Jumped to GPR[{:d}] = {:#x}\n", rs, pc);
}

void EmotionEngine::op_sync()
{
    log("SYNC\n");
}

void EmotionEngine::op_lb()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t base = instr.i_type.rs;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    gpr[rt].dword[0] = (int64_t)read<uint8_t>(vaddr);

    log(BOLD, "LB: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt].dword[0], vaddr, base, gpr[base].word[0], offset);
}

void EmotionEngine::op_swc1()
{
    uint16_t base = instr.i_type.rs;
    uint16_t ft = instr.i_type.rt;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    uint32_t data = fpr[ft].word[0];

    log("SWC1: Writing FPR[{:d}] ({:#x}) to address {:#x} = GPR[{:d}] ({:#x}) + {:d}\n", ft, data, vaddr, base, gpr[base].word[0], offset);
    if ((vaddr & 0b11) != 0)
    {
        log("[ERROR] SW: Address {:#x} is not aligned\n", vaddr);
        std::exit(1); /* NOTE: SignalException (AddressError) */
    }
    else
        write<uint32_t>(vaddr, data);
}

void EmotionEngine::op_lbu()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t base = instr.i_type.rs;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    gpr[rt].dword[0] = read<uint8_t>(vaddr);

    log(BOLD, "LBU: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt].dword[0], vaddr, base, gpr[base].word[0], offset);
}

void EmotionEngine::op_ld()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t base = instr.i_type.rs;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    gpr[rt].dword[0] = read<uint64_t>(vaddr);

    log(BOLD, "LD: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt].dword[0], vaddr, base, gpr[base].word[0], offset);
}

void EmotionEngine::op_j()
{
    uint32_t instr_index = instr.j_type.target;

    pc = (pc & 0xF0000000) | (instr_index << 2);

    log("J: Jumping to PC = {:#x}\n", pc);
}

void EmotionEngine::op_sb()
{
    uint16_t base = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    uint16_t data = gpr[rt].word[0] & 0xFF;

    log(BOLD, "SB: Writing GPR[{:d}] ({:#x}) to address {:#x} = GPR[{:d}] ({:#x}) + {:d}\n", rt, data, vaddr, base, gpr[base].word[0], offset);
    write<uint8_t>(vaddr, data);
}

void EmotionEngine::op_div()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;

    if (gpr[rt].word[0] != 0)
    {
        int32_t q = (int32_t)gpr[rs].word[0] / (int32_t)gpr[rt].word[0];
        lo0 = (int64_t)(int32_t)(q & 0xFFFFFFFF);
        
        int32_t r = (int32_t)gpr[rs].word[0] % (int32_t)gpr[rt].word[0];
        hi0 = (int64_t)(int32_t)(r & 0xFFFFFFFF);

        log("DIV: LO0 = GPR[{:d}] ({:#x}) / GPR[{:d}] ({:#x})\n", rs, gpr[rs].word[0], rt, gpr[rt].word[0]);
    }
    else
    {
        log("[ERROR] DIV: Division by zero!\n");
        std::abort();
    }
}

void EmotionEngine::op_mfhi()
{
    uint16_t rd = instr.r_type.rd;

    gpr[rd].dword[0] = hi0;

    log("MFHI: GPR[{:d}] = HI0 ({:#x})\n", rd, hi0);
}

void EmotionEngine::op_sltu()
{
    uint16_t rd = instr.r_type.rd;
    uint16_t rs = instr.r_type.rs;
    uint16_t rt = instr.r_type.rt;

    log("SLTU: GPR[{:d}] = GPR[{:d}] ({:#x}) < GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs].dword[0], rt, gpr[rt].dword[0]);
    gpr[rd].dword[0] = gpr[rs].dword[0] < gpr[rt].dword[0];
}

void EmotionEngine::op_blez()
{
    int32_t imm = (int16_t)instr.i_type.immediate;
    uint16_t rs = instr.i_type.rs;

    int32_t offset = imm << 2;
    int64_t reg = gpr[rs].dword[0];
    if (reg <= 0)
        pc += offset - 4;

    log("BLEZ: IF GPR[{:d}] ({:#x}) <= 0 THEN PC += {:#x}\n", rs, gpr[rs].dword[0], offset);
}

void EmotionEngine::op_subu()
{
    uint16_t rd = instr.r_type.rd;
    uint16_t rs = instr.r_type.rs;
    uint16_t rt = instr.r_type.rt;
        
    int32_t reg1 = gpr[rs].dword[0];
    int32_t reg2 = gpr[rt].dword[0];
    gpr[rd].dword[0] = reg1 - reg2;

    log("SUBU: GPR[{:d}] = GPR[{:d}] ({:#x}) - GPR[{:d}] ({:#x})\n", rd, rs, reg1, rt, reg2);
}

void EmotionEngine::op_bgtz()
{
    int32_t imm = (int16_t)instr.i_type.immediate;
    uint16_t rs = instr.i_type.rs;

    int32_t offset = imm << 2;
    int64_t reg = gpr[rs].dword[0];
    if (reg > 0)
        pc += offset - 4;

    log("BGTZ: IF GPR[{:d}] ({:#x}) > 0 THEN PC += {:#x}\n", rs, gpr[rs].dword[0], offset);
}

void EmotionEngine::op_movn()
{
    uint16_t rd = instr.r_type.rd;
    uint16_t rs = instr.r_type.rs;
    uint16_t rt = instr.r_type.rt;

    if (gpr[rt].dword[0] != 0) gpr[rd].dword[0] = gpr[rs].dword[0];

    log("MOVN: IF GPR[{:d}] ({:#x}) != 0 THEN GPR[{:d}] = GPR[{:d}] ({:#x})\n", rt, gpr[rt].dword[0], rd, rs, gpr[rs].dword[0]);
}

void EmotionEngine::op_slt()
{
    uint16_t rd = instr.r_type.rd;
    uint16_t rs = instr.r_type.rs;
    uint16_t rt = instr.r_type.rt;

    int64_t reg1 = gpr[rs].dword[0];
    int64_t reg2 = gpr[rt].dword[0];
    gpr[rd].dword[0] = reg1 < reg2;

    log("SLT: GPR[{:d}] = GPR[{:d}] ({:#x}) < GPR[{:d}] ({:#x})\n", rd, rs, reg1, rt, reg2);
}

void EmotionEngine::op_and()
{
    uint16_t rd = instr.r_type.rd;
    uint16_t rs = instr.r_type.rs;
    uint16_t rt = instr.r_type.rt;

    gpr[rd].dword[0] = gpr[rs].dword[0] & gpr[rt].dword[0];

    log("AND: GPR[{:d}] = GPR[{:d}] ({:#x}) & GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs].dword[0], rt, gpr[rt].dword[0]);
}

void EmotionEngine::op_srl()
{
    uint16_t sa = instr.r_type.sa;
    uint16_t rd = instr.r_type.rd;
    uint16_t rt = instr.r_type.rt;

    gpr[rd].dword[0] = (int32_t)(gpr[rt].word[0] >> sa);

    log("SRL: GPR[{:d}] = GPR[{:d}] ({:#x}) >> {:d}\n", rd, rt, gpr[rt].word[0], sa);
}

void EmotionEngine::op_dsll()
{
    uint16_t sa = instr.r_type.sa;
    uint16_t rd = instr.r_type.rd;
    uint16_t rt = instr.r_type.rt;

    log("DSLL: GPR[{:d}] = GPR[{:d}] ({:#x}) << {:d} + 32\n", rd, rt, gpr[rt].dword[0], sa);
    gpr[rd].dword[0] = gpr[rt].dword[0] << sa;
}

void EmotionEngine::op_bltz()
{
    int32_t imm = (int16_t)instr.i_type.immediate;
    uint16_t rs = instr.i_type.rs;

    int32_t offset = imm << 2;
    int64_t reg = gpr[rs].dword[0];
    if (reg < 0)
        pc += offset - 4;

    log("BLTZ: IF GPR[{:d}] ({:#x}) > 0 THEN PC += {:#x}\n", rs, gpr[rs].dword[0], offset);
}

void EmotionEngine::op_lhu()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t base = instr.i_type.rs;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    gpr[rt].dword[0] = read<uint16_t>(vaddr);

    log(BOLD, "LHU: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt].dword[0], vaddr, base, gpr[base].word[0], offset);
}

void EmotionEngine::op_dsll32()
{
    uint16_t sa = instr.r_type.sa;
    uint16_t rd = instr.r_type.rd;
    uint16_t rt = instr.r_type.rt;

    log("DSLL32: GPR[{:d}] = GPR[{:d}] ({:#x}) << {:d} + 32\n", rd, rt, gpr[rt].dword[0], sa);
    gpr[rd].dword[0] = gpr[rt].dword[0] << (sa + 32);
}

void EmotionEngine::op_dsra32()
{
    uint16_t sa = instr.r_type.sa;
    uint16_t rd = instr.r_type.rd;
    uint16_t rt = instr.r_type.rt;

    int64_t reg = gpr[rt].dword[0];
    gpr[rd].dword[0] = reg >> (sa + 32);
        
    log("DSRA32: GPR[{:d}] = GPR[{:d}] ({:#x}) >> {:d} + 32\n", rd, rt, reg, sa);
}

void EmotionEngine::op_lw()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t base = instr.i_type.rs;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    gpr[rt].dword[0] = (int32_t)read<uint32_t>(vaddr);

    log(BOLD, "LW: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt].dword[0], vaddr, base, gpr[base].word[0], offset);
}

void EmotionEngine::op_addiu()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    int16_t imm = (int16_t)instr.i_type.immediate;

    int32_t reg = gpr[rs].word[0];
    gpr[rt].dword[0] = reg + imm;

    log("ADDIU: GPR[{:d}] = GPR[{:d}] ({:#x}) + {:#x}\n", rt, rs, reg, imm);
}

void EmotionEngine::op_tlbwi()
{
    log("TLBWI\n");
}

void EmotionEngine::op_mtc0()
{
    uint16_t rt = (instr.value >> 16) & 0x1F;
    uint16_t rd = (instr.value >> 11) & 0x1F;

    cop0.regs[rd] = gpr[rt].word[0];
    
    log("MTC0: COP0[{:d}] = GPR[{:d}] ({:#x})\n", rd, rt, gpr[rt].word[0]); 
}

void EmotionEngine::op_mmi()
{
    switch(instr.r_type.sa)
    {
    case 0b10000: op_madd1(); break;
    default:
        log("[ERROR] Unimplemented MMI instruction: {:#05b}\n", (uint16_t)instr.r_type.sa);
		std::exit(1);
    }
}

void EmotionEngine::op_madd1()
{
    uint16_t rd = instr.r_type.rd;
    uint16_t rs = instr.r_type.rs;
    uint16_t rt = instr.r_type.rt;

    uint64_t lo = lo1 & 0xFFFFFFFF;
    uint64_t hi = hi1 & 0xFFFFFFFF;
    int64_t result = (hi << 32 | lo) + (int64_t)gpr[rs].word[0] * (int64_t)gpr[rt].word[0];
    
    lo1 = (int64_t)(int32_t)(result & 0xFFFFFFFF);
    hi1 = (int64_t)(int32_t)(result >> 32);
    gpr[rd].dword[0] = (int64_t)lo1;

    log("MADD1: GPR[{:d}] = LO1 = {:#x} and HI1 = {:#x}\n", lo1, hi1);
}

void EmotionEngine::op_jalr()
{
    uint16_t rs = instr.r_type.rs;
    uint16_t rd = instr.r_type.rd;

    gpr[rd].dword[0] = pc;
    pc = gpr[rs].word[0];

    log("JALR: Jumping to PC = GPR[{:d}] ({:#x}) with link address {:#x}\n", rs, pc, gpr[rd].dword[0]);
}

void EmotionEngine::op_sd()
{
    uint16_t base = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    uint64_t data = gpr[rt].dword[0];
    if ((vaddr & 0b111) != 0)
    {
        log("[ERROR] SD: Address {:#x} is not aligned\n", vaddr);
        std::exit(1); /* NOTE: SignalException (AddressError) */
    }
    else
        write<uint64_t>(vaddr, data);

    log(BOLD, "SD: Writing GPR[{:d}] ({:#x}) to address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, data, vaddr, base, gpr[base].word[0], offset);
}

void EmotionEngine::op_jal()
{
    uint32_t instr_index = instr.j_type.target;
    
    gpr[31].dword[0] = pc;
    pc = (pc & 0xF0000000) | (instr_index << 2);
    
    log("JAL: Jumping to PC = {:#x} with return link address {:#x}\n", pc, gpr[31].dword[0]);
}

void EmotionEngine::op_sra()
{
    uint16_t sa = instr.r_type.sa;
    uint16_t rd = instr.r_type.rd;
    uint16_t rt = instr.r_type.rt;

    int32_t reg = gpr[rt].word[0];
    gpr[rd].dword[0] = reg >> sa;

    log("SRA: GPR[{:d}] = GPR[{:d}] ({:#x}) >> {:d}\n", rd, rt, gpr[rt].word[0], sa);
}

void EmotionEngine::op_bgez()
{
    int32_t imm = (int16_t)instr.i_type.immediate;
    uint16_t rs = instr.i_type.rs;

    int32_t offset = imm << 2;
    int64_t reg = gpr[rs].dword[0];
    if (reg >= 0)
        pc += offset - 4;

    log("BGEZ: IF GPR[{:d}] ({:#x}) > 0 THEN PC += {:#x}\n", rs, reg, offset);
}

void EmotionEngine::op_addu()
{
    uint16_t rt = instr.r_type.rt;
    uint16_t rs = instr.r_type.rs;
    uint16_t rd = instr.r_type.rd;

    int32_t reg1 = gpr[rs].dword[0];
    int32_t reg2 = gpr[rt].dword[0];
    gpr[rd].dword[0] = reg1 + reg2;

    log("ADDU: GPR[{:d}] = GPR[{:d}] ({:#x}) + GPR[{:d}] ({:#x})\n", rd, rs, reg1, rt, reg2);
}

void EmotionEngine::op_daddu()
{
    uint16_t rt = instr.r_type.rt;
    uint16_t rs = instr.r_type.rs;
    uint16_t rd = instr.r_type.rd;

    int64_t reg1 = gpr[rs].dword[0];
    int64_t reg2 = gpr[rt].dword[0];
    gpr[rd].dword[0] = reg1 + reg2;

    log("DADDU: GPR[{:d}] = GPR[{:d}] ({:#x}) + GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs].dword[0], rt, gpr[rt].dword[0]);
}

void EmotionEngine::op_andi()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    uint64_t imm = instr.i_type.immediate;

    gpr[rt].dword[0] =  gpr[rs].dword[0] & imm;

    log("ANDI: GPR[{:d}] = GPR[{:d}] ({:#x}) & {:#x}\n", rt, rs, gpr[rs].dword[0], imm);
}

void EmotionEngine::op_beq()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    int32_t imm = (int16_t)instr.i_type.immediate;
    
    int32_t offset = imm << 2;
    if (gpr[rs].dword[0] == gpr[rt].dword[0])
        pc += offset - 4;
    
    log("BEQ: IF GPR[{:d}] ({:#x}) == GPR[{:d}] ({:#x}) THEN PC += {:#x}\n", rt, gpr[rt].dword[0], rs, gpr[rs].dword[0], offset);
}

void EmotionEngine::op_or()
{
    uint16_t rt = instr.r_type.rt;
    uint16_t rs = instr.r_type.rs;
    uint16_t rd = instr.r_type.rd;

    log("OR: GPR[{:d}] = GPR[{:d}] ({:#x}) | GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs].dword[0], rt, gpr[rt].dword[0]);

    gpr[rd].dword[0] = gpr[rs].dword[0] | gpr[rt].dword[0];
}

void EmotionEngine::op_mult()
{
    uint16_t rt = instr.r_type.rt;
    uint16_t rs = instr.r_type.rs;
    uint16_t rd = instr.r_type.rd;
    
    int64_t result = (int64_t)gpr[rs].word[0] * (int64_t)gpr[rt].word[0];
    gpr[rd].dword[0] = lo0 = (int32_t)(result & 0xFFFFFFFF);
    hi0 = (int32_t)(result >> 32);

    log("MULT: GPR[{:d}] = LO0 = {:#x} and HI0 = {:#x}\n", rd, lo0, hi0);
}

void EmotionEngine::op_divu()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;

    if (gpr[rt].word[0] != 0)
    {
        lo0 = (int64_t)(int32_t)(gpr[rs].word[0] / gpr[rt].word[0]);
        hi0 = (int64_t)(int32_t)(gpr[rs].word[0] % gpr[rt].word[0]);
        
        log("DIVU: LO0 = GPR[{:d}] ({:#x}) / GPR[{:d}] ({:#x})\n", rs, gpr[rs].word[0], rt, gpr[rt].word[0]);
    }
    else
    {
        log("[ERROR] DIVU: Division by zero!\n");
        std::abort();
    }
}

void EmotionEngine::op_beql()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    int32_t imm = (int16_t)instr.i_type.immediate;

    int32_t offset = imm << 2;
    if (gpr[rs].dword[0] == gpr[rt].dword[0])
        pc += offset - 4;
    else
        skip_branch_delay = true;

    log("BEQL: IF GPR[{:d}] ({:#x}) == GPR[{:d}] ({:#x}) THEN PC += {:#x}\n", rs, gpr[rs].dword[0], rt, gpr[rt].dword[0], offset);
}

void EmotionEngine::op_mflo()
{
    uint16_t rd = instr.r_type.rd;

    gpr[rd].dword[0] = lo0;

    log("MFLO: GPR[{:d}] = LO0 ({:#x})\n", rd, lo0);
}

void EmotionEngine::op_sltiu()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    uint64_t imm = (int16_t)instr.i_type.immediate;
    
    gpr[rt].dword[0] = gpr[rs].dword[0] < imm;

    log("SLTIU: GPR[{:d}] = GPR[{:d}] ({:#x}) < {:#x}\n", rt, rs, gpr[rs].dword[0], imm);
}

void EmotionEngine::op_bnel()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    int32_t imm = (int16_t)instr.i_type.immediate;

    int32_t offset = imm << 2;
    if (gpr[rs].dword[0] != gpr[rt].dword[0])
        pc += offset - 4;
    else
        skip_branch_delay = true;

    log("BNEL: IF GPR[{:d}] ({:#x}) != GPR[{:d}] ({:#x}) THEN PC += {:#x}\n", rs, gpr[rs].dword[0], rt, gpr[rt].dword[0], offset);
}

/* Template definitions. */
template uint32_t EmotionEngine::read<uint32_t>(uint32_t);
template uint64_t EmotionEngine::read<uint64_t>(uint32_t);
template uint8_t EmotionEngine::read<uint8_t>(uint32_t);
template uint16_t EmotionEngine::read<uint16_t>(uint32_t);
template void EmotionEngine::write<uint32_t>(uint32_t, uint32_t);
template void EmotionEngine::write<uint64_t>(uint32_t, uint64_t);
template void EmotionEngine::write<uint8_t>(uint32_t, uint8_t);