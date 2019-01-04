        org $1000
start   equ *
        ldx #$00
        lda #$20
        sta $00
        lda #$00
        sta $01
        lda $00,x
        ldy #$02
        sta ($00),y

        org $2000
        dta b($a5)

        org $2e0
        dta a(start)