start-microcode render

d# 10 constant STARTY
d# 4 constant ZOOMY
d# 2 constant SHIFT_ZOOMY
[ ZOOMY d# 16 * STARTY + ] constant ENDY


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
            YLINE c@
            dup STARTY >=
            swap ENDY < and
        until
        
        YLINE c@
        dup STARTY - SHIFT_ZOOMY rshift - \ yline - (yline - starty) / zoomy
        RAM_SPR d# 2 + c!
    again
;

end-microcode
