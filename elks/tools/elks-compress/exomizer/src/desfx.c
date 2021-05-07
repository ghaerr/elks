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

#include "desfx.h"
#include "6502emu.h"
#include "log.h"
#include "vec.h"
#include "areatrace.h"

struct mem_ctx
{
    u8 *mem;
    struct areatrace at;
};

static void mem_access_write(struct mem_access *this, u16 address, u8 value)
{
    struct mem_ctx *ctx = this->ctx;

    if ((ctx->mem[1] & 4) == 4 && (ctx->mem[1] & 3) != 0 &&
        address >= 0xd000 && address < 0xe000)
    {
        /* IO-area written and visible */
        return;
    }

    ctx->mem[address] = value;
    areatrace_access(&ctx->at, address);
}

static u8 mem_access_read(struct mem_access *this, u16 address)
{
    struct mem_ctx *ctx = this->ctx;
    return ctx->mem[address];
}

u16 decrunch_sfx(u8 mem[65536], u16 run, int *startp, int *endp, u32 *cyclesp)
{
    struct mem_ctx m;
    struct cpu_ctx r;
    r.cycles = 0;
    r.mem.ctx = &m;
    r.mem.read = mem_access_read;
    r.mem.write = mem_access_write;
    r.pc = run;
    r.sp = '\xf6';
    r.flags = 0;

    m.mem = mem;
    m.mem[1] = 0x37;
    areatrace_init(&m.at);

    LOG(LOG_DEBUG, ("run %04x\n", run));

    /* setting up decrunch */
    while(r.pc >= 0x0400 || r.sp != 0xf6)
    {
        next_inst(&r);
    }

    /* decrunching */
    while(r.pc < 0x400)
    {
        next_inst(&r);
    }

    areatrace_merge_overlapping(&m.at);

    areatrace_get_largest(&m.at, startp, endp);
    if(cyclesp != NULL)
    {
        *cyclesp = r.cycles;
    }

    areatrace_free(&m.at);

    return r.pc;
}
