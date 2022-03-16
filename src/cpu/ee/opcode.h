#pragma once
#include <cpu/ee/ee.h>

namespace ee
{
    /* Opcodes */
    void op_cop0(EmotionEngine* ee);
    void op_mfc0(EmotionEngine* ee);
    void op_sw(EmotionEngine* ee);
    void op_special(EmotionEngine* ee);
    void op_sll(EmotionEngine* ee);
    void op_slti(EmotionEngine* ee);
    void op_bne(EmotionEngine* ee);
    void op_ori(EmotionEngine* ee);
    void op_addi(EmotionEngine* ee);
    void op_lq(EmotionEngine* ee);
    void op_lui(EmotionEngine* ee);
    void op_jr(EmotionEngine* ee);
    void op_addiu(EmotionEngine* ee);
    void op_tlbwi(EmotionEngine* ee);
    void op_mtc0(EmotionEngine* ee);
    void op_lw(EmotionEngine* ee);
    void op_mmi(EmotionEngine* ee);
    void op_madd1(EmotionEngine* ee);
    void op_jalr(EmotionEngine* ee);
    void op_sd(EmotionEngine* ee);
    void op_jal(EmotionEngine* ee);
    void op_sra(EmotionEngine* ee);
    void op_regimm(EmotionEngine* ee);
    void op_bgez(EmotionEngine* ee);
    void op_addu(EmotionEngine* ee);
    void op_daddu(EmotionEngine* ee);
    void op_andi(EmotionEngine* ee);
    void op_beq(EmotionEngine* ee);
    void op_or(EmotionEngine* ee);
    void op_mult(EmotionEngine* ee);
    void op_divu(EmotionEngine* ee);
    void op_beql(EmotionEngine* ee);
    void op_mflo(EmotionEngine* ee);
    void op_sltiu(EmotionEngine* ee);
    void op_bnel(EmotionEngine* ee);
    void op_sync(EmotionEngine* ee);
    void op_lb(EmotionEngine* ee);
    void op_swc1(EmotionEngine* ee);
    void op_lbu(EmotionEngine* ee);
    void op_ld(EmotionEngine* ee);
    void op_j(EmotionEngine* ee);
    void op_sb(EmotionEngine* ee);
    void op_div(EmotionEngine* ee);
    void op_mfhi(EmotionEngine* ee);
    void op_sltu(EmotionEngine* ee);
    void op_blez(EmotionEngine* ee);
    void op_subu(EmotionEngine* ee);
    void op_bgtz(EmotionEngine* ee);
    void op_movn(EmotionEngine* ee);
    void op_slt(EmotionEngine* ee);
    void op_and(EmotionEngine* ee);
    void op_srl(EmotionEngine* ee);
    void op_dsll32(EmotionEngine* ee);
    void op_dsra32(EmotionEngine* ee);
    void op_dsll(EmotionEngine* ee);
    void op_lhu(EmotionEngine* ee);
    void op_bltz(EmotionEngine* ee);
    void op_sh(EmotionEngine* ee);
    void op_madd(EmotionEngine* ee);
    void op_divu1(EmotionEngine* ee);
    void op_mflo1(EmotionEngine* ee);
    void op_dsrav(EmotionEngine* ee);
    void op_xori(EmotionEngine* ee);
    void op_mult1(EmotionEngine* ee);
    void op_movz(EmotionEngine* ee);
    void op_dsllv(EmotionEngine* ee);
    void op_daddiu(EmotionEngine* ee);
    void op_sq(EmotionEngine* ee);
    void op_lh(EmotionEngine* ee);
    void op_cache(EmotionEngine* ee);
    void op_sllv(EmotionEngine* ee);
    void op_srav(EmotionEngine* ee);
    void op_nor(EmotionEngine* ee);
    void op_lwu(EmotionEngine* ee);
    void op_ldl(EmotionEngine* ee);
    void op_ldr(EmotionEngine* ee);
    void op_sdl(EmotionEngine* ee);
    void op_sdr(EmotionEngine* ee);
    void op_dsrl(EmotionEngine* ee);
    void op_srlv(EmotionEngine* ee);
    void op_dsrl32(EmotionEngine* ee);
    void op_syscall(EmotionEngine* ee);
    void op_bltzl(EmotionEngine* ee);
    void op_bgezl(EmotionEngine* ee);
    void op_mfsa(EmotionEngine* ee);
    void op_mthi(EmotionEngine* ee);
    void op_mtlo(EmotionEngine* ee);
    void op_mtsa(EmotionEngine* ee);
    void op_lwc1(EmotionEngine* ee);
    void op_dsubu(EmotionEngine* ee);
    void op_blezl(EmotionEngine* ee);
    void op_xor(EmotionEngine* ee);
    void op_multu(EmotionEngine* ee);
    void op_lwl(EmotionEngine* ee);
    void op_lwr(EmotionEngine* ee);
    void op_swl(EmotionEngine* ee);
    void op_swr(EmotionEngine* ee);
    void op_sqc2(EmotionEngine* ee);
    void op_dsra(EmotionEngine* ee);
    void op_sub(EmotionEngine* ee);
    void op_add(EmotionEngine* ee);

    /* COP0 instructions */
    void op_di(EmotionEngine* ee);
    void op_eret(EmotionEngine* ee);
    void op_ei(EmotionEngine* ee);

    /* COP1 instructions */
    void op_cop1(EmotionEngine* ee);
    void op_mtc1(EmotionEngine* ee);
    void op_ctc1(EmotionEngine* ee);
    void op_cfc1(EmotionEngine* ee);

    /* COP2 instructions */
    void op_cop2(EmotionEngine* ee);

    /* Parallel instructions */
    void op_por(EmotionEngine* ee);
    void op_padduw(EmotionEngine* ee);

    /* MMI instructions */
    void op_plzcw(EmotionEngine* ee);
    void op_mfhi1(EmotionEngine* ee);
    void op_mthi1(EmotionEngine* ee);
    void op_mtlo1(EmotionEngine* ee);
    void op_pcpyh(EmotionEngine* ee);
    void op_pcpyld(EmotionEngine* ee);
    void op_pnor(EmotionEngine* ee);
    void op_psubb(EmotionEngine* ee);
    void op_pand(EmotionEngine* ee);
    void op_pcpyud(EmotionEngine* ee);
    void op_pxor(EmotionEngine* ee);
    void op_psubw(EmotionEngine* ee);
}
