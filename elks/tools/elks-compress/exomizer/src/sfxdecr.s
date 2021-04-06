;
; Copyright (c) 2002 - 2018 Magnus Lind.
;
; This software is provided 'as-is', without any express or implied warranty.
; In no event will the authors be held liable for any damages arising from
; the use of this software.
;
; Permission is granted to anyone to use this software, alter it and re-
; distribute it freely for any non-commercial, non-profit purpose subject to
; the following restrictions:
;
;   1. The origin of this software must not be misrepresented; you must not
;   claim that you wrote the original software. If you use this software in a
;   product, an acknowledgment in the product documentation would be
;   appreciated but is not required.
;
;   2. Altered source versions must be plainly marked as such, and must not
;   be misrepresented as being the original software.
;
;   3. This notice may not be removed or altered from any distribution.
;
;   4. The names of this software and/or it's copyright holders may not be
;   used to endorse or promote products derived from this software without
;   specific prior written permission.
;
; -------------------------------------------------------------------
; -- r_start_addr, done /* required, -2=basic start */
; -- r_target, done /* required, 1, 20, 23, 52, 55, 4, 64, 128, $a2 or $a8 */
; -- r_in_load, done /* required, load address of decrunched data area */
; -- r_in_len, done /* required, length of decrunched data area */
; -- i_literal_sequences_used, done /* defined if true, otherwise not */
; -- i_max_sequence_length_256, done /* defined if true, otherwise not */
; -- i_reuse_offset, done /* defined if true, otherwise not */
; -- i_ram_enter, done /* undef=c_rom_config_value */
; -- i_irq_enter, done /* undef=on, 0=off, 1=on */
; -- i_ram_during, done /* undef=auto
; -- i_irq_during, done /* undef=auto, 0=off, 1=on */
; -- i_ram_exit, done /* undef=auto
; -- i_irq_exit, done /* undef=auto, 0=off, 1=on */
; -- i_nmi_during
; -- i_nmi_enter
; -- i_nmi_exit
; -- i_effect, done /* -1=none, 0(default)=lower right, 1-3=border */
; -- i_effect_speed, done /* undef or 0=slow, otherwise fast */
; -- i_table_addr, done /* undef=$0334 or if(r_target == 128) $0b00 */
; -- i_fourth_offset_table, done /* defined if true, otherwise not */
; -- i_load_addr, done /* optional, if set no basic stub, loads to value */
; -- i_raw, done /* optional, if defined and != 0 outfile is raw */
; -- i_perf, done [-1 (slow/short), 0(default), 1, 2(fast/large)
; -- the crunched file to it.
; -------------------------------------------------------------------
; - if basic start --------------------------------------------------
; -- i_basic_txt_start    /* will be set before basic start, if defined */
; -- i_basic_var_start    /* will be set before basic start, if defined */
; -- i_basic_highest_addr /* will be set before basic start, if defined */
; -------------------------------------------------------------------

.IF(!.DEFINED(r_target))
  .ERROR("Required symbol r_target not defined.")
.ENDIF
.IF(!.DEFINED(i_effect))
  i_effect = 0
.ENDIF
.IF(!.DEFINED(i_perf))
  i_perf = 0
.ENDIF

.IF(.DEFINED(i_irq_enter) && i_irq_enter != 0 && i_irq_enter != 1)
  .ERROR("Symbol i_irq_enter must be undefined, 0 or 1.")
.ENDIF

.IF(.DEFINED(i_irq_during) && i_irq_during != 0 && i_irq_during != 1)
  .ERROR("Symbol i_irq_during must be undefined, 0 or 1.")
.ENDIF

.IF(.DEFINED(i_irq_exit) && i_irq_exit != 0 && i_irq_exit != 1)
  .ERROR("Symbol i_irq_exit must be undefined, 0 or 1.")
.ENDIF

.IF(.DEFINED(i_load_addr) && (i_load_addr < 0 || i_load_addr > 65535))
  .ERROR("Symbol i_load_addr must be undefined or [0 or 65535].")
.ENDIF

.IF(r_target == 1)
  zp_len_lo = $80
  zp_len_hi = $81
  zp_src_lo = $82
  zp_src_hi = zp_src_lo + 1
  zp_bits_hi = $84
  zp_ro_state = $85

  c_basic_start    = $0501
  c_end_of_mem_rom = $c000
  c_effect_color   = $bfdf
  c_border_color   = $bfdf
  c_rom_config_value = 0
  c_ram_config_value = 1
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $b800
.ELIF(r_target == 20 || r_target == 23 || r_target == 52 || r_target == 55 ||
      r_target == 64 || r_target == 128 || r_target == 4032)
  zp_len_lo = $9e
  zp_len_hi = $9f
  zp_src_lo = $ae
  zp_src_hi = zp_src_lo + 1
  zp_bits_hi = $a7
  zp_ro_state = $a8

  .IF(r_target == 20)
  c_basic_start    = $1001
  c_end_of_mem_rom = $2000
  c_effect_char    = $1ff9
  c_effect_color   = $97f9
  c_border_color   = $900f
  c_rom_config_value = 0
  c_ram_config_value = 0
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0334
  .ELIF(r_target == 23)
  c_basic_start    = $0401
  c_end_of_mem_rom = $2000
  c_effect_char    = $1ff9
  c_effect_color   = $97f9
  c_border_color   = $900f
  c_rom_config_value = 0
  c_ram_config_value = 0
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0334
  .ELIF(r_target == 52)
  c_basic_start    = $1201
  c_end_of_mem_rom = $8000
  c_effect_char    = $11f9
  c_effect_color   = $95f9
  c_border_color   = $900f
  c_rom_config_value = 0
  c_ram_config_value = 0
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0334
  .ELIF(r_target == 55)
  c_basic_start    = $1201
  c_end_of_mem_rom = $8000
  c_effect_char    = $11f9
  c_effect_color   = $95f9
  c_border_color   = $900f
  c_rom_config_value = 0
  c_ram_config_value = 0
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0334
  .ELIF(r_target == 64)
  c_basic_start    = $0801
  c_end_of_mem_rom = $a000
  c_effect_char    = $07e7
  c_effect_color   = $dbe7
  c_border_color   = $d020
  c_rom_config_value = $37
  c_ram_config_value = $38
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0334
  .ELIF(r_target == 128)
  c_basic_start    = $1c01
  c_end_of_mem_rom = $4000
  c_effect_char    = $07e7
  c_effect_color   = $dbe7
  c_border_color   = $d020
  c_rom_config_value = $00
  c_ram_config_value = $3f
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0b00
  .ELIF(r_target == 4032)
  c_basic_start = $0401
  c_end_of_mem_rom = $8000
  c_rom_config_value = 0
  c_ram_config_value = 0
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $027a
  .ENDIF
.ELIF(r_target == 16 || r_target == 4)
  zp_len_lo = $d8
  zp_len_hi = $d9
  zp_src_lo = $da
  zp_src_hi = zp_src_lo + 1
  zp_bits_hi = $dc
  zp_ro_state = $dd

  .IF(r_target == 16)
  c_basic_start    = $1001
  c_end_of_mem_rom = $8000
  c_effect_char    = $0fe7
  c_effect_color   = $0be7
  c_border_color   = $ff19
  c_rom_config_value = 0
  c_ram_config_value = 1
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0334
  .ELIF(r_target == 4)
  c_basic_start    = $1001
  c_end_of_mem_rom = $8000
  c_effect_char    = $0fe7
  c_effect_color   = $0be7
  c_border_color   = $ff19
  c_rom_config_value = 0
  c_ram_config_value = 1
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0334
  .ENDIF
.ELIF(r_target == $a2)
  ; http://apple2.org.za/gswv/a2zine/faqs/csa2pfaq.html#017
  zp_len_lo = $eb
  zp_len_hi = $ec
  zp_src_lo = $ed
  zp_src_hi = zp_src_lo + 1
  zp_bits_hi = $ef
  zp_ro_state = $fa

  c_basic_start    = $0801
  c_end_of_mem_rom = $9600
  c_effect_color   = $07f7
  c_border_color   = $07f7
  c_rom_config_value = 0
  c_ram_config_value = 0
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0334
.ELIF(r_target == $a8)
  zp_len_lo = $f7
  zp_len_hi = $fc
  zp_src_lo = $f9
  zp_src_hi = zp_src_lo + 1
  zp_bits_hi = $f8
  zp_ro_state = $f9

  c_end_of_mem_rom = $a000
  c_effect_color   = $d017
  c_border_color   = $d01a
  c_rom_config_value = $ff
  c_ram_config_value = $fe
  c_rom_nmi_value = $40
  c_ram_nmi_value = 0
  c_default_table = $0600
.ELIF(r_target == $bbcb)
  zp_len_lo = $70
  zp_len_hi = $71
  zp_src_lo = $72
  zp_src_hi = zp_src_lo + 1
  zp_bits_hi = $74
  zp_ro_state = $75

  c_basic_start    = $1900
  c_end_of_mem_rom = $7c00
  c_effect_char    = $7fe7
  c_effect_color   = $7fe6
  c_border_color   = $7fe7
  c_rom_config_value = 0
  c_ram_config_value = 0
  c_rom_nmi_value = 0
  c_ram_nmi_value = 0
  c_default_table = $0a34
.ELSE
  .ERROR("Symbol r_target_addr has an invalid value.")
.ENDIF

v_safety_addr = .INCWORD("crunched_data", 0)
transfer_len ?= 0

.IF(i_effect == 0 && (r_target == 16 || r_target == 4 || r_target == $a2) &&
    (r_in_load < c_effect_color + 1 &&
     r_in_load + r_in_len > c_effect_color ||
     v_safety_addr < c_effect_color + 1 &&
     v_safety_addr + transfer_len > c_effect_color))
        ;; disable default effect if the address is in the
        ;; data area for the c64/plus4 and Apple targets.
  i_effect2 = -1
.ELSE
  i_effect2 = i_effect
.ENDIF

.IF(!.DEFINED(i_table_addr))
  i_table_addr = c_default_table
.ENDIF

.IF(!.DEFINED(i_ram_enter))
  i_ram_enter = c_rom_config_value
.ENDIF

.IF(!.DEFINED(i_nmi_enter))
  i_nmi_enter = c_rom_nmi_value
.ENDIF

.IF(!.DEFINED(i_irq_enter))
  i_irq_enter = 1
.ENDIF

.IF(!.DEFINED(i_irq_during))
  .IF((r_target == 16 || r_target == 4) &&
      (r_in_load < $0800 ||
       v_safety_addr < $0800 ||
       i_table_addr < $0800 && i_table_addr != c_default_table))
    ;; must disable irq on plus4/c16 to access mem < $0800
    i_irq_during = 0
    .IF(!.DEFINED(i_irq_exit))
      i_irq_exit = 0
    .ENDIF
  .ELIF((r_target == 20 || r_target == 23 || r_target == 52 ||
         r_target == 55 || r_target == 64 || r_target == 128) &&
        (r_in_load < c_default_table ||
         v_safety_addr < c_default_table ||
         i_table_addr < c_default_table))
    ;; must disable irq on vic20,c64,c128 to access mem < c_default_table
    i_irq_during = 0
    .IF(!.DEFINED(i_irq_exit))
      i_irq_exit = 0
    .ENDIF
  .ENDIF
.ENDIF

.IF(!.DEFINED(i_ram_exit))
  i_ram_exit = c_rom_config_value
.ENDIF

.IF(!.DEFINED(i_nmi_exit))
  i_nmi_exit = c_rom_nmi_value
.ENDIF

.IF(!.DEFINED(i_irq_exit))
  .IF(i_ram_exit != i_ram_enter)
  i_irq_exit = 0
  .ELSE
  i_irq_exit = i_irq_enter
  .ENDIF
.ENDIF

.IF(!.DEFINED(r_start_addr))
  .ERROR("Required symbol r_start_addr not defined.")
.ENDIF

.IF(r_start_addr == -2 && i_ram_exit != c_rom_config_value)
  .ERROR("Basic start and non-ROM configuration can't be combined.")
.ENDIF

.IF(.DEFINED(i_line_number) && .DEFINED(i_load_addr))
  .ERROR("Basic line number and load address can't be combined.")
.ENDIF

.IF(!.DEFINED(i_line_number))
  i_line_number = 31
.ENDIF

.IF(.DEFINED(i_fourth_offset_table))
encoded_entries = 68
.ELSE
encoded_entries = 52
.ENDIF
; -------------------------------------------------------------------
; -- validate some input parameters ---------------------------------
; -------------------------------------------------------------------
.IF(r_target == 1 || r_target == 16 || r_target == 4)
  .IF(i_ram_exit != 0 && i_ram_exit != 1)
    .ERROR("Symbol i_ram_exit must have a value [0-1].")
  .ENDIF
  .IF(i_ram_enter != 0 && i_ram_enter != 1)
    .ERROR("Symbol i_ram_enter must have a value [0-1].")
  .ENDIF
.ELIF(r_target == 64)
  .IF(i_ram_exit > $38 || i_ram_exit < $34)
    .ERROR("Symbol i_ram_exit must be undefined or be within [$34-$38].")
  .ENDIF
  .IF(i_ram_enter > $38 || i_ram_enter < $34)
    .ERROR("Symbol i_ram_enter must be undefined or be within [$34-$38].")
  .ENDIF
.ELIF(r_target == 128)
  .IF(i_ram_exit > $3f || i_ram_exit < 0)
    .ERROR("Symbol i_ram_exit must be undefined or be within [0-$3f].")
  .ENDIF
  .IF(i_ram_enter > $3f || i_ram_enter < 0)
    .ERROR("Symbol i_ram_enter must be undefined or be within [0-$3f].")
  .ENDIF
.ENDIF

; -------------------------------------------------------------------
; -- convert $0 to $10000 but leave $1 - $ffff ----------------------
; -------------------------------------------------------------------
v_highest_addr = (.INCWORD("crunched_data", -2) + 65535) % 65536 + 1

; -------------------------------------------------------------------
; -- file2_start_hook and stage2_exit_hook --------------------------
; -------------------------------------------------------------------
.IF(!.DEFINED(i_effect_custom) && i_effect2 == 0 && .DEFINED(c_effect_char))
file2_start_hook = 1
  .MACRO("file2_start_hook")
    .IF(v_safety_addr < file2start && ; if we are transferring anyhow
        c_effect_char < v_safety_addr &&
        (file2start - c_effect_char < 257 ||
         file2start - v_safety_addr > 256) &&
        (i_table_addr + 3 * encoded_entries < c_effect_char ||
         i_table_addr > v_highest_addr))
raw_transfer_len = file2start - c_effect_char
lowest_addr = c_effect_char
    .ENDIF
  .ENDMACRO
stage2_exit_hook = 1
  .MACRO("stage2_exit_hook")
    .IF(c_effect_char < lowest_addr || c_effect_char > v_highest_addr)
      .IF(r_target == $bbcb)
        sty c_effect_char
      .ELSE
        stx c_effect_char
      .ENDIF
    .ENDIF
  .ENDMACRO
.ENDIF

; -------------------------------------------------------------------
; -- calculate "*_during" parameters --------------------------------
; -------------------------------------------------------------------
.IF(!.DEFINED(i_ram_during))
  .IF(v_highest_addr > c_end_of_mem_rom)
    i_ram_during = c_ram_config_value
  .ELSE
    i_ram_during = i_ram_enter
  .ENDIF
.ENDIF

.IF(!.DEFINED(i_nmi_during))
  .IF(v_highest_addr > c_end_of_mem_rom)
    i_nmi_during = c_ram_nmi_value
  .ELSE
    i_nmi_during = i_nmi_enter
  .ENDIF
.ENDIF
.IF(!.DEFINED(i_irq_during))
  .IF(i_irq_enter==i_irq_exit && i_ram_during==i_ram_enter && r_target!=1)
    i_irq_during = i_irq_enter
  .ELSE
    i_irq_during = 0
  .ENDIF
.ENDIF

.IF(r_target == 16 || r_target == 4)
  .IF(i_ram_during != 0 && i_ram_during != 1)
    .ERROR("Symbol i_ram_during must have a value [0-1].")
  .ENDIF
.ELIF(r_target == 64)
  .IF(i_ram_during > $38 || i_ram_during < $34)
    .ERROR("Symbol i_ram_during must be undefined or be within [$34-$38].")
  .ENDIF
.ELIF(r_target == 128)
  .IF(i_ram_during > $3f || i_ram_during < 0)
    .ERROR("Symbol i_ram_during must be undefined or be within [0-$3f].")
  .ENDIF
.ENDIF

; -------------------------------------------------------------------
; -- The decruncher enter macro definition --------------------------
; -------------------------------------------------------------------
enter_hook = 1
.MACRO("enter_hook")
  .IF(i_irq_during != i_irq_enter)
    .IF(i_irq_during == 1)
        cli
    .ELSE
        sei
    .ENDIF
  .ENDIF
  .IF(i_nmi_during != i_nmi_enter)
    .INCLUDE("b2d_nmi")
  .ENDIF
  .IF(.DEFINED(i_enter_custom))
    .INCLUDE("enter_custom")
  .ENDIF
  .IF(i_ram_during != i_ram_enter)
    .INCLUDE("b2d_ram")
  .ENDIF
.ENDMACRO
; -------------------------------------------------------------------
; -- The decruncher exit macro definition ---------------------------
; -------------------------------------------------------------------
exit_hook = 1
.MACRO("exit_hook")
  .IF(i_ram_exit != i_ram_during)
    .INCLUDE("d2r_ram")
  .ENDIF
  .IF(.DEFINED(i_exit_custom))
    .INCLUDE("exit_custom")
  .ENDIF
  .IF(i_nmi_exit != i_nmi_during)
    .INCLUDE("d2r_nmi")
  .ENDIF
  .IF(i_irq_exit != i_irq_during)
    .IF(i_irq_exit == 1)
        cli
    .ELSE
        sei
    .ENDIF
  .ENDIF
  .IF(r_start_addr == -2)
    .IF(r_target == $a2)
      .IF(.DEFINED(i_basic_txt_start))
        lda #i_basic_txt_start % 256
        sta <$67
        lda #i_basic_txt_start / 256
        sta <$68
      .ENDIF
      .IF(.DEFINED(i_basic_var_start))
        lda #i_basic_var_start % 256
        sta <$69
        lda #i_basic_var_start / 256
        sta <$6a
      .ENDIF
      .IF(.DEFINED(i_basic_highest_addr))
        lda #(i_basic_highest_addr - 1) % 256
        sta <$73
        lda #(i_basic_highest_addr - 1) / 256
        sta <$74
      .ENDIF
    .ELIF(r_target == $a8)
        .ERROR("Atari target can't handle basic start.")
    .ELIF(r_target == $bbcb)
        .ERROR("BBC Micro B target can't handle basic start.")
    .ELIF(r_target == 1)
      .IF(.DEFINED(i_basic_txt_start))
        lda #i_basic_txt_start % 256
        sta <$9a
        lda #i_basic_txt_start / 256
        sta <$9b
      .ENDIF
      .IF(.DEFINED(i_basic_var_start))
        lda #i_basic_var_start % 256
        sta <$9c
        lda #i_basic_var_start / 256
        sta <$9d
      .ENDIF
      .IF(.DEFINED(i_basic_highest_addr))
        lda #(i_basic_highest_addr - 1) % 256
        sta <$a6
        lda #(i_basic_highest_addr - 1) / 256
        sta <$a7
      .ENDIF
    .ELIF(r_target == 128)
      .IF(.DEFINED(i_basic_txt_start))
        lda #i_basic_txt_start % 256
        sta <$2d
        lda #i_basic_txt_start / 256
        sta <$2e
      .ENDIF
      .IF(.DEFINED(i_basic_var_start))
        lda #i_basic_var_start % 256
        sta $1210
        lda #i_basic_var_start / 256
        sta $1211
      .ENDIF
      .IF(.DEFINED(i_basic_highest_addr))
        lda #i_basic_highest_addr % 256
        sta $1212
        lda #i_basic_highest_addr / 256
        sta $1213
      .ENDIF
    .ELSE
      .IF(.DEFINED(i_basic_txt_start))
        lda #i_basic_txt_start % 256
        sta <$2b
        lda #i_basic_txt_start / 256
        sta <$2c
      .ENDIF
      .IF(.DEFINED(i_basic_var_start))
        lda #i_basic_var_start % 256
        sta <$2d
        lda #i_basic_var_start / 256
        sta <$2e
      .ENDIF
      .IF(.DEFINED(i_basic_highest_addr))
        lda #i_basic_highest_addr % 256
        sta <$37
        lda #i_basic_highest_addr / 256
        sta <$38
      .ENDIF
    .ENDIF
    .IF(r_target == 20 || r_target == 23 || r_target == 52 || r_target == 55)
        jsr $c659               ; init
        jsr $c533               ; regenerate line links
        jmp $c7ae               ; start
    .ELIF(r_target == 4032)
        jsr $B622               ; Reset TXTPTR
        jsr $B5F0               ; CLR
        jsr $B4B6               ; relink
        jmp $B74A
    .ELIF(r_target == 16 || r_target == 4)
        jsr $8bbe               ; init
        jsr $8818               ; regenerate line links and set $2d/$2e
        jsr $f3b5               ; regen color table at $0113
        jmp $8bdc               ; start
    .ELIF(r_target == 64)
        jsr $a659               ; init
        jsr $a533               ; regenerate line links
        jmp $a7ae               ; start
    .ELIF(r_target == 128)
        jsr $5ab5               ; init
        jsr $4f4f               ; regenerate line links and set $1210/$1211
        jmp $4af6               ; start
    .ELIF(r_target == $a2)
        lda #a2relinked % 256   ; set kbd vector
        sta <$36
        lda #a2relinked / 256
        sta <$37
        jmp $d4f2               ; relink, exits by kbd vector
a2relinked:
        jsr $fe89               ; reset kbd vector at $36/$37
        jmp $d566               ; start
    .ELIF(r_target == 1)
        bit $fffc
        bmi oric_ROM11
        jsr $c56f               ; regenerate line links
        jmp $c733               ; start
oric_ROM11:
        jsr $c55f               ; regenerate line links
        jmp $c708               ; start
    .ENDIF
  .ELSE
        jmp r_start_addr
  .ENDIF
.ENDMACRO

; -------------------------------------------------------------------
; -- The decrunch effect macro definition ---------------------------
; -------------------------------------------------------------------
.IF(!.DEFINED(i_effect_speed) || i_effect_speed == 0)
  slow_effect_hook = 1
.ELSE
  fast_effect_hook = 1
.ENDIF
.MACRO("effect_hook")
  .IF(.DEFINED(i_effect_custom))
    .INCLUDE("d2io")
    .INCLUDE("effect_custom")
    .INCLUDE("io2d")
  .ELIF(i_effect2 != -1)
    .INCLUDE("d2io")
    .IF(i_effect2 == 0)
      .IF(r_target == 16 || r_target == 4)
        lda <$fd,x
        sta c_effect_color
      .ELIF(r_target == 1)
        lda <$fd,x
        and #$07
        ora #$10
        sta c_effect_color
      .ELIF(r_target == $bbcb)
        txa
        and #$1f
        ora #$80
        sta c_effect_color
      .ELSE
        stx c_effect_color
      .ENDIF
    .ELIF(i_effect2 == 1)
      .IF(r_target == 20 || r_target == 23 || r_target == 52 || r_target == 55)
        and #$07
        ora #$18
      .ELIF(r_target == 16 || r_target == 4)
        ora #$30
      .ENDIF
        sta c_border_color
    .ELIF(i_effect2 == 2)
      .IF(r_target == 20 || r_target == 23 || r_target == 52 || r_target == 55)
        txa
        and #$07
        ora #$18
        sta c_border_color
      .ELIF(r_target == 16 || r_target == 4)
        lda <$fd,x
        sta c_border_color
      .ELSE
        stx c_border_color
      .ENDIF
    .ELIF(i_effect2 == 3)
      .IF(r_target == 20 || r_target == 23 || r_target == 52 || r_target == 55)
        tya
        and #$07
        ora #$18
        sta c_border_color
      .ELIF(r_target == 16 || r_target == 4)
        sty c_border_color
      .ELSE
        sty c_border_color
      .ENDIF
    .ELSE
      .ERROR("Unknown decrunch effect.")
    .ENDIF
    .INCLUDE("io2d")
  .ENDIF
.ENDMACRO

; -------------------------------------------------------------------
; -- The ram/rom switch macros for decrunching ----------------------
; -------------------------------------------------------------------
.IF(r_target == 1)
; -------------------------------------------------------------------
; -- The ram/rom switch macros for Oric-1 ---------------------------
; -------------------------------------------------------------------
  .MACRO("b2d_nmi")
  .ENDMACRO
  .MACRO("b2d_ram")
    .IF(i_ram_during == c_ram_config_value)
        lda #$84
        sta $0314 ; -- %1xxxxx0x -- RAM at $c000-$10000 -------------
    .ELSE
        lda #$86
        sta $0314 ; -- %xxxxxx1x -- ROM at $c000-$10000 -------------
    .ENDIF
  .ENDMACRO
  .MACRO("d2io")
  .ENDMACRO
  .MACRO("io2d")
  .ENDMACRO
  .MACRO("d2r_ram")
    .IF(i_ram_exit == c_rom_config_value)
        lda #$86
        sta $0314 ; -- %xxxxxx1x -- ROM at $c000-$10000 -------------
    .ELSE
        lda #$84
        sta $0314 ; -- %1xxxxx0x -- RAM at $c000-$10000 -------------
    .ENDIF
  .ENDMACRO
  .MACRO("d2r_nmi")
  .ENDMACRO
.ELIF(r_target == 64)
; -------------------------------------------------------------------
; -- The ram/rom switch macros for c64 ------------------------------
; -------------------------------------------------------------------
  .MACRO("b2d_nmi")
  .ENDMACRO
  .MACRO("b2d_ram")
    .IF(i_ram_during == i_ram_enter + 1)
        inc <$01
    .ELIF(i_ram_during == i_ram_enter - 1)
        dec <$01
    .ELSE
        lda #i_ram_during
        sta <$01
    .ENDIF
  .ENDMACRO
  .MACRO("d2io")
    .IF(i_ram_during == $34 || i_ram_during == $38)
      .IF(i_irq_during == 1)
        sei
      .ENDIF
      .IF(i_ram_during == $34)
        inc <$01
      .ELIF(i_ram_during == $38)
        dec <$01
      .ENDIF
    .ENDIF
  .ENDMACRO
  .MACRO("io2d")
    .IF(i_ram_during == $34 || i_ram_during == $38)
      .IF(i_ram_during == $34)
        dec <$01
      .ELIF(i_ram_during == $38)
        inc <$01
      .ENDIF
      .IF(i_irq_during == 1)
        cli
      .ENDIF
    .ENDIF
  .ENDMACRO
  .MACRO("d2r_ram")
    .IF(i_ram_exit == i_ram_during + 1)
        inc <$01
    .ELIF(i_ram_exit == i_ram_during - 1)
        dec <$01
    .ELSE
        lda #i_ram_exit
        sta <$01
    .ENDIF
  .ENDMACRO
  .MACRO("d2r_nmi")
  .ENDMACRO
.ELIF(r_target == 128)
; -------------------------------------------------------------------
; -- The ram/rom switch macros for c128 -----------------------------
; -------------------------------------------------------------------
  .MACRO("b2d_nmi")
  .ENDMACRO
  .MACRO("b2d_ram")
        lda #i_ram_during
        sta $ff00
  .ENDMACRO
  .MACRO("d2io")
    .IF(i_ram_during == $3f)
      .IF(i_irq_during == 1)
        sei
      .ENDIF
      .IF(i_effect2 == 1)
        pha
      .ENDIF
        lda #c_rom_config_value
        sta $ff00
      .IF(i_effect2 == 1)
        pla
      .ENDIF
    .ENDIF
  .ENDMACRO
  .MACRO("io2d")
    .IF(i_ram_during == $3f)
      .IF(i_effect2 == 1)
        pha
      .ENDIF
        lda #i_ram_during
        sta $ff00
      .IF(i_effect2 == 1)
        pla
      .ENDIF
      .IF(i_irq_during == 1)
        cli
      .ENDIF
    .ENDIF
  .ENDMACRO
  .MACRO("d2r_ram")
        lda #i_ram_exit
        sta $ff00
  .ENDMACRO
  .MACRO("d2r_nmi")
  .ENDMACRO
.ELIF(r_target == 16 || r_target == 4)
; -------------------------------------------------------------------
; -- The ram/rom switch macros for c16/+4 ---------------------------
; -------------------------------------------------------------------
  .MACRO("b2d_nmi")
  .ENDMACRO
  .MACRO("b2d_ram")
    .IF(i_ram_during == c_ram_config_value)
        sta $ff3f
    .ELSE
        sta $ff3e
    .ENDIF
  .ENDMACRO
  .MACRO("d2io")
  .ENDMACRO
  .MACRO("io2d")
  .ENDMACRO
  .MACRO("d2r_ram")
    .IF(i_ram_exit == c_rom_config_value)
        sta $ff3e
    .ELSE
        sta $ff3f
    .ENDIF
  .ENDMACRO
  .MACRO("d2r_nmi")
  .ENDMACRO
.ELIF(r_target == 20 || r_target == 23 || r_target == 52 || r_target == 55 ||
      r_target == 4032)
; -------------------------------------------------------------------
; -- The ram/rom switch macros for c20 and PET 4032 -----------------
; -------------------------------------------------------------------
  .MACRO("b2d_nmi")
  .ENDMACRO
  .MACRO("b2d_ram")
  .ENDMACRO
  .MACRO("d2io")
  .ENDMACRO
  .MACRO("io2d")
  .ENDMACRO
  .MACRO("d2r_ram")
  .ENDMACRO
  .MACRO("d2r_nmi")
  .ENDMACRO
.ELIF(r_target == $a2)
; -------------------------------------------------------------------
; -- The ram/rom switch macros for Apple ----------------------------
; -------------------------------------------------------------------
  .MACRO("b2d_nmi")
  .ENDMACRO
  .MACRO("b2d_ram")
  .ENDMACRO
  .MACRO("d2io")
  .ENDMACRO
  .MACRO("io2d")
  .ENDMACRO
  .MACRO("d2r_ram")
  .ENDMACRO
  .MACRO("d2r_nmi")
  .ENDMACRO
.ELIF(r_target == $a8)
; -------------------------------------------------------------------
; -- The ram/rom switch macros for a8 -------------------------------
; -------------------------------------------------------------------
  .MACRO("b2d_nmi")
        lda #i_nmi_during
        sta $d40e
  .ENDMACRO
  .MACRO("b2d_ram")
        lda #i_ram_during
        sta $d301
  .ENDMACRO
  .MACRO("d2io")
  .ENDMACRO
  .MACRO("io2d")
  .ENDMACRO
  .MACRO("d2r_ram")
        lda #i_ram_exit
        sta $d301
  .ENDMACRO
  .MACRO("d2r_nmi")
        lda #i_nmi_exit
        sta $d40e
  .ENDMACRO
.ELSE
.ELIF(r_target == $bbcb)
; -------------------------------------------------------------------
; -- The ram/rom switch macros for BBC Micro B ----------------------
; -------------------------------------------------------------------
  .MACRO("b2d_nmi")
  .ENDMACRO
  .MACRO("b2d_ram")
  .ENDMACRO
  .MACRO("d2io")
  .ENDMACRO
  .MACRO("io2d")
  .ENDMACRO
  .MACRO("d2r_ram")
  .ENDMACRO
  .MACRO("d2r_nmi")
  .ENDMACRO
  .ERROR("Unhandled target for macro definitions.")
.ENDIF
; -------------------------------------------------------------------
; -- End of bank switch definitions ---------------------------------
; -------------------------------------------------------------------
; -------------------------------------------------------------------
; -- Start of file header stuff -------------------------------------
; -------------------------------------------------------------------
.IF(r_target == 1)
; -------------------------------------------------------------------
; -- Oric-1 file header stuff ---------------------------------------
; -------------------------------------------------------------------
  .IF(r_start_addr == -2)
    .IF(!.DEFINED(i_raw) || i_raw == 0)
        .BYTE($16,$16,$16,$24,$00,$00,$00,$c7)
        .BYTE((o1_end - 1) / 256, (o1_end - 1) % 256)
        .BYTE(c_basic_start / 256, c_basic_start % 256)
        .BYTE(0, 0)
    .ENDIF
        .ORG(c_basic_start)
lowest_addr_out:
        .WORD(basic_end, i_line_number)
        .BYTE($bf, o1_start / 1000 % 10 + 48, o1_start / 100 % 10 + 48)
        .BYTE(o1_start / 10 % 10 + 48, o1_start % 10 + 48, 0)
basic_end:
        .BYTE(0,0)
  .ELSE
    .IF(!.DEFINED(i_raw) || i_raw == 0)
        .BYTE($16,$16,$16,$24,$00,$00,$80,$c7)
        .BYTE((o1_end - 1) / 256, (o1_end - 1) % 256)
        .BYTE(o1_start / 256, o1_start % 256)
        .BYTE(0, 0)
    .ENDIF
    .IF(!.DEFINED(i_load_addr))
        .ORG($0500)
    .ELSE
        .ORG(i_load_addr)
    .ENDIF
lowest_addr_out:
  .ENDIF
o1_start:
.ELIF(r_target == 20 || r_target == 23 || r_target == 52 || r_target == 55 ||
      r_target == 16 || r_target == 4 || r_target == 64 || r_target == 128 ||
      r_target == 4032)
; -------------------------------------------------------------------
; -- Commodore file header stuff ------------------------------------
; -------------------------------------------------------------------
  .IF(.DEFINED(i_load_addr))
    .IF(!.DEFINED(i_raw) || i_raw == 0)
        .WORD(i_load_addr)
    .ENDIF
        .ORG(i_load_addr)
lowest_addr_out:
  .ELSE
    .IF(!.DEFINED(i_raw) || i_raw == 0)
        .WORD(c_basic_start)
    .ENDIF
        .ORG(c_basic_start)
lowest_addr_out:
        .WORD(basic_end, i_line_number)
        .BYTE($9e, cbm_start / 1000 % 10 + 48, cbm_start / 100 % 10 + 48)
        .BYTE(cbm_start / 10 % 10 + 48, cbm_start % 10 + 48, 0)
basic_end:
trqwrk ?= 2
    .IF(r_target == 16 || r_target == 4 ||
        r_target == 128 || (transfer_len - trqwrk) % 256 != 0)
        .BYTE(0,0)
trqwrk = 2
    .ELSE
trqwrk = 0
    .ENDIF
; -------------------------------------------------------------------
cbm_start:
  .ENDIF
.ELIF(r_target == $a8)
; -------------------------------------------------------------------
; -- Atari file header stuff ------------------------------------
; -------------------------------------------------------------------
  .IF(!.DEFINED(i_raw) || i_raw == 0)
        .WORD($FFFF, a8_start, a8_end - 1)
  .ENDIF
  .IF(!.DEFINED(i_load_addr))
        .ORG($2000)
  .ELSE
        .ORG(i_load_addr)
  .ENDIF
lowest_addr_out:
a8_start:
.ELIF(r_target == $a2)
; -------------------------------------------------------------------
; -- Apple file header stuff ------------------------------------
; -------------------------------------------------------------------
  .IF(!.DEFINED(i_a2_file_type))
    .IF(.DEFINED(i_load_addr))
i_a2_file_type = 6              ; binary
    .ELSE
i_a2_file_type = $fc            ; Applesoft basic
    .ENDIF
  .ENDIF
  .IF(!.DEFINED(i_raw) || i_raw == 0)
        .ORG(0)
as_start:
        ;; prodos file, AppleSingle header
        ;; http://apple2online.com/web_documents/ft_e0.0001_applesingle.pdf
        ;; http://kaiser-edv.de/documents/AppleSingle_AppleDouble.pdf
        .BYTE($00, $05, $16, $00) ; magic
        .BYTE($00, $02, $00, $00) ; version
        .BYTE($00, $00, $00, $00, $00, $00, $00, $00) ; filler
        .BYTE($00, $00, $00, $00, $00, $00, $00, $00)
        .BYTE($00, $02)         ; number of entries
        ;; data entry descriptor
        .BYTE($00, $00, $00, $01) ; entry ID
        .BYTE($00, $00, $00, as_data_entry - as_start) ; offset
        .BYTE($00, $00,
              (a2_end - a2_load) / 256, (a2_end - a2_load) % 256) ; length
        ;; PRODOS entry descriptor
        .BYTE($00, $00, $00, $0b) ; entry ID
        .BYTE($00, $00, $00, as_prodos_entry - as_start) ; offset
        .BYTE($00, $00, $00, $08) ; length
as_prodos_entry:
        ;; PRODOS entry
        .BYTE($00, $c3) ; access
        .BYTE($00, i_a2_file_type) ; filetype
    .IF(.DEFINED(i_load_addr))
        .BYTE($00, $00,
              i_load_addr / 256, i_load_addr % 256) ; aux file type
    .ELSE
        ;; Applesoft basic file
        .BYTE($00, $00,
              c_basic_start / 256, c_basic_start % 256) ; aux file type
    .ENDIF
as_data_entry:
  .ENDIF
  .IF(.DEFINED(i_load_addr))
        .ORG(i_load_addr)
lowest_addr_out:
a2_load:
  .ELSE
        .ORG(c_basic_start)
lowest_addr_out:
a2_load:
        .WORD(basic_end, i_line_number)
        .BYTE($8c, a2_start / 1000 % 10 + 48, a2_start / 100 % 10 + 48)
        .BYTE(a2_start / 10 % 10 + 48, a2_start % 10 + 48, 0)
basic_end:
    .IF(transfer_len % 256 != 0)
        .BYTE(0,0)
    .ENDIF
  .ENDIF
; -------------------------------------------------------------------
a2_start:
  .IF(.DEFINED(i_a2_disable_dos))
        lda #$00
        jsr $fe95
        lda #$00
        jsr $fe8b
  .ENDIF
.ELIF(r_target == $bbcb)
; -- No header at all for BBC b
  .IF(.DEFINED(i_load_addr))
        .ORG(i_load_addr)
  .ELSE
        .ORG(c_basic_start)
  .ENDIF
lowest_addr_out:
.ELSE
  .ERROR("Unhandled target for file header stuff")
.ENDIF
; -------------------------------------------------------------------
; -- End of file header stuff ---------------------------------------
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; -- Start of the actual decruncher ---------------------------------
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; -- required symbols:
; --
; --  zp_len_lo                 A zerpoage location used for a byte.
; --  zp_len_hi                 A zerpoage location used for a byte.
; --  zp_src_lo + zp_src_hi     A zeropage location used for a word.
; --  zp_bits_hi                A zeropage location used for a byte.
; --  v_safety_addr
; --  i_table_addr
; --
; -- optional symbols
; --  enter_hook                must exit with y == 0.
; --  fast_effect_hook          must not touch x or y.
; --  slow_effect_hook          must not touch x or y.
; --  exit_hook
; --  stage2_exit_hook          must exit with x == 0 and y == 0.
; --
; -- macros:
; --  enter_hook
; --  effect_hook
; --  exit_hook
; --  stage2_exit_hook
; -------------------------------------------------------------------

zp_bitbuf  = $fd
zp_dest_lo = zp_bitbuf + 1      ; dest addr lo
zp_dest_hi = zp_bitbuf + 2      ; dest addr hi

; -------------------------------------------------------------------
; -- start of stage 1 -----------------------------------------------
; -------------------------------------------------------------------
max_transfer_len = .INCLEN("crunched_data") - 5
        ldy #transfer_len % 256
        .IF(.DEFINED(enter_hook))
          .INCLUDE("enter_hook")
        .ENDIF
        tsx
cploop:
        lda stage2end - 4,x
        sta $0100 - 4,x
        dex
        bne cploop
.IF(transfer_len > 256)
        ldx #transfer_len / 256 + 1
.ENDIF
.IF(transfer_len != max_transfer_len)
        jmp stage2start
.ENDIF
stage1end:
; -------------------------------------------------------------------
; -- end of stage 1 -------------------------------------------------
; -------------------------------------------------------------------
; -------------------------------------------------------------------
; -- start of file part 2 -------------------------------------------
; -------------------------------------------------------------------
file2start:
.IF(.DEFINED(file2_start_hook))
  .INCLUDE("file2_start_hook")
.ENDIF
.IF(!.DEFINED(raw_transfer_len))
  .IF(v_safety_addr > file2start || v_highest_addr < file2start)
raw_transfer_len = 0
lowest_addr = file2start
  .ELSE
raw_transfer_len = file2start - v_safety_addr
lowest_addr = v_safety_addr
  .ENDIF
.ENDIF

.IF(raw_transfer_len > max_transfer_len)
transfer_len = max_transfer_len
.ELSE
transfer_len = raw_transfer_len
.ENDIF
        .INCBIN("crunched_data", transfer_len + 2, max_transfer_len - transfer_len)
file2end:
; -------------------------------------------------------------------
; -- end of file part 2 ---------------------------------------------
; -------------------------------------------------------------------
; -------------------------------------------------------------------
; -- start of stage 2 -----------------------------------------------
; -------------------------------------------------------------------
.IF(transfer_len == 0)
stage2start:
.ELIF(transfer_len < 257)
stage2start:
copy1_loop:
  .IF(transfer_len == 256)
        lda file1start,y
        sta lowest_addr,y
  .ELSE
        lda file1start - 1,y
        sta lowest_addr - 1,y
  .ENDIF
        dey
        bne copy1_loop
.ELSE
        bne stage2start
copy2_loop1:
        dey
lda_fixup:
        lda file1start + transfer_len / 256 * 256,y
sta_fixup:
        sta lowest_addr + transfer_len / 256 * 256,y
stage2start:
        tya
        bne copy2_loop1
        dec lda_fixup + 2
        dec sta_fixup + 2
        dex
        bne copy2_loop1
.ENDIF
; -------------------------------------------------------------------
tabl_bi = i_table_addr
tabl_lo = i_table_addr + encoded_entries
tabl_hi = i_table_addr + 2 * encoded_entries
        clc
table_gen:
        tax
        tya
        and #$0f
        sta tabl_lo,y
        beq shortcut            ; start a new sequence
; -------------------------------------------------------------------
        txa
        adc tabl_lo - 1,y
        sta tabl_lo,y
        lda <zp_len_hi
        adc tabl_hi - 1,y
shortcut:
        sta tabl_hi,y
; -------------------------------------------------------------------
        lda #$01
        sta <zp_len_hi
        lda #$78                ; %01111000
        jsr get_bits
; -------------------------------------------------------------------
        lsr
        tax
        beq rolled
        php
rolle:
        asl <zp_len_hi
        sec
        ror
        dex
        bne rolle
        plp
rolled:
        ror
        sta tabl_bi,y
        bmi no_fixup_lohi
        lda <zp_len_hi
        stx <zp_len_hi
        .BYTE($24)             ; bit zero page
no_fixup_lohi:
        txa
; -------------------------------------------------------------------
        iny
        cpy #encoded_entries
        bne table_gen
        .IF(.DEFINED(stage2_exit_hook))
          .INCLUDE("stage2_exit_hook")
        .ENDIF
; ###################################################################
.IF(i_perf == -1)
; ###################################################################
; ## tiny start #####################################################
; ###################################################################
        ldy #(v_highest_addr - 1) % 256
        jmp begin
; -------------------------------------------------------------------
; -- end of stage 2 -------------------------------------------------
; -------------------------------------------------------------------
        .INCBIN("crunched_data", max_transfer_len + 2, 1) ; => zp_bitbuf
        .WORD((((v_highest_addr - 1) % 65536) / 256) * 256) ; => zp_dest_lo/hi
stage2end:
        .ORG($0100)
; -------------------------------------------------------------------
; -- start of stage 3 -----------------------------------------------
; -------------------------------------------------------------------
stage3start:
; -------------------------------------------------------------------
; get bits (26 bytes)
;
get_bits:
        adc #$80                ; needs c=0, affects v
        asl
        bpl gb_skip
gb_next:
        asl <zp_bitbuf
        bne gb_no_refill
        pha
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
        pla
gb_no_refill:
        rol
        bmi gb_next
gb_skip:
        bvs gb_get_hi
        rts
gb_get_hi:
        sec
        sta <zp_bits_hi
; -------------------------------------------------------------------
; get crunched byte (15 bytes) + hooks
;
get_crunched_byte:
        .IF(.DEFINED(fast_effect_hook))
          .INCLUDE("effect_hook")
        .ENDIF
        lda get_byte_fixup + 1
        bne get_byte_skip_hi
        dec get_byte_fixup + 2
        .IF(.DEFINED(slow_effect_hook))
          .INCLUDE("effect_hook")
        .ENDIF
get_byte_skip_hi:
        dec get_byte_fixup + 1
get_byte_fixup:
        lda lowest_addr + max_transfer_len
        rts
; -------------------------------------------------------------------
; fetch sequence length index (13 bytes)
; x must be #0 when entering and contains the length index + 1
; when exiting or 0 for literal byte
begin:
.IF(.DEFINED(i_reuse_offset))
        ror <zp_ro_state
.ENDIF
next_bit:
        asl <zp_bitbuf
        bne nofetch8
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
nofetch8:
        inx
        bcc next_bit
; -------------------------------------------------------------------
; check for literal byte (4 bytes)
;
        cpx #1
        beq copy_next
; -------------------------------------------------------------------
; check for decrunch done and literal sequences (4 bytes)
;
        cpx #$12
        bcs exit_or_lit_seq
; -------------------------------------------------------------------
; calulate length of sequence (zp_len) (26(16) bytes)
; exit with len_lo in X
;
        lda #0
        sta <zp_bits_hi
        lda tabl_bi - 2,x
        jsr get_bits
        adc tabl_lo - 2,x       ; we have now calculated zp_len_lo
        sta <zp_len_lo
.IF(!.DEFINED(i_max_sequence_length_256))
        lda <zp_bits_hi
        adc tabl_hi - 2,x       ; c = 0 after this.
        sta <zp_len_hi
        ldx <zp_len_lo
.ELSE
        tax
.ENDIF
.IF(!.DEFINED(i_max_sequence_length_256))
        lda #0
.ENDIF
.IF(.DEFINED(i_reuse_offset))
; -------------------------------------------------------------------
; here we decide to reuse latest offset or not (13(15) bytes)
;
        bit <zp_ro_state
        bpl no_reuse
        bvs no_reuse
.IF(.DEFINED(i_max_sequence_length_256))
        lda #$00                ; fetch one bit
.ENDIF
        jsr gb_next
        bne copy_next           ; bit != 0 => C=0, reuse previous offset
no_reuse:
.ENDIF
; -------------------------------------------------------------------
; here we decide what offset table to use (17(15) bytes)
;
.IF(!.DEFINED(i_max_sequence_length_256))
        sta <zp_bits_hi
.ENDIF
        lda #$f1
.IF(.DEFINED(i_fourth_offset_table))
        cpx #$04
.ELSE
        cpx #$03
.ENDIF
        bcs gbnc2_next
        lda tabl_bit - 1,x
gbnc2_next:
        clv
        jsr gb_next
        clc
        tax
; -------------------------------------------------------------------
; calulate absolute offset (zp_src) (20 bytes)
;
        lda tabl_bi,x
        jsr get_bits
        adc tabl_lo,x
        sta <zp_src_lo
        lda <zp_bits_hi
        adc tabl_hi,x
        adc <zp_dest_hi
        sta <zp_src_hi
; -------------------------------------------------------------------
; prepare for copy loop (2 bytes)
;
pre_copy:
        ldx <zp_len_lo
; -------------------------------------------------------------------
; main copy loop (31(25) bytes)
;
copy_next:
        bcs get_literal_byte
        lda (zp_src_lo),y
literal_byte_gotten:
        sta (zp_dest_lo),y
        tya
        bne copy_skip_hi
        dec <zp_dest_hi
        dec <zp_src_hi
copy_skip_hi:
        dey
        dex
        bne copy_next
.IF(!.DEFINED(i_max_sequence_length_256))
        lda <zp_len_hi
.ENDIF
        beq begin
.IF(!.DEFINED(i_max_sequence_length_256))
        dec <zp_len_hi
        jmp copy_next
.ENDIF
get_literal_byte:
        jsr get_crunched_byte
        bcs literal_byte_gotten
; -------------------------------------------------------------------
; exit or literal sequence handling (16(12) bytes)
;
exit_or_lit_seq:
.IF(.DEFINED(i_literal_sequences_used))
        beq decr_exit
        jsr get_crunched_byte
  .IF(!.DEFINED(i_max_sequence_length_256))
        sta <zp_len_hi
  .ENDIF
        jsr get_crunched_byte
        tax
        bcs copy_next
decr_exit:
.ENDIF
.IF(.DEFINED(exit_hook))
  .INCLUDE("exit_hook")
.ELSE
        rts
.ENDIF
.IF(.DEFINED(i_fourth_offset_table))
; -------------------------------------------------------------------
; the static stable used for bits+offset for lengths 1, 2 and 3 (3 bytes)
; bits 2, 4, 4 and offsets 64, 48, 32 corresponding to
; %11010000, %11110011, %11110010
tabl_bit:
        .BYTE($d0, $f3, $f2)
.ELSE
; -------------------------------------------------------------------
; the static stable used for bits+offset for lengths 1 and 2 (2 bytes)
; bits 2, 4 and offsets 48, 32 corresponding to %11001100, %11110010
tabl_bit:
        .BYTE($cc, $f2)
.ENDIF
; ###################################################################
; ## tiny end #######################################################
; ###################################################################
.ELIF(i_perf == 0)
; ###################################################################
; ## small start ####################################################
; ###################################################################
        ldy #(v_highest_addr - 1) % 256
        txa
        jmp begin_stx
; -------------------------------------------------------------------
; -- end of stage 2 -------------------------------------------------
; -------------------------------------------------------------------
        .INCBIN("crunched_data", max_transfer_len + 2, 1) ; => zp_bitbuf
        .WORD((((v_highest_addr - 1) % 65536) / 256) * 256) ; => zp_dest_lo/hi
stage2end:
        .ORG($0100)
; -------------------------------------------------------------------
; -- start of stage 3 -----------------------------------------------
; -------------------------------------------------------------------
stage3start:
; -------------------------------------------------------------------
; get bits (26 bytes)
;
get_bits:
        adc #$80                ; needs c=0, affects v
        asl
        bpl gb_skip
gb_next:
        asl <zp_bitbuf
        bne gb_no_refill
        pha
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
        pla
gb_no_refill:
        rol
        bmi gb_next
gb_skip:
        bvs gb_get_hi
        rts
gb_get_hi:
        sec
        sta <zp_bits_hi
; -------------------------------------------------------------------
; get crunched byte (15 bytes) + hooks
;
get_crunched_byte:
        .IF(.DEFINED(fast_effect_hook))
          .INCLUDE("effect_hook")
        .ENDIF
        lda get_byte_fixup + 1
        bne get_byte_skip_hi
        dec get_byte_fixup + 2
        .IF(.DEFINED(slow_effect_hook))
          .INCLUDE("effect_hook")
        .ENDIF
get_byte_skip_hi:
        dec get_byte_fixup + 1
get_byte_fixup:
        lda lowest_addr + max_transfer_len
        rts
; -------------------------------------------------------------------
; copy one literal byte to destination (11 bytes)
;
literal_start1:
        jsr get_crunched_byte
        sta (zp_dest_lo),y
        tya
        bne no_hi_decr
        dec <zp_dest_hi
.IF(.DEFINED(i_reuse_offset))
        dec <zp_src_hi
.ENDIF
no_hi_decr:
        dey
; -------------------------------------------------------------------
; fetch sequence length index (14(16) bytes)
; x must be #0 when entering and contains the length index + 1
; when exiting or 0 for literal byte
begin:
.IF(.DEFINED(i_reuse_offset))
        ror <zp_ro_state
.ENDIF
        dex
no_literal1:
        asl <zp_bitbuf
        bne nofetch8
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
nofetch8:
        inx
        bcc no_literal1
; -------------------------------------------------------------------
; check for literal byte (2 bytes)
;
        beq literal_start1
; -------------------------------------------------------------------
; check for decrunch done and literal sequences (4 bytes)
;
        cpx #$11
        bcs exit_or_lit_seq
; -------------------------------------------------------------------
; calulate length of sequence (zp_len) (22(12) bytes)
;
        lda tabl_bi - 1,x
        jsr get_bits
        adc tabl_lo - 1,x       ; we have now calculated zp_len_lo
        sta <zp_len_lo
.IF(!.DEFINED(i_max_sequence_length_256))
        lda <zp_bits_hi
        adc tabl_hi - 1,x       ; c = 0 after this.
        sta <zp_len_hi
        ldx <zp_len_lo
.ELSE
        tax
.ENDIF
.IF(!.DEFINED(i_max_sequence_length_256))
        lda #0
.ENDIF
.IF(.DEFINED(i_reuse_offset))
; -------------------------------------------------------------------
; here we decide to reuse latest offset or not (13(15) bytes)
;
        bit <zp_ro_state
        bpl no_reuse
        bvs no_reuse
.IF(.DEFINED(i_max_sequence_length_256))
        lda #$00                ; fetch one bit
.ENDIF
        jsr gb_next
        bne copy_next           ; bit != 0 => C=0, reuse previous offset
no_reuse:
.ENDIF
; -------------------------------------------------------------------
; here we decide what offset table to use (17(15) bytes)
;
.IF(!.DEFINED(i_max_sequence_length_256))
        sta <zp_bits_hi
.ENDIF
        lda #$f1
.IF(.DEFINED(i_fourth_offset_table))
        cpx #$04
.ELSE
        cpx #$03
.ENDIF
        bcs gbnc2_next
        lda tabl_bit - 1,x
gbnc2_next:
        clv
        jsr gb_next
        clc
        tax
; -------------------------------------------------------------------
; calulate absolute offset (zp_src) (24 bytes)
;
.IF(!.DEFINED(i_max_sequence_length_256))
        lda #0
        sta <zp_bits_hi
.ENDIF
        lda tabl_bi,x
        jsr get_bits
        adc tabl_lo,x
        sta <zp_src_lo
        lda <zp_bits_hi
        adc tabl_hi,x
        adc <zp_dest_hi
        sta <zp_src_hi
; -------------------------------------------------------------------
; prepare for copy loop (2 bytes)
;
pre_copy:
        ldx <zp_len_lo
; -------------------------------------------------------------------
; main copy loop (31(25) bytes)
;
copy_next:
.IF(.DEFINED(i_literal_sequences_used))
        bcs get_literal_byte
.ENDIF
        lda (zp_src_lo),y
literal_byte_gotten:
        sta (zp_dest_lo),y
        tya
        bne copy_skip_hi
        dec <zp_dest_hi
        dec <zp_src_hi
copy_skip_hi:
        dey
        dex
        bne copy_next
.IF(!.DEFINED(i_max_sequence_length_256))
        lda <zp_len_hi
.ENDIF
begin_stx:
        stx <zp_bits_hi
        beq begin
.IF(!.DEFINED(i_max_sequence_length_256))
        dec <zp_len_hi
        jmp copy_next
.ENDIF
.IF(.DEFINED(i_literal_sequences_used))
get_literal_byte:
        jsr get_crunched_byte
        bcs literal_byte_gotten
.ENDIF
; -------------------------------------------------------------------
; exit or literal sequence handling (16(12) bytes)
;
exit_or_lit_seq:
.IF(.DEFINED(i_literal_sequences_used))
        beq decr_exit
        jsr get_crunched_byte
  .IF(!.DEFINED(i_max_sequence_length_256))
        sta <zp_len_hi
  .ENDIF
        jsr get_crunched_byte
        tax
        bcs copy_next
decr_exit:
.ENDIF
.IF(.DEFINED(exit_hook))
  .INCLUDE("exit_hook")
.ELSE
        rts
.ENDIF
.IF(.DEFINED(i_fourth_offset_table))
; -------------------------------------------------------------------
; the static stable used for bits+offset for lengths 1, 2 and 3 (3 bytes)
; bits 2, 4, 4 and offsets 64, 48, 32 corresponding to
; %11010000, %11110011, %11110010
tabl_bit:
        .BYTE($d0, $f3, $f2)
.ELSE
; -------------------------------------------------------------------
; the static stable used for bits+offset for lengths 1 and 2 (2 bytes)
; bits 2, 4 and offsets 48, 32 corresponding to %11001100, %11110010
tabl_bit:
        .BYTE($cc, $f2)
.ENDIF
; ###################################################################
; ## small end ######################################################
; ###################################################################
.ELIF(i_perf == 1)
; ###################################################################
; ## quick start ####################################################
; ###################################################################
        ldy #(v_highest_addr - 1) % 256
        txa
        jmp begin_stx
; -------------------------------------------------------------------
; -- end of stage 2 -------------------------------------------------
; -------------------------------------------------------------------
        .INCBIN("crunched_data", max_transfer_len + 2, 1) ; => zp_bitbuf
        .WORD((((v_highest_addr - 1) % 65536) / 256) * 256) ; => zp_dest_lo/hi
stage2end:
        .ORG($0100)
; -------------------------------------------------------------------
; -- start of stage 3 -----------------------------------------------
; -------------------------------------------------------------------
stage3start:
; -------------------------------------------------------------------
; get bits (26 bytes)
;
get_bits:
        adc #$80                ; needs c=0, affects v
        asl
        bpl gb_skip
gb_next:
        asl <zp_bitbuf
        bne gb_no_refill
        pha
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
        pla
gb_no_refill:
        rol
        bmi gb_next
gb_skip:
        bvs gb_get_hi
        rts
gb_get_hi:
        sec
        sta <zp_bits_hi
; -------------------------------------------------------------------
; get crunched byte (15 bytes) + hooks
;
get_crunched_byte:
        .IF(.DEFINED(fast_effect_hook))
          .INCLUDE("effect_hook")
        .ENDIF
        lda get_byte_fixup + 1
        bne get_byte_skip_hi
        dec get_byte_fixup + 2
        .IF(.DEFINED(slow_effect_hook))
          .INCLUDE("effect_hook")
        .ENDIF
get_byte_skip_hi:
        dec get_byte_fixup + 1
get_byte_fixup:
        lda lowest_addr + max_transfer_len
        rts
; -------------------------------------------------------------------
; copy one literal byte to destination (11 bytes)
;
literal_start1:
        jsr get_crunched_byte
        sta (zp_dest_lo),y
        tya
        bne no_hi_decr
        dec <zp_dest_hi
.IF(.DEFINED(i_reuse_offset))
        dec <zp_src_hi
.ENDIF
no_hi_decr:
        dey
; -------------------------------------------------------------------
; fetch sequence length index (14(16) bytes)
; x must be #0 when entering and contains the length index + 1
; when exiting or 0 for literal byte
begin:
.IF(.DEFINED(i_reuse_offset))
        ror <zp_ro_state
.ENDIF
        dex
no_literal1:
        asl <zp_bitbuf
        bne nofetch8
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
nofetch8:
        inx
        bcc no_literal1
; -------------------------------------------------------------------
; check for literal byte (2 bytes)
;
        beq literal_start1
; -------------------------------------------------------------------
; check for decrunch done and literal sequences (4 bytes)
;
        cpx #$11
        bcs exit_or_lit_seq
; -------------------------------------------------------------------
; calulate length of sequence (zp_len) (22(12) bytes)
;
        lda tabl_bi - 1,x
        jsr get_bits
        adc tabl_lo - 1,x       ; we have now calculated zp_len_lo
        sta <zp_len_lo
.IF(!.DEFINED(i_max_sequence_length_256))
        lda <zp_bits_hi
        adc tabl_hi - 1,x       ; c = 0 after this.
        sta <zp_len_hi
        ldx <zp_len_lo
.ELSE
        tax
.ENDIF
.IF(!.DEFINED(i_max_sequence_length_256))
        lda #0
.ENDIF
.IF(.DEFINED(i_reuse_offset))
; -------------------------------------------------------------------
; here we decide to reuse latest offset or not (4 bytes)
        bit <zp_ro_state
        bmi test_reuse
no_reuse:
.ENDIF
; -------------------------------------------------------------------
; here we decide what offset table to use (27(25) bytes)
;
.IF(!.DEFINED(i_max_sequence_length_256))
        sta <zp_bits_hi
.ENDIF
        lda #$e1
.IF(.DEFINED(i_fourth_offset_table))
        cpx #$04
.ELSE
        cpx #$03
.ENDIF
        bcs gbnc2_next
        lda tabl_bit - 1,x
gbnc2_next:
        asl <zp_bitbuf
        bne gbnc2_ok
        tax
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
        txa
gbnc2_ok:
        rol
        bcs gbnc2_next
        tax
; -------------------------------------------------------------------
; calulate absolute offset (zp_src) (20 bytes)
;
        lda tabl_bi,x
        jsr get_bits
        adc tabl_lo,x
        sta <zp_src_lo
        lda <zp_bits_hi
        adc tabl_hi,x
        adc <zp_dest_hi
        sta <zp_src_hi
; -------------------------------------------------------------------
; prepare for copy loop (4(2) bytes)
;
pre_copy:
        ldx <zp_len_lo
; -------------------------------------------------------------------
; main copy loop (31(25) bytes)
;
copy_next:
.IF(.DEFINED(i_literal_sequences_used))
        bcs get_literal_byte
.ENDIF
        lda (zp_src_lo),y
literal_byte_gotten:
        sta (zp_dest_lo),y
        tya
        bne copy_skip_hi
        dec <zp_dest_hi
        dec <zp_src_hi
copy_skip_hi:
        dey
        dex
        bne copy_next
.IF(!.DEFINED(i_max_sequence_length_256))
        lda <zp_len_hi
.ENDIF
begin_stx:
        stx <zp_bits_hi
        beq begin
.IF(!.DEFINED(i_max_sequence_length_256))
        dec <zp_len_hi
        jmp copy_next
.ENDIF
.IF(.DEFINED(i_literal_sequences_used))
get_literal_byte:
        jsr get_crunched_byte
        bcs literal_byte_gotten
.ENDIF
.IF(.DEFINED(i_reuse_offset))
; -------------------------------------------------------------------
; test for offset reuse (11 bytes)
;
test_reuse:
        bvs no_reuse
.IF(.DEFINED(i_max_sequence_length_256))
        lda #$00                ; fetch one bit
.ENDIF
        jsr gb_next
        beq no_reuse            ; bit == 0 => C=0, no reuse
        bne copy_next           ; bit != 0 => C=0, reuse previous offset
.ENDIF
; -------------------------------------------------------------------
; exit or literal sequence handling (16(12) bytes)
;
exit_or_lit_seq:
.IF(.DEFINED(i_literal_sequences_used))
        beq decr_exit
        jsr get_crunched_byte
  .IF(!.DEFINED(i_max_sequence_length_256))
        sta <zp_len_hi
  .ENDIF
        jsr get_crunched_byte
        tax
        bcs copy_next
decr_exit:
.ENDIF
.IF(.DEFINED(exit_hook))
  .INCLUDE("exit_hook")
.ELSE
        rts
.ENDIF
.IF(.DEFINED(i_fourth_offset_table))
; -------------------------------------------------------------------
; the static stable used for bits+offset for lengths 1, 2 and 3 (3 bytes)
; bits 2, 4, 4 and offsets 64, 48, 32 corresponding to
; %10010000, %11100011, %11100010
tabl_bit:
        .BYTE($90, $e3, $e2)
.ELSE
; -------------------------------------------------------------------
; the static stable used for bits+offset for lengths 1 and 2 (2 bytes)
; bits 2, 4 and offsets 48, 32 corresponding to %10001100, %11100010
tabl_bit:
        .BYTE($8c, $e2)
.ENDIF
; ###################################################################
; ## quick end ######################################################
; ###################################################################
.ELIF(i_perf == 2)
; ###################################################################
; ## fast start #####################################################
; ###################################################################
        ldy #v_highest_addr % 256
        txa
        jmp begin_stx
; -------------------------------------------------------------------
; -- end of stage 2 -------------------------------------------------
; -------------------------------------------------------------------
        .INCBIN("crunched_data", max_transfer_len + 2, 1) ; => zp_bitbuf
        .WORD(((v_highest_addr % 65536) / 256) * 256) ; => zp_dest_lo/hi
stage2end:
        .ORG($0100)
; -------------------------------------------------------------------
; -- start of stage 3 -----------------------------------------------
; -------------------------------------------------------------------
stage3start:
; -------------------------------------------------------------------
; get bits (26 bytes)
;
get_bits:
        adc #$80                ; needs c=0, affects v
        asl
        bpl gb_skip
gb_next:
        asl <zp_bitbuf
        bne gb_no_refill
        pha
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
        pla
gb_no_refill:
        rol
        bmi gb_next
gb_skip:
        bvs gb_get_hi
        rts
gb_get_hi:
        sec
        sta <zp_bits_hi
; -------------------------------------------------------------------
; get crunched byte (15 bytes) + hooks
;
get_crunched_byte:
        .IF(.DEFINED(fast_effect_hook))
          .INCLUDE("effect_hook")
        .ENDIF
        lda get_byte_fixup + 1
        bne get_byte_skip_hi
        dec get_byte_fixup + 2
        .IF(.DEFINED(slow_effect_hook))
          .INCLUDE("effect_hook")
        .ENDIF
get_byte_skip_hi:
        dec get_byte_fixup + 1
get_byte_fixup:
        lda lowest_addr + max_transfer_len
        rts
; -------------------------------------------------------------------
; copy one literal byte to destination (11 bytes)
;
literal_start1:
        tya
        bne no_hi_decr
        dec <zp_dest_hi
.IF(.DEFINED(i_reuse_offset))
        dec <zp_src_hi
.ENDIF
no_hi_decr:
        dey
        jsr get_crunched_byte
        sta (zp_dest_lo),y
; -------------------------------------------------------------------
; fetch sequence length index (22(24) bytes)
; x must be #0 when entering and contains the length index + 1
; when exiting or 0 for literal byte
begin:
.IF(.DEFINED(i_reuse_offset))
        ror <zp_ro_state
.ENDIF
        asl <zp_bitbuf
        beq fetch8
        bcs literal_start1
        lda <zp_bitbuf
        inx
no_literal1:
        asl
        bne nofetch8
fetch8:
        jsr get_crunched_byte
        rol
nofetch8:
        inx
        bcc no_literal1
        sta <zp_bitbuf
        dex
; -------------------------------------------------------------------
; check for literal byte (2 bytes)
;
        beq literal_start1
; -------------------------------------------------------------------
; check for decrunch done and literal sequences (4 bytes)
;
        cpx #$11
        bcs exit_or_lit_seq
; -------------------------------------------------------------------
; calulate length of sequence (zp_len) (22(12) bytes)
;
        lda tabl_bi - 1,x
        jsr get_bits
        adc tabl_lo - 1,x       ; we have now calculated zp_len_lo
        sta <zp_len_lo
.IF(!.DEFINED(i_max_sequence_length_256))
        lda <zp_bits_hi
        adc tabl_hi - 1,x       ; c = 0 after this.
        sta <zp_len_hi
        ldx <zp_len_lo
.ELSE
        tax
.ENDIF
.IF(!.DEFINED(i_max_sequence_length_256))
        lda #0
.ENDIF
.IF(.DEFINED(i_reuse_offset))
; -------------------------------------------------------------------
; here we decide to reuse latest offset or not (4 bytes)
        bit <zp_ro_state
        bmi test_reuse
no_reuse:
.ENDIF
; -------------------------------------------------------------------
; here we decide what offset table to use (27(25) bytes)
;
.IF(!.DEFINED(i_max_sequence_length_256))
        sta <zp_bits_hi
.ENDIF
        lda #$e1
.IF(.DEFINED(i_fourth_offset_table))
        cpx #$04
.ELSE
        cpx #$03
.ENDIF
        bcs gbnc2_next
        lda tabl_bit - 1,x
gbnc2_next:
        asl <zp_bitbuf
        bne gbnc2_ok
        tax
        jsr get_crunched_byte
        rol
        sta <zp_bitbuf
        txa
gbnc2_ok:
        rol
        bcs gbnc2_next
        tax
; -------------------------------------------------------------------
; calulate absolute offset (zp_src) (20 bytes)
;
        lda tabl_bi,x
        jsr get_bits
        adc tabl_lo,x
        sta <zp_src_lo
        lda <zp_bits_hi
        adc tabl_hi,x
        adc <zp_dest_hi
        sta <zp_src_hi
; -------------------------------------------------------------------
; prepare for copy loop (4(2) bytes)
;
pre_copy:
        ldx <zp_len_lo
; -------------------------------------------------------------------
; main copy loop (24 bytes)
;
copy_next:
        tya
        beq copy_decr_hi
copy_skip_hi:
        dey
        beq tya_loop
.IF(.DEFINED(i_literal_sequences_used))
        bcs get_literal_byte1
.ENDIF
        lda (zp_src_lo),y
literal_byte_gotten1:
        sta (zp_dest_lo),y
        dex
        bne copy_skip_hi
after_len_lo:
.IF(!.DEFINED(i_max_sequence_length_256))
        lda <zp_len_hi
.ENDIF
begin_stx:
        stx <zp_bits_hi
        beq begin
.IF(!.DEFINED(i_max_sequence_length_256))
        dec <zp_len_hi
        jmp copy_next
.ENDIF
.IF(.DEFINED(i_literal_sequences_used))
get_literal_byte1:
        jsr get_crunched_byte
        bcs literal_byte_gotten1
.ENDIF
.IF(.DEFINED(i_reuse_offset))
; -------------------------------------------------------------------
; test for offset reuse (11 bytes)
;
test_reuse:
        bvs no_reuse
.IF(.DEFINED(i_max_sequence_length_256))
        lda #$00                ; fetch one bit
.ENDIF
        jsr gb_next
        beq no_reuse            ; bit == 0 => C=0, no reuse
        bne copy_next           ; bit != 0 => C=0, reuse previous offset
.ENDIF
; -------------------------------------------------------------------
; exit or literal sequence handling (16(12) bytes)
;
exit_or_lit_seq:
.IF(.DEFINED(i_literal_sequences_used))
        beq decr_exit
        jsr get_crunched_byte
  .IF(!.DEFINED(i_max_sequence_length_256))
        sta <zp_len_hi
  .ENDIF
        jsr get_crunched_byte
        tax
        bcs copy_next
decr_exit:
.ENDIF
.IF(.DEFINED(exit_hook))
  .INCLUDE("exit_hook")
.ELSE
        rts
.ENDIF
; -------------------------------------------------------------------
; main copy loop 2 (16 bytes)
;
tya_loop:
.IF(.DEFINED(i_literal_sequences_used))
        bcs get_literal_byte2
.ENDIF
        lda (zp_src_lo),y
literal_byte_gotten2:
        sta (zp_dest_lo),y
        dex
        beq after_len_lo
copy_decr_hi:
        dec <zp_dest_hi
        dec <zp_src_hi
        bne copy_skip_hi
.IF(.DEFINED(i_literal_sequences_used))
get_literal_byte2:
        jsr get_crunched_byte
        bcs literal_byte_gotten2
.ENDIF
.IF(.DEFINED(i_fourth_offset_table))
; -------------------------------------------------------------------
; the static stable used for bits+offset for lengths 1, 2 and 3 (3 bytes)
; bits 2, 4, 4 and offsets 64, 48, 32 corresponding to
; %10010000, %11100011, %11100010
tabl_bit:
        .BYTE($90, $e3, $e2)
.ELSE
; -------------------------------------------------------------------
; the static stable used for bits+offset for lengths 1 and 2 (2 bytes)
; bits 2, 4 and offsets 48, 32 corresponding to %10001100, %11100010
tabl_bit:
        .BYTE($8c, $e2)
.ENDIF
; ###################################################################
; ## fast end #######################################################
; ###################################################################
.ELSE
  .ERROR("Symbol i_perf must have a value of [-1 - 2].")
.ENDIF
; ###################################################################
; -------------------------------------------------------------------
stage3end:
; -------------------------------------------------------------------
; -- end of stage 3 -------------------------------------------------
; -------------------------------------------------------------------
        .ORG(stage2end + stage3end - stage3start)
; -------------------------------------------------------------------
; -- start of file part 1 -------------------------------------------
; -------------------------------------------------------------------
file1start:
        .INCBIN("crunched_data", 2, transfer_len)
file1end:
highest_addr_out:
; -------------------------------------------------------------------
; -- end of file part 1 ---------------------------------------------
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; -- End of the actual decruncher -----------------------------------
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; -- Start of file footer stuff -------------------------------------
; -------------------------------------------------------------------
.IF(r_target == 1)
; -------------------------------------------------------------------
; -- Start of Oric-1 file footer stuff ------------------------------
; -------------------------------------------------------------------
o1_end:
.ELIF(r_target == 20 || r_target == 23 || r_target == 52 || r_target == 55 ||
      r_target == 16 || r_target == 4 || r_target == 64 || r_target == 128 ||
      r_target == 4032)
; -------------------------------------------------------------------
; -- Start of Commodore file footer stuff ---------------------------
; -------------------------------------------------------------------
.ELIF(r_target == $a8)
; -------------------------------------------------------------------
; -- Start of Atari file footer stuff -------------------------------
; -------------------------------------------------------------------
a8_end:
.IF(!.DEFINED(i_raw) || i_raw == 0)
        .WORD($02e0, $02e1, a8_start)
.ENDIF
.ELIF(r_target == $a2)
; -------------------------------------------------------------------
; -- Start of Apple file footer stuff -------------------------------
; -------------------------------------------------------------------
a2_end:
.ELIF(r_target == $bbcb)
; -- No header at all for BBC b
.ELSE
  .ERROR("Unhandled target for file header stuff")
.ENDIF
; -------------------------------------------------------------------
; -- End of file footer stuff ---------------------------------------
; -------------------------------------------------------------------
