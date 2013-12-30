start-microcode render

\ NOTE: rendering starts one character line before actual, to avoid flickering of first line

RAM_SPRIMG constant FRAMEBUFFER \ fb block for now at RAM_SPRIMG
d# 640 constant CHR_BLOCK_SIZE \ 40 chars of 16 bytes --> 40 * 16 bytes per block
[ CHR_BLOCK_SIZE d# 5 * ] constant CHR_YOFFSET
d# 32 constant START_YLINE
[ d# 25 d# 8 * START_YLINE + ] constant END_YLINE

: 1+    d# 1 + ;
: -     invert 1+ + ;
: >     swap < ;

: checkinit ( -- )
    \ flag set?
    COMM+0 c@ d# 1 = if
        FRAMEBUFFER \ start
        dup d# 16384 + \ end
        begin
            d# 0 over c! \ zero out
            1- 2dup=
        until
        d# 0 COMM+0 c! \ reset flag
    then ;

: main
    begin
        \ wait until VBLANK is over
        begin
            VBLANK c@ d# 0 =
        until
        
        begin
            YLINE c@
            dup START_YLINE >
            swap END_YLINE < and
            dup d# 0 = if checkinit then
        until
        
        \ wait until start of a character
        begin
            YLINE c@ d# 7 and d# 0 =
        until

        YLINE c@
        dup d# 3 rshift CHR_BLOCK_SIZE * \ char block
        FRAMEBUFFER + CHR_YOFFSET - \ src
        
        swap d# 3 rshift d# 1 and \ 0 = even ch line, 1 = odd ch line
        CHR_BLOCK_SIZE * RAM_CHR + \ dest
               
        CHR_BLOCK_SIZE \ size
        
        begin
            >r
            over c@ over c!
            1+ swap 1+ swap
            r> 1- dup d# 0 =
        until
        drop \ size
        drop \ dest
        drop \ src
    again
;

end-microcode
