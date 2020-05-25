.export zpd_mov_xy
.export zpd_inc_x
.export zpd_addb_xay
.export zpd_is_minus1_y

.code

.proc zpd_mov_xy
    lda 0,x
    sta 0,y
    lda 1,x
    sta 1,y
    lda 2,x
    sta 2,y
    lda 3,x
    sta 3,y
    rts
.endproc

.proc zpd_inc_x
    inc 0,x
    bne n
    inc 1,x
    bne n
    inc 2,x
    bne n
    inc 3,x
n:  rts
.endproc

.proc zpd_addb_xay
    sta 0,y
    lda #0
    sta 1,y
    sta 2,y
    sta 3,y

    lda 0,x
    clc
    adc 0,y
    sta 0,y
    lda 1,x
    adc 1,y
    sta 1,y
    lda 2,x
    adc 2,y
    sta 2,y
    lda 3,x
    adc 3,y
    sta 3,y
    rts
.endproc

.proc zpd_is_minus1_y
    lda 0,y
    and 1,y
    and 2,y
    and 3,y
    cmp #$ff
    rts
.endproc
