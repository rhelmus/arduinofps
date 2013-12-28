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
: !     over swab over 1+ c! c! ;
: 2drop drop drop ;


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
                \ dupc@ r@ 1+ < r@ d# 200 < and if
                dupc@ r@ 1+ < over c@ d# 96 + r@ >= and if
                    \ dup d# 2 + @ \ y counter (FP)
                    \ \ dup 1+ @ \ yzoom (UNDONE)
                    \ d# 205 \ yzoom
                    \ + \ ycounter + yzoom
                    \ over d# 2 + over swap ! \ write new value

                    \ ((starty+yzoom)+128)/256
                    \ d# 128 + d# 8 rshift \ new sprite y

                    \ \ 1+ swap - * over c@ d# 256 * + d# 128 + d# 8 rshift \ ((yline+1 - starty) * yzoom + (starty * 256) + 128) / 256
                    \ over d# 2046 - c! \ RAM_SPRZOOM - 2048 + 2 == RAM_SPR + 2 == sprite y
                    r@ over d# 2046 - c!
                    noop noop noop noop
                    noop noop noop noop
                    noop noop \ noop noop
                    \ noop noop noop noop
                    \ noop noop noop noop
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

        \ Reset sprites (we're now VBLANKING)
        RAM_SPRZOOM \ begin
        begin
            dup c@ \ starty
            d# 256 * over d# 2 + !
            d# 4 + 2dup<
        until

        2drop

    again
;

end-microcode
