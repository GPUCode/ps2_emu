#pragma once

namespace ee
{
    /* Instruction opcodes */
    enum Opcodes
    {
        COP2,
        SW,
        SLTI,
        BNE,
        ORI,
        ADDI,
        LQ,
        LUI,
        ADDIU,
        MMI,
        SD,
        JAL,
        REGIMM,
        ANDI,
        BEQ,
        BEQL,
        SLTIU,
        BNEL,
        LB,
        SWC1,
        LBU,
        LD,
        J,
        LW,
        SB,
        BLEZ,
        BGTZ,
        LHU,
        SH,
        XORI,
        DADDIU,
        SQ,
        LH,
        CACHE,
        LWU,
        LDL,
        LDR,
        SDL,
        SDR,
        LWC1,
        BLEZL,
        LWL,
        LWR,
        SWL,
        SWR,
        SQC2
    };

    enum Special
    {
        JR,
        JALR,
        SYSCALL,
        SLL,
        SYNC,
        SRA,
        ADDU,
        DADDU,
        OR,
        MULT,
        DIVU,
        MFLO,
        DIV,
        MFHI,
        SLTU,
        SUBU,
        MOVN,
        SLT,
        AND,
        SRL,
        DSLL32,
        DSRA32,
        DSLL,
        DSRAV,
        MOVZ,
        DSSLV,
        SLLV,
        SRAV,
        NOR,
        DSRL,
        SRLV,
        DSRL32,
        MFSA,
        MTHI,
        MTLO,
        MTSA,
        DSUBU,
        XOR,
        MULTU,
        DSRA,
        SUB,
        ADD
    };

    enum OpcodeType
    {
        Arithmetic,
        Branch,
        Coprocessor,
        Comparison,
        Logic,
        Memory,
        Move,
        Shift
    };

    struct EmotionEngine;
    using InterpreterFunc = void (EmotionEngine::*)();

    struct DecodeInfo
    {
        OpcodeType type;
        InterpreterFunc fallback = nullptr;
    };
}
