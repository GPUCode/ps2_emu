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
}

void EmotionEngine::fetch_instruction()
{
    /* Handle branch delay slots by prefetching the next one */
    instr.value = next_instr.value;
    next_instr = manager->read_memory<uint32_t>(pc);
    pc += 4;
    
    std::cout << "PC: " << std::hex << pc << " Instruction: " << instr.value << '\n';
    switch(instr.opcode)
    {
    case COP0_OPCODE: op_cop0(); break;
    case SPECIAL_OPCODE: op_special(); break;
    case 0b101011: op_sw(); break;
    case 0b001010: op_slti(); break;
    case 0b000101: op_bne(); break;
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
    gpr[data.rt].value = cop0.regs[data.action];
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
    uint8_t base = instr.i_type.rs;
    uint8_t rt = instr.i_type.rt;
    uint16_t offset = instr.i_type.immediate;

    uint32_t vaddr = (uint32_t)(int16_t)offset + gpr[base].doubleword[0];
    if (vaddr & 0b11 != 0)
        return; /* NOTE: SignalException (AddressError) */

    uint32_t data = gpr[rt].doubleword[0];
    manager->write_memory<uint32_t>(vaddr, data);
}

void EmotionEngine::op_sll()
{
    uint8_t rt = instr.r_type.rt;
    uint8_t rd = instr.r_type.rd;
    uint8_t sa = instr.r_type.sa;

    uint32_t result = gpr[rt].doubleword[0] << sa;
    gpr[rd].quadword[0] = (uint64_t)(int32_t)result;
}

void EmotionEngine::op_slti()
{
    uint8_t rs = instr.i_type.rs;
    uint8_t rt = instr.i_type.rt;
    uint16_t imm = instr.i_type.immediate;

    gpr[rt].quadword[0] = gpr[rs].quadword[0] < (uint32_t)(int16_t)imm; 
}

void EmotionEngine::op_bne()
{
    uint8_t rt = instr.i_type.rt;
    uint8_t rs = instr.i_type.rs;
    uint16_t imm = instr.i_type.immediate;

    uint32_t offset = (uint32_t)(int16_t)imm << 2;
    if (gpr[rs].quadword[0] != gpr[rt].quadword[0])
        pc += offset;
}