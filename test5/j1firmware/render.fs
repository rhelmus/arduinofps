start-microcode render

d# 10 constant STARTY
d# 128 constant YPLUS
[ RAM_SPR 2048 + ] constant RAM_SPRZOOM \ Use 2nd sprite page for zoom info
COMM+0 constant NUM_ZSPRITES \ number of sprites to zoom
d# 4 constant SPRZOOM_SIZE \ NOTE: +1 byte to make calculating RAM_SPR pos easier

: 1+    d# 1 + ;
: -     invert 1+ + ;
: 2dup>= over over ;fallthru
: >=    1- ;fallthru
: >     swap < ;
: @     dup c@ swap 1+ c@ swab or ;
: 2drop drop drop ;
: 2dup<= over over 1+ < ;


: main
    begin
        \ wait until VBLANK is over
        begin
            VBLANK c@ d# 0 =
        until

        begin
            YLINE c@
            STARTY >=
            \ UNDONE
            \ dup STARTY >=
            \ swap ENDY < and
        until

        YLINE c@ >r

        NUM_ZSPRITES c@ SPRZOOM_SIZE * RAM_SPRZOOM + \ end
        RAM_SPRZOOM \ start
        begin
            dup 1+ @ \ yzoom
            over c@ \ startY

            r@ swap - * over c@ d# 256 * + d# 8 rshift \ (yline - starty) * yzoom + (starty * 256) / 256
            \ drop drop r@
            \ RAM_SPRZOOM SPRZOOM_SIZE + d# 2046 - c!
            over d# 2046 - c! \ RAM_SPRZOOM - 2046 == RAM_SPR + 2 == sprite y

            SPRZOOM_SIZE + 2dup<=
        until

        2drop
        r>

        \ STARTY d# 256 *
        \ YLINE c@
        \ STARTY - YPLUS * + d# 8 rshift \ (yline - starty) * yplus + (starty * 256) / 256
        \ \ STARTY - YPLUS * STARTY_FP + d# 8 rshift \ (yline - starty) * yplus + (starty*256) / 256

        \ RAM_SPR d# 2 + c!
    again
;

end-microcode
