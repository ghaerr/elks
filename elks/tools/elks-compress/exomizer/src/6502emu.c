/*
 * Copyright (c) 2007 - 2018 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

#include "6502emu.h"
#include "log.h"
#include <stdlib.h>

#define FLAG_N 128
#define FLAG_V  64
#define FLAG_D   8
#define FLAG_I   4
#define FLAG_Z   2
#define FLAG_C   1

struct arg_ea
{
    u16 value;
};
struct arg_relative
{
    i8 value;
};
struct arg_immediate
{
    u8 value;
};

union inst_arg
{
    struct arg_ea ea;
    struct arg_relative rel;
    struct arg_immediate imm;
};

typedef void op_f(struct cpu_ctx *r, int mode, union inst_arg *arg);
typedef int mode_f(struct cpu_ctx *r, union inst_arg *arg);

struct op_info
{
    op_f *f;
    char *fmt;
};

struct mode_info
{
    mode_f *f;
    char *fmt;
};

struct inst_info
{
    struct op_info *op;
    struct mode_info *mode;
    u8 cycles;
};

u16 mem_access_read_u16le(struct mem_access *this, u16 address)
{
    u16 value;
    value = MEM_ACCESS_READ(this, address);
    value |= MEM_ACCESS_READ(this, address + 1) << 8;
    return value;
}

#define MODE_IMMEDIATE 0
#define MODE_ZERO_PAGE 1
#define MODE_ZERO_PAGE_X 2
#define MODE_ZERO_PAGE_Y 3
#define MODE_ABSOLUTE 4
#define MODE_ABSOLUTE_X 5
#define MODE_ABSOLUTE_Y 6
#define MODE_INDIRECT 7
#define MODE_INDIRECT_X 8
#define MODE_INDIRECT_Y 9
#define MODE_RELATIVE 10
#define MODE_ACCUMULATOR 11
#define MODE_IMPLIED 12

static int mode_imm(struct cpu_ctx *r, union inst_arg *arg)
{
    arg->imm.value = MEM_ACCESS_READ(&r->mem, r->pc + 1);
    r->pc += 2;
    return MODE_IMMEDIATE;
}
static int mode_zp(struct cpu_ctx *r, union inst_arg *arg)
{
    arg->ea.value = MEM_ACCESS_READ(&r->mem, r->pc + 1);
    r->pc += 2;
    return MODE_ZERO_PAGE;
}
static int mode_zpx(struct cpu_ctx *r, union inst_arg *arg)
{
    u8 lsbLo = MEM_ACCESS_READ(&r->mem, r->pc + 1) + r->x;
    arg->ea.value = lsbLo;
    r->pc += 2;
    return MODE_ZERO_PAGE_X;
}
static int mode_zpy(struct cpu_ctx *r, union inst_arg *arg)
{
    u8 lsbLo = MEM_ACCESS_READ(&r->mem, r->pc + 1) + r->y;
    arg->ea.value = lsbLo;
    r->pc += 2;
    return MODE_ZERO_PAGE_Y;
}
static int mode_abs(struct cpu_ctx *r, union inst_arg *arg)
{
    u16 offset = MEM_ACCESS_READ(&r->mem, r->pc + 1);
    u16 base = MEM_ACCESS_READ(&r->mem, r->pc + 2) << 8;
    arg->ea.value = base + offset;
    r->pc += 3;
    return MODE_ABSOLUTE;
}
static int mode_absx(struct cpu_ctx *r, union inst_arg *arg)
{
    u16 offset = MEM_ACCESS_READ(&r->mem, r->pc + 1) + r->x;
    u16 base = MEM_ACCESS_READ(&r->mem, r->pc + 2) << 8;
    arg->ea.value = base + offset;
    r->pc += 3;
    r->cycles += (offset > 255);
    return MODE_ABSOLUTE_X;
}
static int mode_absy(struct cpu_ctx *r, union inst_arg *arg)
{
    u16 offset = MEM_ACCESS_READ(&r->mem, r->pc + 1) + r->y;
    u16 base = MEM_ACCESS_READ(&r->mem, r->pc + 2) << 8;
    arg->ea.value = base + offset;
    r->pc += 3;
    r->cycles += (offset > 255);
    return MODE_ABSOLUTE_Y;
}
static int mode_ind(struct cpu_ctx *r, union inst_arg *arg)
{
    u8 lsbLo = MEM_ACCESS_READ(&r->mem, r->pc + 1);
    u8 lsbHi = MEM_ACCESS_READ(&r->mem, r->pc + 2);
    u8 msbLo = lsbLo + 1;
    u8 msbHi = lsbHi;
    u16 base = MEM_ACCESS_READ(&r->mem, msbLo + (msbHi << 8)) << 8;
    u16 offset = MEM_ACCESS_READ(&r->mem, lsbLo + (lsbHi << 8));
    arg->ea.value = base + offset;
    r->pc += 3;
    return MODE_INDIRECT;
}
static int mode_indx(struct cpu_ctx *r, union inst_arg *arg)
{
    u8 lsbLo = MEM_ACCESS_READ(&r->mem, r->pc + 1) + r->x;
    u8 msbLo = lsbLo + 1;
    u16 base = MEM_ACCESS_READ(&r->mem, msbLo) << 8;
    u16 offset = MEM_ACCESS_READ(&r->mem, lsbLo);
    arg->ea.value = base + offset;
    r->pc += 2;
    return MODE_INDIRECT_X;
}
static int mode_indy(struct cpu_ctx *r, union inst_arg *arg)
{
    u8 lsbLo = MEM_ACCESS_READ(&r->mem, r->pc + 1);
    u8 msbLo = lsbLo + 1;
    u16 base = MEM_ACCESS_READ(&r->mem, msbLo) << 8;
    u16 offset = MEM_ACCESS_READ(&r->mem, lsbLo) + r->y;
    arg->ea.value = base + offset;
    r->pc += 2;
    r->cycles += (offset > 255);
    return MODE_INDIRECT_Y;
}
static int mode_rel(struct cpu_ctx *r, union inst_arg *arg)
{
    arg->rel.value = MEM_ACCESS_READ(&r->mem, r->pc + 1);
    r->pc += 2;
    return MODE_RELATIVE;
}
static int mode_acc(struct cpu_ctx *r, union inst_arg *arg)
{
    r->pc += 1;
    return MODE_ACCUMULATOR;
}
static int mode_imp(struct cpu_ctx *r, union inst_arg *arg)
{
    r->pc += 1;
    return MODE_IMPLIED;
}

static struct mode_info mode_imm_o  = {&mode_imm, "#$%02x"};
static struct mode_info mode_zp_o   = {&mode_zp, "$%02x"};
static struct mode_info mode_zpx_o  = {&mode_zpx, "$%02x,x"};
static struct mode_info mode_zpy_o  = {&mode_zpy, "$%02x,y"};
static struct mode_info mode_abs_o  = {&mode_abs, "$%04x"};
static struct mode_info mode_absx_o = {&mode_absx, "$%04x,x"};
static struct mode_info mode_absy_o = {&mode_absy, "$%04x,y"};
static struct mode_info mode_ind_o  = {&mode_ind, "($%04x)"};
static struct mode_info mode_indx_o = {&mode_indx, "($%02x,x)"};
static struct mode_info mode_indy_o = {&mode_indy, "($%02x),y"};
static struct mode_info mode_rel_o  = {&mode_rel, "$%02x"};
static struct mode_info mode_acc_o  = {&mode_acc, "a"};
static struct mode_info mode_imp_o  = {&mode_imp, NULL};

static void update_flags_nz(struct cpu_ctx *r, u8 value)
{
    r->flags &= ~(FLAG_Z | FLAG_N);
    r->flags |= (value == 0 ? FLAG_Z : 0) | (value & FLAG_N);
}

static void update_carry(struct cpu_ctx *r, int bool)
{
    r->flags = (r->flags & ~FLAG_C) | (bool != 0 ? FLAG_C : 0);
}

static void update_overflow(struct cpu_ctx *r, int bool)
{
    r->flags = (r->flags & ~FLAG_V) | (bool != 0 ? FLAG_V : 0);
}

static u8 read_op_arg(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    switch(mode)
    {
    case MODE_IMMEDIATE:
        value = arg->imm.value;
        break;
    case MODE_ACCUMULATOR:
        value = r->a;
        break;
    default:
        value = MEM_ACCESS_READ(&r->mem, arg->ea.value);
    }
    return value;
}

static void write_op_arg(struct cpu_ctx *r,
                         int mode, union inst_arg *arg, u8 value)
{
    switch(mode)
    {
    case MODE_ACCUMULATOR:
        r->a = value;
        break;
    default:
        MEM_ACCESS_WRITE(&r->mem, arg->ea.value, value);
    }
}

static void op_adc(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    u16 result;
    value = read_op_arg(r, mode, arg);
    result = r->a + value + (r->flags & FLAG_C);
    update_carry(r, result & 256);
    update_overflow(r, !((r->a & 0x80) ^ (value & 0x80)) &&
                        ((r->a & 0x80) ^ (result & 0x80)));
    r->a = result;
    update_flags_nz(r, r->a);
}

static void op_and(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    r->a &= value;
    update_flags_nz(r, r->a);
}

static void op_asl(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    update_carry(r, value & 128);
    value <<= 1;
    write_op_arg(r, mode, arg, value);
    update_flags_nz(r, value);
}

static void branch(struct cpu_ctx *r, union inst_arg *arg)
{
    u16 target = r->pc + arg->rel.value;
    r->cycles += 1 + ((target & ~255) != (r->pc & ~255));
    r->pc = target;
}

static void op_bcc(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    if(!(r->flags & FLAG_C))
    {
        branch(r, arg);
    }
}

static void op_bcs(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    if(r->flags & FLAG_C)
    {
        branch(r, arg);
    }
}

static void op_beq(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    if(r->flags & FLAG_Z)
    {
        branch(r, arg);
    }
}

static void op_bit(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    r->flags &= ~(FLAG_N | FLAG_V | FLAG_Z);
    r->flags |= value & (FLAG_N | FLAG_V);
    r->flags |= (value & r->a) == 0 ? FLAG_Z : 0;
}

static void op_bmi(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    if(r->flags & FLAG_N)
    {
        branch(r, arg);
    }
}

static void op_bne(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    if(!(r->flags & FLAG_Z))
    {
        branch(r, arg);
    }
}

static void op_bpl(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    if(!(r->flags & FLAG_N))
    {
        branch(r, arg);
    }
}

static void op_brk(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    MEM_ACCESS_WRITE(&r->mem, 0x100 + r->sp--, (r->pc + 1) >> 8);
    MEM_ACCESS_WRITE(&r->mem, 0x100 + r->sp--, r->pc + 1);
    MEM_ACCESS_WRITE(&r->mem, 0x100 + r->sp--, r->flags | 0x10);
}

static void op_bvc(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    if(!(r->flags & FLAG_V))
    {
        branch(r, arg);
    }
}

static void op_bvs(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    if((r->flags & FLAG_V))
    {
        branch(r, arg);
    }
}

static void op_clc(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags &= ~FLAG_C;
}

static void op_cld(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags &= ~FLAG_D;
}

static void op_cli(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags &= ~FLAG_I;
}

static void op_clv(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags &= ~FLAG_V;
}

static u16 subtract(struct cpu_ctx *r,
                    int carry,
                    u8 val1,
                    u8 value)
{
    u16 target = val1 - value - (1 - !!carry);
    update_carry(r, !(target & 256));
    update_flags_nz(r, target & 255);
    return target;
}

static void op_cmp(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    subtract(r, 1, r->a, value);
}

static void op_cpx(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    subtract(r, 1, r->x, value);
}

static void op_cpy(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    subtract(r, 1, r->y, value);
}

static void op_dec(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value = MEM_ACCESS_READ(&r->mem, arg->ea.value) - 1;
    MEM_ACCESS_WRITE(&r->mem, arg->ea.value, value);
    update_flags_nz(r, value);
}

static void op_dex(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->x--;
    update_flags_nz(r, r->x);
}

static void op_dey(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->y--;
    update_flags_nz(r, r->y);
}

static void op_eor(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    r->a ^= value;
    update_flags_nz(r, r->a);
}

static void op_inc(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value = MEM_ACCESS_READ(&r->mem, arg->ea.value) + 1;
    MEM_ACCESS_WRITE(&r->mem, arg->ea.value, value);
    update_flags_nz(r, value);
}

static void op_inx(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->x++;
    update_flags_nz(r, r->x);
}

static void op_iny(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->y++;
    update_flags_nz(r, r->y);
}

static void op_jmp(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->pc = arg->ea.value;
}

static void op_jsr(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->pc--;
    MEM_ACCESS_WRITE(&r->mem, 0x100 + r->sp--, r->pc >> 8);
    MEM_ACCESS_WRITE(&r->mem, 0x100 + r->sp--, r->pc);
    r->pc = arg->ea.value;
}

static void op_lda(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    r->a = value;
    update_flags_nz(r, r->a);
}

static void op_ldx(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    r->x = value;
    update_flags_nz(r, r->x);
}

static void op_ldy(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    r->y = value;
    update_flags_nz(r, r->y);
}

static void op_lsr(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    update_carry(r, value & 1);
    value >>= 1;
    write_op_arg(r, mode, arg, value);
    update_flags_nz(r, value);
}

static void op_nop(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
}

static void op_ora(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    value = read_op_arg(r, mode, arg);
    r->a |= value;
    update_flags_nz(r, r->a);
}

static void op_pha(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    MEM_ACCESS_WRITE(&r->mem, 0x100 + r->sp--, r->a);
}

static void op_php(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    MEM_ACCESS_WRITE(&r->mem, 0x100 + r->sp--, r->flags & ~0x10);
}

static void op_pla(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->a = MEM_ACCESS_READ(&r->mem, 0x100 + ++r->sp);
    update_flags_nz(r, r->a);
}

static void op_plp(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags = MEM_ACCESS_READ(&r->mem, 0x100 + ++r->sp);
}

static void op_rol(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    u8 old_flags;
    value = read_op_arg(r, mode, arg);
    old_flags = r->flags;
    update_carry(r, value & 128);
    value <<= 1;
    value |= (old_flags & FLAG_C) != 0 ? 1 : 0;
    write_op_arg(r, mode, arg, value);
    update_flags_nz(r, value);
}

static void op_ror(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    u8 old_flags;
    value = read_op_arg(r, mode, arg);
    old_flags = r->flags;
    update_carry(r, value & 1);
    value >>= 1;
    value |= (old_flags & FLAG_C) != 0 ? 128 : 0;
    write_op_arg(r, mode, arg, value);
    update_flags_nz(r, value);
}

static void op_rti(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags = MEM_ACCESS_READ(&r->mem, 0x100 + ++r->sp);
    r->pc = MEM_ACCESS_READ(&r->mem, 0x100 + ++r->sp);
    r->pc |= MEM_ACCESS_READ(&r->mem, 0x100 + ++r->sp) << 8;
}

static void op_rts(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->pc = MEM_ACCESS_READ(&r->mem, 0x100 + ++r->sp);
    r->pc |= MEM_ACCESS_READ(&r->mem, 0x100 + ++r->sp) << 8;
    r->pc += 1;
}

static void op_sbc(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    u8 value;
    u16 result;
    value = read_op_arg(r, mode, arg);
    result = subtract(r, r->flags & FLAG_C, r->a, value);
    update_overflow(r, ((r->a & 0x80) ^ (value & 0x80)) &&
                       ((r->a & 0x80) ^ (result & 0x80)));
    r->a = result;
    update_flags_nz(r, r->a);
}

static void op_sec(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags |= FLAG_C;
}

static void op_sed(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags |= FLAG_D;
}

static void op_sei(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->flags |= FLAG_I;
}

static void op_sta(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    MEM_ACCESS_WRITE(&r->mem, arg->ea.value, r->a);
}

static void op_stx(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    MEM_ACCESS_WRITE(&r->mem, arg->ea.value, r->x);
}

static void op_sty(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    MEM_ACCESS_WRITE(&r->mem, arg->ea.value, r->y);
}

static void op_tax(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->x = r->a;
    update_flags_nz(r, r->x);
}

static void op_tay(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->y = r->a;
    update_flags_nz(r, r->y);
}

static void op_tsx(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->x = r->sp;
    update_flags_nz(r, r->x);
}

static void op_txa(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->a = r->x;
    update_flags_nz(r, r->a);
}

static void op_txs(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->sp = r->x;
}

static void op_tya(struct cpu_ctx *r, int mode, union inst_arg *arg)
{
    r->a = r->y;
    update_flags_nz(r, r->a);
}

static struct op_info op_adc_o = {&op_adc, "adc"};
static struct op_info op_and_o = {&op_and, "and"};
static struct op_info op_asl_o = {&op_asl, "asl"};
static struct op_info op_bcc_o = {&op_bcc, "bcc"};
static struct op_info op_bcs_o = {&op_bcs, "bcs"};
static struct op_info op_beq_o = {&op_beq, "beq"};
static struct op_info op_bit_o = {&op_bit, "bit"};
static struct op_info op_bmi_o = {&op_bmi, "bmi"};
static struct op_info op_bne_o = {&op_bne, "bne"};
static struct op_info op_bpl_o = {&op_bpl, "bpl"};
static struct op_info op_brk_o = {&op_brk, "brk"};
static struct op_info op_bvc_o = {&op_bvc, "bvc"};
static struct op_info op_bvs_o = {&op_bvs, "bvs"};
static struct op_info op_clc_o = {&op_clc, "clc"};

static struct op_info op_cld_o = {&op_cld, "cld"};
static struct op_info op_cli_o = {&op_cli, "cli"};
static struct op_info op_clv_o = {&op_clv, "clv"};
static struct op_info op_cmp_o = {&op_cmp, "cmp"};
static struct op_info op_cpx_o = {&op_cpx, "cpx"};
static struct op_info op_cpy_o = {&op_cpy, "cpy"};
static struct op_info op_dec_o = {&op_dec, "dec"};
static struct op_info op_dex_o = {&op_dex, "dex"};
static struct op_info op_dey_o = {&op_dey, "dey"};
static struct op_info op_eor_o = {&op_eor, "eor"};
static struct op_info op_inc_o = {&op_inc, "inc"};
static struct op_info op_inx_o = {&op_inx, "inx"};
static struct op_info op_iny_o = {&op_iny, "iny"};
static struct op_info op_jmp_o = {&op_jmp, "jmp"};

static struct op_info op_jsr_o = {&op_jsr, "jsr"};
static struct op_info op_lda_o = {&op_lda, "lda"};
static struct op_info op_ldx_o = {&op_ldx, "ldx"};
static struct op_info op_ldy_o = {&op_ldy, "ldy"};
static struct op_info op_lsr_o = {&op_lsr, "lsr"};
static struct op_info op_nop_o = {&op_nop, "nop"};
static struct op_info op_ora_o = {&op_ora, "ora"};
static struct op_info op_pha_o = {&op_pha, "pha"};
static struct op_info op_php_o = {&op_php, "php"};
static struct op_info op_pla_o = {&op_pla, "pla"};
static struct op_info op_plp_o = {&op_plp, "plp"};
static struct op_info op_rol_o = {&op_rol, "rol"};
static struct op_info op_ror_o = {&op_ror, "ror"};
static struct op_info op_rti_o = {&op_rti, "rti"};

static struct op_info op_rts_o = {&op_rts, "rts"};
static struct op_info op_sbc_o = {&op_sbc, "sbc"};
static struct op_info op_sec_o = {&op_sec, "sec"};
static struct op_info op_sed_o = {&op_sed, "sed"};
static struct op_info op_sei_o = {&op_sei, "sei"};
static struct op_info op_sta_o = {&op_sta, "sta"};
static struct op_info op_stx_o = {&op_stx, "stx"};
static struct op_info op_sty_o = {&op_sty, "sty"};
static struct op_info op_tax_o = {&op_tax, "tax"};
static struct op_info op_tay_o = {&op_tay, "tay"};
static struct op_info op_tsx_o = {&op_tsx, "tsx"};
static struct op_info op_txa_o = {&op_txa, "txa"};
static struct op_info op_txs_o = {&op_txs, "txs"};
static struct op_info op_tya_o = {&op_tya, "tya"};

#define NULL_OP {NULL, NULL, 0}

/* http://www.obelisk.demon.co.uk/6502/reference.html is a nice reference */
static struct inst_info ops[256] = {
    /* 0x00 */
    {&op_brk_o, &mode_imp_o, 7},
    {&op_ora_o, &mode_indx_o, 6},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_ora_o, &mode_zp_o, 3},
    {&op_asl_o, &mode_zp_o, 5},
    NULL_OP,
    {&op_php_o, &mode_imp_o, 3},
    {&op_ora_o, &mode_imm_o, 2},
    {&op_asl_o, &mode_acc_o, 2},
    NULL_OP,
    NULL_OP,
    {&op_ora_o, &mode_abs_o, 4},
    {&op_asl_o, &mode_abs_o, 6},
    NULL_OP,
    /* 0x10 */
    {&op_bpl_o, &mode_rel_o, 2},
    {&op_ora_o, &mode_indy_o, 5},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_ora_o, &mode_zpx_o, 4},
    {&op_asl_o, &mode_zpx_o, 6},
    NULL_OP,
    {&op_clc_o, &mode_imp_o, 2},
    {&op_ora_o, &mode_absy_o, 4},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_ora_o, &mode_absx_o, 4},
    {&op_asl_o, &mode_absx_o, 7},
    NULL_OP,
    /* 0x20 */
    {&op_jsr_o, &mode_abs_o, 6},
    {&op_and_o, &mode_indx_o, 6},
    NULL_OP,
    NULL_OP,
    {&op_bit_o, &mode_zp_o, 3},
    {&op_and_o, &mode_zp_o, 3},
    {&op_rol_o, &mode_zp_o, 5},
    NULL_OP,
    {&op_plp_o, &mode_imp_o, 4},
    {&op_and_o, &mode_imm_o, 2},
    {&op_rol_o, &mode_acc_o, 2},
    NULL_OP,
    {&op_bit_o, &mode_abs_o, 4},
    {&op_and_o, &mode_abs_o, 4},
    {&op_rol_o, &mode_abs_o, 6},
    NULL_OP,
    /* 0x30 */
    {&op_bmi_o, &mode_rel_o, 2},
    {&op_and_o, &mode_indy_o, 5},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_and_o, &mode_zpx_o, 4},
    {&op_rol_o, &mode_zpx_o, 6},
    NULL_OP,
    {&op_sec_o, &mode_imp_o, 2},
    {&op_and_o, &mode_absy_o, 4},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_and_o, &mode_absx_o, 4},
    {&op_rol_o, &mode_absx_o, 7},
    NULL_OP,
    /* 0x40 */
    {&op_rti_o, &mode_imp_o, 6},
    {&op_eor_o, &mode_indx_o, 6},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_eor_o, &mode_zp_o, 4},
    {&op_lsr_o, &mode_zp_o, 5},
    NULL_OP,
    {&op_pha_o, &mode_imp_o, 3},
    {&op_eor_o, &mode_imm_o, 2},
    {&op_lsr_o, &mode_acc_o, 2},
    NULL_OP,
    {&op_jmp_o, &mode_abs_o, 3},
    {&op_eor_o, &mode_abs_o, 4},
    {&op_lsr_o, &mode_abs_o, 6},
    NULL_OP,
    /* 0x50 */
    {&op_bvc_o, &mode_rel_o, 2},
    {&op_eor_o, &mode_indy_o, 5},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_eor_o, &mode_zpx_o, 3},
    {&op_lsr_o, &mode_zpx_o, 6},
    NULL_OP,
    {&op_cli_o, &mode_imp_o, 2},
    {&op_eor_o, &mode_absy_o, 4},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_eor_o, &mode_absx_o, 4},
    {&op_lsr_o, &mode_absx_o, 7},
    NULL_OP,
    /* 0x60 */
    {&op_rts_o, &mode_imp_o, 6},
    {&op_adc_o, &mode_indx_o, 6},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_adc_o, &mode_zp_o, 3},
    {&op_ror_o, &mode_zp_o, 5},
    NULL_OP,
    {&op_pla_o, &mode_imp_o, 4},
    {&op_adc_o, &mode_imm_o, 2},
    {&op_ror_o, &mode_acc_o, 2},
    {&op_jmp_o, &mode_ind_o, 5},
    NULL_OP,
    {&op_adc_o, &mode_abs_o, 4},
    {&op_ror_o, &mode_abs_o, 6},
    NULL_OP,
    /* 0x70 */
    {&op_bvs_o, &mode_rel_o, 2},
    {&op_adc_o, &mode_indy_o, 5},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_adc_o, &mode_zpx_o, 4},
    {&op_ror_o, &mode_zpx_o, 6},
    NULL_OP,
    {&op_sei_o, &mode_imp_o, 2},
    {&op_adc_o, &mode_absy_o, 4},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_adc_o, &mode_absx_o, 4},
    {&op_ror_o, &mode_absx_o, 7},
    NULL_OP,
    /* 0x80 */
    NULL_OP,
    {&op_sta_o, &mode_indx_o, 6},
    NULL_OP,
    NULL_OP,
    {&op_sty_o, &mode_zp_o, 3},
    {&op_sta_o, &mode_zp_o, 3},
    {&op_stx_o, &mode_zp_o, 3},
    NULL_OP,
    {&op_dey_o, &mode_imp_o, 2},
    NULL_OP,
    {&op_txa_o, &mode_imp_o, 2},
    NULL_OP,
    {&op_sty_o, &mode_abs_o, 4},
    {&op_sta_o, &mode_abs_o, 4},
    {&op_stx_o, &mode_abs_o, 4},
    NULL_OP,
    /* 0x90 */
    {&op_bcc_o, &mode_rel_o, 2},
    {&op_sta_o, &mode_indy_o, 6},
    NULL_OP,
    NULL_OP,
    {&op_sty_o, &mode_zpx_o, 4},
    {&op_sta_o, &mode_zpx_o, 4},
    {&op_stx_o, &mode_zpy_o, 4},
    NULL_OP,
    {&op_tya_o, &mode_imp_o, 2},
    {&op_sta_o, &mode_absy_o, 5},
    {&op_txs_o, &mode_imp_o, 2},
    NULL_OP,
    NULL_OP,
    {&op_sta_o, &mode_absx_o, 5},
    NULL_OP,
    NULL_OP,
    /* 0xa0 */
    {&op_ldy_o, &mode_imm_o, 2},
    {&op_lda_o, &mode_indx_o, 6},
    {&op_ldx_o, &mode_imm_o, 2},
    NULL_OP,
    {&op_ldy_o, &mode_zp_o, 3},
    {&op_lda_o, &mode_zp_o, 3},
    {&op_ldx_o, &mode_zp_o, 3},
    NULL_OP,
    {&op_tay_o, &mode_imp_o, 2},
    {&op_lda_o, &mode_imm_o, 2},
    {&op_tax_o, &mode_imp_o, 2},
    NULL_OP,
    {&op_ldy_o, &mode_abs_o, 4},
    {&op_lda_o, &mode_abs_o, 4},
    {&op_ldx_o, &mode_abs_o, 4},
    NULL_OP,
    /* 0xb0 */
    {&op_bcs_o, &mode_rel_o, 2},
    {&op_lda_o, &mode_indy_o, 5},
    NULL_OP,
    NULL_OP,
    {&op_ldy_o, &mode_zpx_o, 4},
    {&op_lda_o, &mode_zpx_o, 4},
    {&op_ldx_o, &mode_zpy_o, 4},
    NULL_OP,
    {&op_clv_o, &mode_imp_o, 2},
    {&op_lda_o, &mode_absy_o, 4},
    {&op_tsx_o, &mode_imp_o, 2},
    NULL_OP,
    {&op_ldy_o, &mode_absx_o, 4},
    {&op_lda_o, &mode_absx_o, 4},
    {&op_ldx_o, &mode_absy_o, 4},
    NULL_OP,
    /* 0xc0 */
    {&op_cpy_o, &mode_imm_o, 2},
    {&op_cmp_o, &mode_indx_o, 6},
    NULL_OP,
    NULL_OP,
    {&op_cpy_o, &mode_zp_o, 3},
    {&op_cmp_o, &mode_zp_o, 3},
    {&op_dec_o, &mode_zp_o, 5},
    NULL_OP,
    {&op_iny_o, &mode_imp_o, 2},
    {&op_cmp_o, &mode_imm_o, 2},
    {&op_dex_o, &mode_imp_o, 2},
    NULL_OP,
    {&op_cpy_o, &mode_abs_o, 4},
    {&op_cmp_o, &mode_abs_o, 4},
    {&op_dec_o, &mode_abs_o, 6},
    NULL_OP,
    /* 0xd0 */
    {&op_bne_o, &mode_rel_o, 2},
    {&op_cmp_o, &mode_indy_o, 5},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_cmp_o, &mode_zpx_o, 4},
    {&op_dec_o, &mode_zpx_o, 6},
    NULL_OP,
    {&op_cld_o, &mode_imp_o, 2},
    {&op_cmp_o, &mode_absy_o, 4},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_cmp_o, &mode_absx_o, 4},
    {&op_dec_o, &mode_absx_o, 7},
    NULL_OP,
    /* 0xe0 */
    {&op_cpx_o, &mode_imm_o, 2},
    {&op_sbc_o, &mode_indx_o, 6},
    NULL_OP,
    NULL_OP,
    {&op_cpx_o, &mode_zp_o, 3},
    {&op_sbc_o, &mode_zp_o, 3},
    {&op_inc_o, &mode_zp_o, 5},
    NULL_OP,
    {&op_inx_o, &mode_imp_o, 2},
    {&op_sbc_o, &mode_imm_o, 2},
    {&op_nop_o, &mode_imp_o, 2},
    NULL_OP,
    {&op_cpx_o, &mode_abs_o, 4},
    {&op_sbc_o, &mode_abs_o, 4},
    {&op_inc_o, &mode_abs_o, 6},
    NULL_OP,
    /* 0xf0 */
    {&op_beq_o, &mode_rel_o, 2},
    {&op_sbc_o, &mode_indy_o, 5},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_sbc_o, &mode_zpx_o, 4},
    {&op_inc_o, &mode_zpx_o, 6},
    NULL_OP,
    {&op_sed_o, &mode_imp_o, 2},
    {&op_sbc_o, &mode_absy_o, 4},
    NULL_OP,
    NULL_OP,
    NULL_OP,
    {&op_sbc_o, &mode_absx_o, 4},
    {&op_inc_o, &mode_absx_o, 7},
    NULL_OP,
};

void next_inst(struct cpu_ctx *r)
{
    union inst_arg arg;
    int oldpc = r->pc;
    int op_code = MEM_ACCESS_READ(&r->mem, r->pc);
    struct inst_info *info = ops + op_code;
    int mode;
    if(info->op == NULL)
    {
        LOG(LOG_ERROR, ("unimplemented opcode $%02X @ $%04X\n",
                        op_code, r->pc));
        exit(1);
    }
    LOG(LOG_DUMP, ("%08d, %02x %02x %02x %02x: ",
                   r->cycles, r->a, r->x, r->y, r->sp));
    mode = info->mode->f(r, &arg);

    if(IS_LOGGABLE(LOG_DUMP))
    {
        int i;
        int pc = oldpc;
        LOG(LOG_DUMP, ("%04x", pc));
        for(i = 0; i < 3; ++i)
        {
            if(pc < r->pc)
            {
                LOG(LOG_DUMP, (" %02x", MEM_ACCESS_READ(&r->mem, pc)));
                ++pc;
            }
            else
            {
                LOG(LOG_DUMP, ("   "));
            }
        }
        LOG(LOG_DUMP, (" %s", info->op->fmt));

        if(info->mode->fmt != NULL)
        {
            int value = 0;
            while(--pc > oldpc)
            {
                value <<= 8;
                value |= MEM_ACCESS_READ(&r->mem, pc);
            }
            LOG(LOG_DUMP, (" "));
            if(mode == MODE_RELATIVE)
            {
                LOG(LOG_DUMP, ("$%04x", r->pc + arg.rel.value));
            }
            else
            {
                LOG(LOG_DUMP, (info->mode->fmt, value));
            }
            switch (mode)
            {
            case MODE_RELATIVE:
            case MODE_IMPLIED:
            case MODE_ACCUMULATOR:
            case MODE_IMMEDIATE:
            case MODE_ABSOLUTE:
                break;
            default:
                LOG(LOG_DUMP, (" ($%04x)", arg.ea.value));
            }
        }
        LOG(LOG_DUMP, ("\n"));
    }
    info->op->f(r, mode, &arg);
    r->cycles += info->cycles;
}
