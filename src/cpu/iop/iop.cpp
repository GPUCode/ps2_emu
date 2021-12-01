#include <cpu/iop/iop.hpp>
#include <cstring>
#include <fmt/color.h>

#ifndef NDEBUG
#define log(x) (void)0
#else
constexpr fmt::v8::text_style BOLD = fg(fmt::color::forest_green) | fmt::emphasis::bold;
#define log(...) fmt::print(disassembly, __VA_ARGS__)
#endif

namespace iop
{
    IOProcessor::IOProcessor(ComponentManager* manager) :
        manager(manager)
    {
        /* Set PRID Processor ID*/
        cop0.PRId = 0x1f;

        /* Open output log */
        disassembly = std::fopen("disassembly_iop.log", "w");

        /* Reset CPU state. */
        reset();
    }

    IOProcessor::~IOProcessor()
    {
        std::fclose(disassembly);
    }

    void IOProcessor::reset()
    {
        /* Reset registers and PC. */
        pc = 0xbfc00000;
        hi = 0; lo = 0;

        /* Add first instruction into the pipeline */
        direct_jump();

        /* Clear general purpose registers. */
        std::memset(gpr, 0, 32 * sizeof(uint32_t));
    }

    void IOProcessor::tick()
    {
        /* Fetch next instruction. */
        fetch();

        /* Execute it. */
        switch (instr.opcode)
        {
        case 0b000000: op_special(); break;
        case 0b000001: op_bcond(); break;
        case 0b001111: op_lui(); break;
        case 0b001101: op_ori(); break;
        case 0b101011: op_sw(); break;
        case 0b001001: op_addiu(); break;
        case 0b001000: op_addi(); break;
        case 0b000010: op_j(); break;
        case 0b010000: op_cop0(); break;
        case 0b100011: op_lw(); break;
        case 0b000101: op_bne(); break;
        case 0b101001: op_sh(); break;
        case 0b000011: op_jal(); break;
        case 0b001100: op_andi(); break;
        case 0b101000: op_sb(); break;
        case 0b100000: op_lb(); break;
        case 0b000100: op_beq(); break;
        case 0b000111: op_bgtz(); break;
        case 0b000110: op_blez(); break;
        case 0b100100: op_lbu(); break;
        case 0b001010: op_slti(); break;
        case 0b001011: op_sltiu(); break;
        case 0b100101: op_lhu(); break;
        case 0b100001: op_lh(); break;
        case 0b001110: op_xori(); break;
        case 0b101110: op_swr(); break;
        case 0b101010: op_swl(); break;
        case 0b100010: op_lwl(); break;
        case 0b100110: op_lwr(); break;
        default: exception(Exception::IllegalInstr);
        }

        /* Apply pending load delays. */
        handle_load_delay();
    }

    void IOProcessor::op_special()
    {
        switch (instr.r_type.funct)
        {
        case 0b000000: op_sll(); break;
        case 0b100101: op_or(); break;
        case 0b101011: op_sltu(); break;
        case 0b100001: op_addu(); break;
        case 0b001000: op_jr(); break;
        case 0b100100: op_and(); break;
        case 0b100000: op_add(); break;
        case 0b001001: op_jalr(); break;
        case 0b100011: op_subu(); break;
        case 0b000011: op_sra(); break;
        case 0b011010: op_div(); break;
        case 0b010010: op_mflo(); break;
        case 0b000010: op_srl(); break;
        case 0b011011: op_divu(); break;
        case 0b010000: op_mfhi(); break;
        case 0b101010: op_slt(); break;
        case 0b001100: op_syscall(); break;
        case 0b010011: op_mtlo(); break;
        case 0b010001: op_mthi(); break;
        case 0b000100: op_sllv(); break;
        case 0b100111: op_nor(); break;
        case 0b000111: op_srav(); break;
        case 0b000110: op_srlv(); break;
        case 0b011001: op_multu(); break;
        case 0b100110: op_xor(); break;
        case 0b001101: op_break(); break;
        case 0b011000: op_mult(); break;
        case 0b100010: op_sub(); break;
        default: exception(Exception::IllegalInstr);
        }
    }

    void IOProcessor::op_cop0()
    {
        switch (instr.i_type.rs)
        {
        case 0b00000: op_mfc0(); break;
        case 0b00100: op_mtc0(); break;
        case 0b10000: op_rfe(); break;
        default: exception(Exception::IllegalInstr);
        }
    }

    void IOProcessor::fetch()
    {
        /* Fetch instruction from main RAM. */
        instr = next_instr;

        /* Check aligment errors. */
        if (pc & 0x3) [[unlikely]]
        {
            cop0.BadA = pc;
            exception(Exception::ReadError);
            return;
        }

        /* Update PC. */
        direct_jump();

        log("PC: {:#x} instruction: {:#x} ", instr.pc, instr.value);
    }

    void IOProcessor::direct_jump()
    {
        next_instr = {};
        next_instr.value = read<uint32_t>(pc);
        next_instr.pc = pc;
        pc += 4;
    }

    void IOProcessor::exception(Exception cause, uint32_t cop)
    {
        fmt::print("[IOP] Exception of type {:d}\n", (int)cause);

        uint32_t mode = cop0.sr.value & 0x3F;
        cop0.sr.value &= ~(uint32_t)0x3F;
        cop0.sr.value |= (mode << 2) & 0x3F;

        uint32_t copy = cop0.cause.value & 0xff00;
        cop0.cause.exc_code = (uint32_t)cause;
        cop0.cause.CE = cop;
    
        bool is_delay_slot = instr.is_delay_slot;
        bool branch_taken = instr.branch_taken;
        if (cause == Exception::Interrupt) 
        {
            cop0.epc = next_instr.pc;

            /* Hack: related to the delay of the ex interrupt*/
            is_delay_slot = next_instr.is_delay_slot;
            branch_taken = next_instr.branch_taken;
        }
        else 
        {
            cop0.epc = instr.pc;
        }

        if (is_delay_slot) 
        {
            cop0.epc -= 4;

            cop0.cause.BD = true;
            cop0.TAR = next_instr.pc;

            if (branch_taken) 
            {
                cop0.cause.BT = true;
            }
        }

        /* Select exception address. */
        pc = exception_addr[cop0.sr.BEV];

        /* Bypass the pipeline and insert our instruction. */
        direct_jump();
    }

    void IOProcessor::handle_load_delay()
    {
        if (delayed_memory_load.reg != memory_load.reg) 
        {
            gpr[memory_load.reg] = memory_load.value;
        }
    
        memory_load = delayed_memory_load;
        delayed_memory_load.reg = 0;

        gpr[write_back.reg] = write_back.value;
        write_back.reg = 0;
        gpr[0] = 0;
    }

    void IOProcessor::op_bcond()
    {
        uint32_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;

        next_instr.is_delay_slot = true;

        bool should_link = (rt & 0x1E) == 0x10;
        bool should_branch = (int)(gpr[rs] ^ (rt << 31)) < 0;

        if (should_link) gpr[31] = instr.pc + 8;
        if (should_branch) branch();
    }

    void IOProcessor::op_swr()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        int16_t imm = (int16_t)instr.i_type.immediate;

        uint32_t addr = gpr[rs] + imm;
        uint32_t aligned_addr = addr & 0xFFFFFFFC;
        uint32_t aligned_load = read<uint32_t>(aligned_addr);

        uint32_t value;
        switch (addr & 0x3)
        {
        case 0:
            value = gpr[rt];
            break;
        case 1:
            value = (aligned_load & 0x000000FF) | (gpr[rt] << 8);
            break;
        case 2:
            value = (aligned_load & 0x0000FFFF) | (gpr[rt] << 16);
            break;
        case 3:
            value = (aligned_load & 0x00FFFFFF) | (gpr[rt] << 24);
            break;
        }

        write<uint32_t>(aligned_addr, value);
    }

    void IOProcessor::op_swl()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        int16_t imm = (int16_t)instr.i_type.immediate;

        uint32_t addr = gpr[rs] + imm;
        uint32_t aligned_addr = addr & 0xFFFFFFFC;
        uint32_t aligned_load = read<uint32_t>(aligned_addr);

        uint32_t value;
        switch (addr & 0x3)
        {
        case 0:
            value = (aligned_load & 0xFFFFFF00) | (gpr[rt] >> 24);
            break;
        case 1:
            value = (aligned_load & 0xFFFF0000) | (gpr[rt] >> 16);
            break;
        case 2:
            value = (aligned_load & 0xFF000000) | (gpr[rt] >> 8);
            break;
        case 3:
            value = gpr[rt]; break;
        }

        write<uint32_t>(aligned_addr, value);
    }

    void IOProcessor::op_lwr()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        int16_t imm = (int16_t)instr.i_type.immediate;

        uint32_t addr = gpr[rs] + imm;
        uint32_t aligned_addr = addr & 0xFFFFFFFC;
        uint32_t aligned_load = read<uint32_t>(aligned_addr);

        uint32_t value = 0;
        uint32_t LRValue = gpr[rt];

        if (rt == memory_load.reg)
        {
            LRValue = memory_load.value;
        }

        switch (addr & 0x3)
        {
        case 0:
            value = aligned_load;
            break;
        case 1:
            value = (LRValue & 0xFF000000) | (aligned_load >> 8);
            break;
        case 2:
            value = (LRValue & 0xFFFF0000) | (aligned_load >> 16);
            break;
        case 3:
            value = (LRValue & 0xFFFFFF00) | (aligned_load >> 24);
            break;
        }

        load(rt, value);
    }

    void IOProcessor::op_lwl()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        int16_t imm = (int16_t)instr.i_type.immediate;

        uint32_t addr = gpr[rs] + imm;
        uint32_t aligned_addr = addr & 0xFFFFFFFC;
        uint32_t aligned_load = read<uint32_t>(aligned_addr);

        uint32_t value = 0;
        uint32_t LRValue = gpr[rt];

        if (rt == memory_load.reg)
        {
            LRValue = memory_load.value;
        }

        switch (addr & 0x3)
        {
        case 0:
            value = (LRValue & 0x00FFFFFF) | (aligned_load << 24);
            break;
        case 1:
            value = (LRValue & 0x0000FFFF) | (aligned_load << 16);
            break;
        case 2:
            value = (LRValue & 0x000000FF) | (aligned_load << 8);
            break;
        case 3:
            value = aligned_load;
            break;
        }

        load(rt, value);
    }

    void IOProcessor::op_xori()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        uint32_t imm = instr.i_type.immediate;

        set_reg(rt, gpr[rs] ^ imm);

        log("XORI: GPR[{:d}] = GPR[{:d}] ({:#x}) ^ {:#x}\n", rt, rs, gpr[rs], imm);
    }

    void IOProcessor::op_sub()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        uint32_t sub = gpr[rs] - gpr[rt];
        set_reg(rd, sub);

        log("SUB: GPR[{:d}] = GPR[{:d}] ({:#x}) - GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_mult()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;

        int64_t result = (int64_t)(int32_t)gpr[rs] * (int64_t)(int32_t)gpr[rt];

        hi = result >> 32;
        lo = result;

        log("MULT: GPR[{:d}] ({:#x}) * GPR[{:d}] ({:#x}) = {:#x} STORED IN LO = {:#x} and HI = {:#x}\n", rs, gpr[rs], rt, gpr[rt], result, lo, hi);
    }

    void IOProcessor::op_break()
    {
        exception(Exception::Break);

        log("BREAK\n");
    }

    void IOProcessor::op_xor()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        set_reg(rd, gpr[rs] ^ gpr[rt]);

        log("XOR: GPR[{:d}] = GPR[{:d}] ({:#x}) ^ GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_multu()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;

        uint64_t result = (uint64_t)gpr[rs] * (uint64_t)gpr[rt];

        hi = result >> 32;
        lo = result;

        log("MULTU: GPR[{:d}] ({:#x}) * GPR[{:d}] ({:#x}) = {:#x} STORED IN LO = {:#x} and HI = {:#x}\n", rs, gpr[rs], rt, gpr[rt], result, lo, hi);
    }

    void IOProcessor::op_srlv()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        uint16_t sa = gpr[rs] & 0x1F;
        set_reg(rd, gpr[rt] >> sa);

        log("SRLV: GPR[{:d}] = GPR[{:d}] ({:#x}) >> {:d}\n", rd, rt, gpr[rt], sa);
    }

    void IOProcessor::op_srav()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        uint16_t sa = gpr[rs] & 0x1F;
        int32_t reg = (int32_t)gpr[rt];
        set_reg(rd, reg >> sa);

        log("SRAV: GPR[{:d}] = GPR[{:d}] ({:#x}) >> {:d}\n", rd, rt, reg, sa);
    }

    void IOProcessor::op_nor()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        uint32_t result = ~(gpr[rs] | gpr[rt]);
        set_reg(rd, result);

        log("NOR: GPR[{:d}] = NOT {:#x} = GPR[{:d}] ({:#x}) | GPR[{:d}] ({:#x})\n", rd, result, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_lh()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t base = instr.i_type.rs;
        int16_t offset = (int16_t)instr.i_type.immediate;

        uint32_t vaddr = gpr[base] + offset;
        if (!cop0.sr.IsC)
        {
            if (vaddr & 0x1) [[unlikely]]
            {
                cop0.BadA = vaddr;
                exception(Exception::ReadError);
            }
            else
            {
                uint32_t value = (int16_t)read<uint16_t>(vaddr);
                load(rt, value);
            }
        }

        log("LH: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt], vaddr, base, gpr[base], offset);
    }

    void IOProcessor::op_sllv()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        uint16_t sa = gpr[rs] & 0x1F;
        set_reg(rd, gpr[rt] << sa);

        log("SLLV: GPR[{:d}] = GPR[{:d}] ({:#x}) << GPR[{:d}] ({:d})\n", rd, rt, gpr[rt], rs, sa);
    }

    void IOProcessor::op_lhu()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t base = instr.i_type.rs;
        int16_t offset = (int16_t)instr.i_type.immediate;

        uint32_t vaddr = gpr[base] + offset;
        if (!cop0.sr.IsC) 
        {
            if (vaddr & 0x1) [[unlikely]]
            {
                cop0.BadA = vaddr;
                exception(Exception::ReadError, 0);
            }
            else
            {
                uint32_t value = read<uint16_t>(vaddr);
                load(rt, value);
            }
        }

        log("LHU: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt], vaddr, base, gpr[base], offset);
    }

    void IOProcessor::op_rfe()
    {
        uint32_t mode = cop0.sr.value & 0x3F;

        /* Shift kernel/user mode bits back. */
        cop0.sr.value &= ~(uint32_t)0xF;
        cop0.sr.value |= mode >> 2;

        log("RFE\n");
    }

    void IOProcessor::op_mthi()
    {
        uint16_t rs = instr.r_type.rs;
        hi = gpr[rs];

        log("MTHI: HI = GPR[{:d}] ({:#x})\n", rs, gpr[rs]);
    }

    void IOProcessor::op_mtlo()
    {
        uint16_t rs = instr.r_type.rs;
        lo = gpr[rs];

        log("MTLO: LO = GPR[{:d}] ({:#x})\n", rs, gpr[rs]);
    }

    void IOProcessor::op_syscall()
    {
        exception(Exception::Syscall, 0);

        log("SYSCALL\n");
    }

    void IOProcessor::op_slt()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        int32_t reg1 = (int32_t)gpr[rs];
        int32_t reg2 = (int32_t)gpr[rt];
        bool condition = reg1 < reg2;
        set_reg(rd, condition);

        log("SLT: GPR[{:d}] = GPR[{:d}] ({:#x}) < GPR[{:d}] ({:#x})\n", rd, rs, reg1, rt, reg2);
    }

    void IOProcessor::op_mfhi()
    {
        uint16_t rd = instr.r_type.rd;
        set_reg(rd, hi);

        log("MFHI: GPR[{:d}] = HI ({:#x})\n", rd, hi);
    }

    void IOProcessor::op_divu()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;

        uint32_t dividend = gpr[rs];
        uint32_t divisor = gpr[rt];
        if (divisor == 0) [[unlikely]]
        {
            hi = dividend;
            lo = 0xFFFFFFFF;
        }
        else 
        {
            hi = dividend % divisor;
            lo = dividend / divisor;
        }

        log("DIVU: GPR[{:d}] ({:#x}) / GPR[{:d}] ({:#x}) OUTPUT LO0 = {:#x} and HI0 = {:#x}\n", rs, gpr[rs], rt, gpr[rt], lo, hi);
    }

    void IOProcessor::op_sltiu()
    {
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;
        uint32_t imm = instr.i_type.immediate;

        bool condition = gpr[rs] < imm;
        set_reg(rt, condition);

        log("SLTIU: GPR[{:d}] = GPR[{:d}] ({:#x}) < {:#x}\n", rt, rs, gpr[rs], imm);
    }

    void IOProcessor::op_srl()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t sa = instr.r_type.sa;

        set_reg(rd, gpr[rt] >> sa);

        log("SRL: GPR[{:d}] = GPR[{:d}] ({:#x}) >> {:d}\n", rd, rt, gpr[rt], sa);
    }

    void IOProcessor::op_mflo()
    {
        uint16_t rd = instr.r_type.rd;
        set_reg(rd, lo);

        log("MFLO: GPR[{:d}] = LO ({:#x})\n", rd, lo);
    }

    void IOProcessor::op_div()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;

        int32_t dividend = (int32_t)gpr[rs];
        int32_t divisor = (int32_t)gpr[rt];
        if (divisor == 0) [[unlikely]]
        {
            hi = gpr[rs];
            lo = (dividend >= 0 ? 0xFFFFFFFF : 1);
        }
        else if (gpr[rs] == 0x80000000 && divisor == -1) [[unlikely]]
        {
            hi = 0;
            lo = 0x80000000;
        }
        else 
        {
            hi = (uint32_t)(dividend % divisor);
            lo = (uint32_t)(dividend / divisor);
        }

        log("DIV: LO = GPR[{:d}] ({:#x}) / GPR[{:d}] ({:#x})\n", rs, dividend, rt, divisor);
    }

    void IOProcessor::op_sra()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t sa = instr.r_type.sa;

        int32_t reg = (int32_t)gpr[rt];
        set_reg(rd, reg >> sa);

        log("SRA: GPR[{:d}] = GPR[{:d}] ({:#x}) >> {:d}\n", rd, rt, reg, sa);
    }

    void IOProcessor::op_subu()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        set_reg(rd, gpr[rs] - gpr[rt]);

        log("SUBU: GPR[{:d}] = GPR[{:d}] ({:#x}) - GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_slti()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        int16_t imm = (int16_t)instr.i_type.immediate;

        int32_t reg = (int32_t)gpr[rs];
        bool condition = reg < imm;
        set_reg(rt, condition);

        log("SLTI: GPR[{:d}] = GPR[{:d}] ({:#x}) < {:#x}\n", rt, rs, reg, imm);
    }

    void IOProcessor::branch()
    {
        int32_t imm = (int16_t)instr.i_type.immediate;

        next_instr.branch_taken = true;
        pc = next_instr.pc + (imm << 2);
    }

    void IOProcessor::op_jalr()
    {
        uint16_t rd = instr.r_type.rd;

        set_reg(rd, instr.pc + 8);
        op_jr();
    }

    void IOProcessor::op_lbu()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t base = instr.i_type.rs;
        int16_t offset = (int16_t)instr.i_type.immediate;

        uint32_t vaddr = gpr[base] + offset;
        if (!cop0.sr.IsC) 
        {
            uint32_t value = read<uint8_t>(vaddr);
            load(rt, value);
        }

        log("LBU: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt], vaddr, base, gpr[base], offset);
    }

    void IOProcessor::op_blez()
    {
        uint16_t rs = instr.i_type.rs;

        next_instr.is_delay_slot = true;
        int32_t reg = (int32_t)gpr[rs];
        if (reg <= 0)
        {
            branch();
        }

        log("BLEZ: IF GPR[{:d}] ({:#x}) <= 0 THEN PC = {:#x}\n", rs, gpr[rs], pc);
    }

    void IOProcessor::op_bgtz()
    {
        uint16_t rs = instr.i_type.rs;

        next_instr.is_delay_slot = true;
        int32_t reg = (int32_t)gpr[rs];
        if (reg > 0)
        {
            branch();
        }

        log("BGTZ: IF GPR[{:d}] ({:#x}) > 0 THEN PC = {:#x}\n", rs, gpr[rs], pc);
    }

    void IOProcessor::op_add()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        uint32_t add = gpr[rs] + gpr[rt];
        set_reg(rd, add);

        log("ADD: GPR[{:d}] = GPR[{:d}] ({:#x}) + GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_and()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;

        set_reg(rd, gpr[rs] & gpr[rt]);

        log("AND: GPR[{:d}] = GPR[{:d}] ({:#x}) & GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_mfc0()
    {
        uint16_t rd = instr.r_type.rd;
        uint16_t rt = instr.r_type.rt;

        load(rt, cop0.regs[rd]);

        log("MFC0: GPR[{:d}] = COP0_REG[{:d}] ({:#x})\n", rt, rd, cop0.regs[rd]);
    }

    void IOProcessor::op_beq()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;

        next_instr.is_delay_slot = true;
        if (gpr[rs] == gpr[rt]) 
        {
            branch();
        }

        log("BEQ: IF GPR[{:d}] ({:#x}) == GPR[{:d}] ({:#x}) THEN PC = {:#x}\n", rt, gpr[rt], rs, gpr[rs], pc);
    }

    void IOProcessor::op_lb()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t base = instr.i_type.rs;
        int16_t offset = (int16_t)instr.i_type.immediate;

        uint32_t vaddr = gpr[base] + offset;
        if (!cop0.sr.IsC) 
        {
            uint32_t value = (int8_t)read<uint8_t>(vaddr);
            load(rt, value);
        }

        log("LB: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt], vaddr, base, gpr[base], offset);

    }

    void IOProcessor::op_jr()
    {
        uint16_t rs = instr.i_type.rs;

        pc = gpr[rs];
        next_instr.is_delay_slot = true;
        next_instr.branch_taken = true;

        log("JR: Jumped to GPR[{:d}] = {:#x}\n", rs, pc);
    }

    void IOProcessor::op_sb()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t base = instr.i_type.rs;
        int16_t offset = (int16_t)instr.i_type.immediate;

        uint32_t vaddr = gpr[base] + offset;
        if (!cop0.sr.IsC)
        {
            write<uint8_t>(vaddr, gpr[rt]);
        }

        log("SB: Writing GPR[{:d}] ({:#x}) to address {:#x} = GPR[{:d}] ({:#x}) + {:d}\n", rt, gpr[rt], vaddr, base, gpr[base], offset);
    }

    void IOProcessor::op_andi()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        uint16_t imm = instr.i_type.immediate;

        set_reg(rt, gpr[rs] & imm);

        log("ANDI: GPR[{:d}] = GPR[{:d}] ({:#x}) & {:#x}\n", rt, rs, gpr[rs], imm);
    }

    void IOProcessor::op_jal()
    {
        set_reg(31, pc);
        op_j();
    }

    void IOProcessor::op_sh()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t base = instr.i_type.rs;
        int16_t offset = (int16_t)instr.i_type.immediate;

        uint32_t vaddr = gpr[base] + offset;
        if (!cop0.sr.IsC) 
        {
            if (vaddr & 0x1) [[unlikely]]
            {
                cop0.BadA = vaddr;
                exception(Exception::WriteError, 0);
            }
            else 
            {
                write<uint16_t>(vaddr, gpr[rt]);
            }
        }

        log("SH: Writing GPR[{:d}] ({:#x}) to address {:#x} = GPR[{:d}] ({:#x}) + {:d}\n", rt, gpr[rt], vaddr, base, gpr[base], offset);
    }

    void IOProcessor::op_addu()
    {
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;
        uint16_t rd = instr.r_type.rd;

        set_reg(rd, gpr[rs] + gpr[rt]);

        log("ADDU: GPR[{:d}] = GPR[{:d}] ({:#x}) + GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_sltu()
    {
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;
        uint16_t rd = instr.r_type.rd;

        bool condition = gpr[rs] < gpr[rt];
        set_reg(rd, condition);

        log("SLTU: GPR[{:d}] = GPR[{:d}] ({:#x}) < GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_lw()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t base = instr.i_type.rs;
        int16_t offset = (int16_t)instr.i_type.immediate;

        uint32_t vaddr = gpr[base] + offset;
        if (!cop0.sr.IsC) 
        {
            if (vaddr & 0x3) [[unlikely]]
            {
                cop0.BadA = vaddr;
                exception(Exception::ReadError, 0);
            }
            else 
            {
                uint32_t value = read<uint32_t>(vaddr);
                load(rt, value);
            }
        }

        log("LW: GPR[{:d}] = {:#x} from address {:#x} = GPR[{:d}] ({:#x}) + {:#x}\n", rt, gpr[rt], vaddr, base, gpr[base], offset);
    }

    void IOProcessor::op_addi()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        int16_t imm = (int16_t)instr.i_type.immediate;

        int32_t reg = (int32_t)gpr[rs];
        set_reg(rt, reg + imm);

        log("ADDI: GPR[{:d}] = GPR[{:d}] ({:#x}) + {:#x}\n", rt, rs, reg, imm);
    }

    void IOProcessor::op_bne()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;

        next_instr.is_delay_slot = true;
        if (gpr[rs] != gpr[rt])
        {
            branch();
        }

        log("BNE: IF GPR[{:d}] ({:#x}) != GPR[{:d}] ({:#x}) THEN PC = {:#x}\n", rt, gpr[rt], rs, gpr[rs], pc);
    }

    void IOProcessor::op_mtc0()
    {
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;
        uint16_t rd = instr.r_type.rd;

        cop0.regs[rd] = gpr[rt];

        log("MTC0: COP0[{:d}] = GPR[{:d}] ({:#x})\n", rd, rt, gpr[rt]);
    }

    void IOProcessor::op_or()
    {
        uint16_t rt = instr.r_type.rt;
        uint16_t rs = instr.r_type.rs;
        uint16_t rd = instr.r_type.rd;

        set_reg(rd, gpr[rs] | gpr[rt]);

        log("OR: GPR[{:d}] = GPR[{:d}] ({:#x}) | GPR[{:d}] ({:#x})\n", rd, rs, gpr[rs], rt, gpr[rt]);
    }

    void IOProcessor::op_j()
    {
        pc = (pc & 0xF0000000) | (instr.j_type.target << 2);
        next_instr.is_delay_slot = true;
        next_instr.branch_taken = true;

        log("J: Jumping to PC = {:#x}\n", pc);
    }

    void IOProcessor::op_addiu()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        int16_t imm = (int16_t)instr.i_type.immediate;

        set_reg(rt, gpr[rs] + imm);

        log("ADDIU: GPR[{:d}] = GPR[{:d}] ({:#x}) + {:#x}\n", rt, rs, gpr[rs], imm);
    }

    void IOProcessor::op_sll()
    {
        uint16_t rt = instr.r_type.rt;
        uint16_t rd = instr.r_type.rd;
        uint16_t sa = instr.r_type.sa;

        set_reg(rd, gpr[rt] << sa);

        if (instr.value == 0) log("NOP\n");
        else log("SLL: GPR[{:d}] = GPR[{:d}] ({:#x}) << {:d}\n", rd, rt, gpr[rt], sa);
    }

    void IOProcessor::op_sw()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t base = instr.i_type.rs;
        int16_t offset = (int16_t)instr.i_type.immediate;

        uint32_t vaddr = gpr[base] + offset;
        if (!cop0.sr.IsC) 
        {
            if (vaddr & 0x3) [[unlikely]]
            {
                cop0.BadA = vaddr;
                exception(Exception::WriteError, 0);
            }
            else 
            {
                write<uint32_t>(vaddr, gpr[rt]);
            }
        }

        log("SW: Writing GPR[{:d}] ({:#x}) to address {:#x} = GPR[{:d}] ({:#x}) + {:d}\n", rt, gpr[rt], vaddr, base, gpr[base], offset);
    }

    void IOProcessor::op_lui()
    {
        uint16_t rt = instr.i_type.rt;
        uint32_t imm = instr.i_type.immediate;

        set_reg(rt, imm << 16);

        log("LUI: GPR[{:d}] = {:#x}\n", rt, imm << 16);
    }

    void IOProcessor::op_ori()
    {
        uint16_t rt = instr.i_type.rt;
        uint16_t rs = instr.i_type.rs;
        uint32_t imm = instr.i_type.immediate;

        set_reg(rt, gpr[rs] | imm);

        log("ORI: GPR[{:d}] = GPR[{:d}] ({:#x}) | {:#x}\n", rt, rs, gpr[rs], imm);
    }

    void IOProcessor::set_reg(uint32_t regN, uint32_t value)
    {
        write_back.reg = regN;
        write_back.value = value;
    }

    void IOProcessor::load(uint32_t regN, uint32_t value)
    {
        delayed_memory_load.reg = regN;
        delayed_memory_load.value = value;
    }
};