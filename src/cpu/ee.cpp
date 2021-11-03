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
    gpr[0].quadword = 0;
}

void EmotionEngine::fetch_instruction()
{
    /* Handle branch delay slots by prefetching the next one */
    instr.value = next_instr.value;
    next_instr = manager->read_memory<uint32_t>(pc);
    std::cout << "PC: " << std::hex << pc << ' ' << std::dec;

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
    default:
        std::cout << "Unimplemented opcode: " << std::bitset<6>(instr.opcode) << '\n';
        std::abort();
    }
}

void EmotionEngine::op_cop0()
{
    /*  The manual says that COP0 is only accessible in kernel mode
        There are exceptions but we will deal with those later.  */
    if (cop0.get_operating_mode() != OperatingMode::KERNEL_MODE)
        return;

    COP0Instr cop0_instr{instr.value};
	switch (cop0_instr.input)
	{
	case COP0_MF0:
		if (cop0_instr.input == 0) 
			op_mfc0();
		break;
	
	default:
		std::cout << "Unimplemented COP0 instruction: " << std::bitset<5>(cop0_instr.type) << '\n';
		std::exit(1);
	}
}

void EmotionEngine::op_mfc0()
{
	COP0Instr data{instr.value};
    gpr[data.rt].quadword = cop0.regs[data.action];

    std::cout << "MFC0: GPR[" << data.rt << "] = COP0_REG[" << data.action << "] (" << cop0.regs[data.action] << ")\n";
}

void EmotionEngine::op_special()
{
    switch(instr.r_type.funct)
    {
    case 0b000000: op_sll(); break;
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
    std::cout << "SW: Writing data: " << data << " to address: 0x" << vaddr << '\n';
    manager->write_memory<uint32_t>(vaddr, data);
}

void EmotionEngine::op_sll()
{
    uint16_t rt = instr.r_type.rt;
    uint16_t rd = instr.r_type.rd;
    uint32_t sa = instr.r_type.sa;

    uint32_t result = gpr[rt].word[0] << sa;
    gpr[rd].doubleword[0] = (uint64_t)(int32_t)result;

    if (instr.value == 0)
        std::cout << "NOP\n";
    else
        std::cout << "SLL: GPR[" << rd << "] = GPR[" << rt << "] (" << gpr[rd].doubleword[0] << ") << " << sa << '\n';
}

void EmotionEngine::op_slti()
{
    uint16_t rs = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int64_t imm = (int64_t)(int16_t)instr.i_type.immediate;

    gpr[rt].doubleword[0] = (int64_t)gpr[rs].doubleword[0] < imm;
    std::cout << "SLTI: GPR[" << rt << "] = GPR[" << rs << "] (" << (int64_t)gpr[rs].doubleword[0] << ") < " << imm << '\n';
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
    int16_t imm = (int16_t)instr.i_type.immediate;

    gpr[rt].doubleword[0] = gpr[rs].doubleword[0] | imm;
    std::cout << "ORI: GPR[" << rt << "] = GPR[" << rs << "] (" << gpr[rs].doubleword[0] << ") | " << imm << '\n';
}

void EmotionEngine::op_addi()
{
    uint16_t rs = instr.i_type.rs;
    uint16_t rt = instr.i_type.rt;
    int16_t imm = (int16_t)instr.i_type.immediate;

    /* TODO: Overflow detection */
    gpr[rt].doubleword[0] = gpr[rs].doubleword[0] + imm;
    std::cout << "ADDI: GPR[" << rt << "] = GPR[" << rs << " (" << gpr[rs].doubleword[0] << ") + " << imm << '\n';
}

void EmotionEngine::op_lq()
{
    uint16_t rt = instr.i_type.rt;
    uint16_t base = instr.i_type.rs;
    int16_t imm = (int16_t)instr.i_type.immediate;

    uint32_t vaddr = (gpr[base].doubleword[0] + imm) | 0b0000;
    gpr[rt].quadword = manager->read_memory<uint128_t>(vaddr);
}