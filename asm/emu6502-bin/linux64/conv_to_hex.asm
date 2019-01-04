
                opt f-g-h+l+o+
                org $1000

start           equ *

                lda <text
                sta $80
                lda >text
                sta $81
loop            ldy #0
                lda counter
                jsr hex_to_text

                lda <text
                ldx >text
                jsr $ff80
                lda counter
                add #1
                sta counter
                bcc loop
                brk

hex_to_text     pha
                jsr msb_to_digit
                pla
                :4 asl @
msb_to_digit    :4 lsr @
                and #$0f
                add #'0'
                cmp #10+'0'
                bcc save
                add #'A'-'0'-10
save            sta ($80),y
                iny
                rts

byte            dta b(0)
counter         dta b(0)

                org $2000
text            equ *
                dta b(0),b(0)
                dta b(10) ; '\n'
                dta b(0)

                org $2E0
                dta a(start)

                end of file
