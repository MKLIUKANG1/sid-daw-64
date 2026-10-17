; Minimal SID player for PSID export (64tass syntax)
; Load/Init: $1000, Play: $1003 (50 Hz PAL VBI)

ptr_lo   = $FB
ptr_hi   = $FC
wait_lo  = $FD
wait_hi  = $FE
count    = $F9
temp_reg = $FA

* = $1000

    jmp sid_init
    jmp sid_play

sid_init:
    ldx #$18
    lda #$00
clear_loop:
    sta $D400,x
    dex
    bpl clear_loop
    lda #$0F
    sta $D418
    lda #<data_start
    sta ptr_lo
    lda #>data_start
    sta ptr_hi
    lda #0
    sta wait_lo
    sta wait_hi
    jsr exec_batch
    rts

sid_play:
    lda wait_lo
    ora wait_hi
    beq play_exec
    lda wait_lo
    bne dec_lo
    dec wait_hi
dec_lo:
    dec wait_lo
    rts
play_exec:
    jsr exec_batch
    rts

exec_batch:
    ldy #0
    lda (ptr_lo),y
    bmi loop_song
    sta count
    beq skip_writes
    iny
write_loop:
    lda (ptr_lo),y
    iny
    sta temp_reg
    lda (ptr_lo),y
    iny
    ldx temp_reg
    sta $D400,x
    dec count
    bne write_loop
    jmp read_delay
skip_writes:
    iny
read_delay:
    lda (ptr_lo),y
    sta wait_lo
    iny
    lda (ptr_lo),y
    sta wait_hi
    iny
    tya
    clc
    adc ptr_lo
    sta ptr_lo
    bcc advance_done
    inc ptr_hi
advance_done:
    rts

loop_song:
    lda #<data_start
    sta ptr_lo
    lda #>data_start
    sta ptr_hi
    lda #0
    sta wait_lo
    sta wait_hi
    rts

data_start:
