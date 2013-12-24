start-microcode render

d# 10 constant STARTY
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

        NUM_ZSPRITES c@ SPRZOOM_SIZE * RAM_SPRZOOM + \ end

        begin
            \ YLINE c@ >r

            RAM_SPRZOOM \ start

            begin
                dup 1+ @ \ yzoom
                over c@ \ startY

                YLINE c@
                dup d# 1 and dup>r SPR_PAGE c!
                swap - * over c@ d# 256 * + d# 128 + d# 8 rshift \ ((yline - starty) * yzoom + (starty * 256) + 128) / 256
                over d# 2048 r> rshift d# 2 - - c! \ RAM_SPRZOOM - (2048 >> SPR_PAGE) - 2 == RAM_SPR + 2 == sprite y

                SPRZOOM_SIZE + 2dup<=
            until

            \ r>
            drop

            VBLANK c@
        until

        drop
    again
;

end-microcode
