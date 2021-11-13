#include <cpu/ee.hpp>
#include <common/manager.hpp>
#include <bitset>
#include <iostream>

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
    instr.value = 0;
    next_instr.value = 0;

    /* Set this to zero */
    gpr[0].doubleword[0] = gpr[0].doubleword[1] = 0;

    /* Set EE pRId */
    cop0.prid = 0x00002E20;
}

void EmotionEngine::fetch_instruction()
{
    /* Handle branch delay slots by prefetching the next one */
    instr.value = next_instr.value;
    next_instr = read<uint32_t>(pc);
    std::cout << "PC: " << std::hex << pc - 4 << " Instruction: 0x" << instr.value << ' ' << std::dec;

    pc += 4;
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
    default:
        std::cout << "Unimplemented opcode: " << std::bitset<6>(instr.opcode) << '\n';
        std::abort();
    }
}

template <typename T>
T EmotionEngine::read(uint32_t addr)
{
    if (addr >= 0x70000000 && addr < 0x70004000) /* Read from scratchpad */
        return *(uint32_t*)&scratchpad[addr & 0x3FFC];
    else
        return manager->read<T>(addr);
}

template <typename T>
void EmotionEngine::write(uint32_t addr, T data)
{
    if (addr >= 0x70000000 && addr < 0x70004000)
        *(uint32_t*)&scratchpad[addr & 0x3FFC] = data;
    else
        manager->write<T>(addr, data);
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
            std::cout << "Unimplemented COP0 MF0 instruction: " << std::bitset<3>(instr.value & 0x7) << '\n';
            std::exit(1);
        }
        break;
    case COP0_MT0:
        switch (instr.value & 0x7)
        {
        case 0b000: op_mtc0(); break;
        default:
            std::cout << "Unimplemented COP0 MT0 instruction: " << std::bitset<3>(instr.value & 0x7) << '\n';
            std::exit(1);
        }
        break;
	case COP0_TLB:
        if ((instr.value & 0x3F) == 0b000010) op_tlbwi();
        break;
	default:
		std::cout << "Unimplemented COP0 instruction: " << std::bitset<5>(type) << '\n';
		std::exit(1);
	}
}

void EmotionEngine::op_mfc0()
{
    uint16_t rd = (instr.value >> 11) & 0x1F;
    uint16_t rt = (instr.value >> 16) & 0x1F;

    gpr[rt].doubleword[0] = cop0.regs[rd];

    std::cout << "MFC0: GPR[" << rt << "] = COP0_REG[" << rd << "] (" << cop0.regs[rd] << ")\n";
}

void EmotionEngine::op_special()
{
    switch(instr.r_type.funct)
    {
    case 0b000000: op_sll(); break;
    case 0b001000: op_jr(); break;
    case 0b001111: std::cout << "SYNC\n"; break;
    case 0b001001: op_jalr(); break;
    case 0b000011: op_sra(); break;
    case 0b100001: op_addu(); break;
    default:
        std::cout << "Unimplemented SPECIAL instruction: " << std::bitset<6>(instr.r_type.funct) << '\n';
		std::exit(1);
    }
}

void EmotionEngine::op_regimm()
{
    uint16_t type = (instr.value >> 16) & 0x1F;
    switch(type)
    {
    case 0b00001: op_bgez(); break;
    default:
        std::cout << "Unimplemented REGIMM instruction: " << std::bitset<5>(type) << '\n';
		std::exit(1);
    }
}

void EmotionEngine::op_sw()
{
    uint16_t base = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    if (vaddr & 0b11 != 0)
    {
        std::cout << "SW: Address 0x" << std::hex << vaddr << " is not aligned\n";
        std::exit(1); /* NOTE: SignalException (AddressError) */
    }

    uint32_t data = gpr[rt].word[0];
    std::cout << "SW: Writing GPR[" << rt << std::hex << "] (0x" << data << ") to address: 0x" << vaddr;
    std::cout << " = GPR[" << base << "] (" << gpr[base].word[0] << ") + " << std::dec << offset << '\n';
    write<uint32_t>(vaddr, data);
}

void EmotionEngine::op_sll()
{
    uint16_t rt = instr.r_type.rt;
    uint16_t rd = instr.r_type.rd;
    uint32_t sa = instr.r_type.sa;

    if (instr.value == 0)
        std::cout << "NOP\n";
    else
        std::cout << "SLL: GPR[" << rd << "] = GPR[" << rt << "] (0x" << std::hex << gpr[rd].doubleword[0] << ") << " << sa << '\n';

    uint32_t result = gpr[rt].word[0] << sa;
    gpr[rd].doubleword[0] = (uint64_t)(int32_t)result;
}

void EmotionEngine::op_slti()
{
    uint16_t rs = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int64_t imm = (int64_t)(int16_t)instr.i_type.immediate;

    std::cout << "SLTI: GPR[" << rt << "] = GPR[" << rs << "] (" << (int64_t)gpr[rs].doubleword[0] << ") < " << imm << '\n';
    gpr[rt].doubleword[0] = (int64_t)gpr[rs].doubleword[0] < imm;
}

void EmotionEngine::op_bne()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    int32_t imm = (int16_t)instr.i_type.immediate;

    int32_t offset = imm << 2;
    std::cout << "BNE: || GPR[" << rt << "] (" << gpr[rt].doubleword[0] << ") != GPR[" << rs << "] (" << gpr[rs].doubleword[0];
    std::cout << ")|| -> OFFSET: " << offset << '\n'; 
    if (gpr[rs].doubleword[0] != gpr[rt].doubleword[0])
        pc += offset;
}

void EmotionEngine::op_ori()
{
    uint16_t rs = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    uint64_t imm = (uint64_t)instr.i_type.immediate;

    std::cout << "ORI: GPR[" << rt << "] = GPR[" << rs << "] (0x" << std::hex << gpr[rs].doubleword[0] << ") | 0x" << imm << std::dec << '\n';
    gpr[rt].doubleword[0] = gpr[rs].doubleword[0] | imm;
}

void EmotionEngine::op_addi()
{
    uint16_t rs = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int16_t imm = (int16_t)instr.i_type.immediate;

    /* TODO: Overflow detection */
    std::cout << "ADDI: GPR[" << rt << "] = GPR[" << rs << " (0x" << std::hex << gpr[rs].doubleword[0] << ") + 0x" << imm << std::dec << '\n';
    gpr[rt].doubleword[0] = gpr[rs].doubleword[0] + imm;
}

void EmotionEngine::op_lq()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t base = instr.i_type.rs;
    int16_t imm = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = (gpr[base].doubleword[0] + imm) | 0b0000;
    gpr[rt].doubleword[0] = read<uint64_t>(vaddr);
    gpr[rt].doubleword[1] = read<uint64_t>(vaddr + 8);

    std::cout << "LQ: GPR[" << rt << "] = 0x" << std::hex << gpr[rt].doubleword[0] << " from address 0x" << vaddr  << '\n';
}

void EmotionEngine::op_lui()
{
    uint16_t rt = instr.i_type.rt;
    uint32_t imm = instr.i_type.immediate;

    std::cout << "LUI: GPR[" << rt << "] = 0x" << std::hex << imm << std::dec << '\n';
    gpr[rt].doubleword[0] = (int64_t)(int32_t)(imm << 16);
}

void EmotionEngine::op_jr()
{
    uint16_t rs = instr.i_type.rs;
    pc = gpr[rs].word[0];
    std::cout << "JR: Jumped to GPR[" << rs << "] = 0x" << std::hex << pc << std::dec << '\n';
}

void EmotionEngine::op_addiu()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t rs = instr.i_type.rs;
    int16_t imm = (int16_t)instr.i_type.immediate;

    int64_t temp = gpr[rs].doubleword[0] + (int64_t)imm;
    gpr[rt].doubleword[0] = (int64_t)(temp & 0xFFFFFFFF);
    std::cout << "ADDIU: GPR[" << rt << "] = GPR[" << rs << "] (0x" << std::hex << gpr[rs].doubleword[0] << ") + 0x" << imm << std::dec << '\n';
}

void EmotionEngine::op_tlbwi()
{
    std::cout << "TLBWI: \n";
}

void EmotionEngine::op_mtc0()
{
    uint16_t rt = (instr.value >> 16) & 0x1F;
    uint16_t rd = (instr.value >> 11) & 0x1F;

    cop0.regs[rd] = gpr[rt].word[0];
    std::cout << "MTC0: COP0_REG[" << rd << "] = GPR[" << rt << std::hex << "] (0x" << gpr[rt].word[0] << std::dec << ")\n"; 
}

void EmotionEngine::op_mmi()
{
    switch(instr.r_type.sa)
    {
    case 0b10000: op_madd1(); break;
    default:
        std::cout << "Unimplemented MMI instruction: " << std::bitset<5>(instr.r_type.sa) << '\n';
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
    gpr[rd].doubleword[0] = (int64_t)lo1;

    std::cout << "MADD1: GPR[" << rd << "] = LO1 = 0x" << std::hex << (int64_t)lo1 << "and HI1 = 0x" << hi1 << std::dec << '\n';
}

void EmotionEngine::op_jalr()
{
    uint16_t rs = instr.r_type.rs;
    uint16_t rd = instr.r_type.rd;

    gpr[rd].doubleword[0] = pc + 4; /* Normally this should be PC + 8 but we compensate for the prefetching with -4 */
    pc = gpr[rs].word[0];

    std::cout << "JALR: Jumped to PC = GPR[" << rs << "] (0x" << std::hex << pc << ") with link address 0x" << gpr[rd].doubleword[0] << std::dec << '\n';
}

void EmotionEngine::op_sd()
{
    uint16_t base = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int16_t offset = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = offset + gpr[base].word[0];
    if (vaddr & 0b111 != 0)
    {
        std::cout << "SD: Address 0x" << std::hex << vaddr << " is not aligned\n";
        std::exit(1); /* NOTE: SignalException (AddressError) */
    }

    uint64_t data = gpr[rt].doubleword[0];
    std::cout << "SD: Writing GPR[" << rt << std::hex << "] (0x" << data << ") to address: 0x" << vaddr;
    std::cout << " = GPR[" << base << "] (" << gpr[base].word[0] << ") + " << std::dec << offset << '\n';
    write<uint64_t>(vaddr, data);
}

void EmotionEngine::op_jal()
{
    uint32_t instr_index = instr.j_type.target;
    
    gpr[31].doubleword[0] = pc + 4;
    pc = (pc & 0xF0000000) + (instr_index << 2);
    std::cout << "JAL: Jumped to PC = 0x" << std::hex << pc << std::dec << '\n';
}

void EmotionEngine::op_sra()
{
    uint16_t sa = instr.r_type.sa;
    uint16_t rd = instr.r_type.rd;
    uint16_t rt = instr.r_type.rt;

    static uint32_t mask[2] = { 0x0, 0xFFFFFFFF };
    uint32_t result = (mask[gpr[rt].word[0] >> 31] << sa) | (gpr[rt].word[0] >> sa);

    gpr[rd].doubleword[0] = (int64_t)(gpr[rt].word[0] >> sa);
    std::cout << "SRA: GPR[" << rd << "] (0x" << std::hex << gpr[rd].doubleword[0] << ") = GPR[" << rt << "] (0x";
    std::cout << gpr[rt].word[0] << ") >> " << std::dec << sa << '\n';
}

void EmotionEngine::op_bgez()
{
    uint32_t imm = instr.i_type.immediate;
    uint16_t rs = instr.i_type.rs;

    int32_t offset = (int32_t)(imm << 2);
    if (gpr[rs].doubleword[0] >= 0)
        pc += offset;

    std::cout << "BGEZ: IF GPR[" << rs << "] (0x" << std::hex << gpr[rs].word[0] << ") > 0 THEN PC += " << offset << std::dec << '\n';
}

void EmotionEngine::op_addu()
{
    uint16_t rt = instr.r_type.rt;
    uint16_t rs = instr.r_type.rs;
    uint16_t rd = instr.r_type.rd;

    uint64_t result = gpr[rs].doubleword[0] + gpr[rt].doubleword[0];
    gpr[rd].doubleword[0] = (int64_t)(int32_t)(result & 0xFFFFFFFF);

    std::cout << "ADDU: GPR[" << rd << "] (0x" << std::hex << gpr[rd].doubleword[0] << ") = GPR[" << rs << "] (0x" << gpr[rs].doubleword[0] << ")";
    std::cout << std::dec << " + GPR[" << rt << "] (0x" << std::hex << gpr[rt].doubleword[0] << std::dec << ")\n";
}

/* Template definitions. */
template uint32_t EmotionEngine::read<uint32_t>(uint32_t);
template uint64_t EmotionEngine::read<uint64_t>(uint32_t);
template void EmotionEngine::write<uint32_t>(uint32_t, uint32_t);
template void EmotionEngine::write<uint64_t>(uint32_t, uint64_t);