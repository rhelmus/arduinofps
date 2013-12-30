start-microcode render

\ Keep this in sync with C++ code
d# 20 constant SCREEN_WIDTH_SPR
d# 54 constant STARTY
[ STARTY d# 192 + ] constant ENDY

: 1+    d# 1 + ;
: -     invert 1+ + ;
: >=    1- ;fallthru
: >     swap < ;


: main
    begin
        \ wait until VBLANK is over
        begin
            VBLANK c@ d# 0 =
        until

        begin
            YLINE c@ STARTY =
        until

        begin
            YLINE c@ >r

            d# 0 \ counter
            begin
                \ sprite starty = STARTY + ((yline-STARTY)/32)*32 (multiples of 32)
                r@ STARTY - d# 5 rshift d# 32 * STARTY +

                \ sprite y = sprite starty + ((yline-sprite starty + 1)/2)
                r@ over - 1+ d# 1 rshift +

                \ set sprite y index = (counter + (((yline-starty)/32) * SCREEN_WIDTH_SPR) * 4) + RAM_SPR + 2
                over r@ STARTY - d# 5 rshift SCREEN_WIDTH_SPR * + d# 4 * RAM_SPR + d# 2 + c!

                1+ dup SCREEN_WIDTH_SPR =
            until
            drop

            r>
            begin
                dup YLINE c@ = d# 0 = \ Wait until next YLINE
            until

            ENDY 1+ =
        until

        \ wait till VBLANK
        begin
            VBLANK c@
        until
    again
;

end-microcode
