start-microcode render

RAM_SPRPAL constant RAM_SPRZOOM \ Use 4 256 color palettes for zoom info
COMM+0 constant NUM_ZSPRITES \ number of sprites to zoom
d# 4 constant SPRZOOM_SIZE \ NOTE: +1 byte to make calculating RAM_SPR pos easier

: 1+    d# 1 + ;
: -     invert 1+ + ;
: 2dup>= over over ;fallthru
: >=    1- ;fallthru
: >     swap < ;
: @     dupc@ swap 1+ c@ swab or ;
: 2drop drop drop ;
: 2dup< over over < ;


: main
    begin
        \ wait until VBLANK is over
        begin
            VBLANK c@ d# 0 =
        until

        NUM_ZSPRITES c@ SPRZOOM_SIZE * RAM_SPRZOOM + \ end

        begin
            YLINE c@ >r

            RAM_SPRZOOM \ start
            begin
                \ dupc@ YLINE c@ 1- < YLINE c@ d# 200 < and if
                \ dupc@ r@ 1+ < r@ d# 200 < and if
                dupc@ r@ 1+ < over c@ d# 96 + r@ >= and if
                    \ dup 1+ @ \ yzoom
                    \ over c@ \ startY
                    d# 205
                    d# 50

                    \ YLINE c@
                    r@

                    \ dup d# 1 and dup>r SPR_PAGE c!
                    1+ swap - * over c@ d# 256 * + d# 128 + d# 8 rshift \ ((yline+1 - starty) * yzoom + (starty * 256) + 128) / 256
                    \ \ over d# 2048 r> d# 0 = rshift d# 2 - - c! \ RAM_SPRZOOM - (2048 >> (SPR_PAGE==0)) - 2 == RAM_SPR + 2 == sprite y
                    \ over r> if d# 2046 else d# 1022 then
                    \ - c!
                    over d# 2046 - c! \ RAM_SPRZOOM - 2048 + 2 == RAM_SPR + 2 == sprite y
                then

                SPRZOOM_SIZE + 2dup<
            until

            drop

            r>
            begin
                dup YLINE c@ = d# 0 = \ Wait until next YLINE
            until
            drop

            VBLANK c@
        until

        drop
    again
;

end-microcode
