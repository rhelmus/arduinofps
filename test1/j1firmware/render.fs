start-microcode render

\ COMM interface:

h# 6fff constant FRAMEBUFFER \ fb block is at end of RAM_SPRIMG
d# 64 constant CHR_LINE_SIZE \ every 8 pixels 32 chars of 16 bytes --> 32 * 16 / 8 bytes per line
d# 512 constant CHR_BLOCK_SIZE \ 32 chars of 16 bytes --> 32 * 16 bytes per block

: negate invert ;fallthru
: 1+    d# 1 + ;
: >     swap < ;

: main
    begin
        \ wait until VBLANK is over
        begin
            VBLANK c@ d# 0 =
        until
        
        begin
            YLINE c@ d# 64 <
        until
        
        \ wait until start of a character
        begin
            YLINE c@ d# 3 and d# 0 =
        until

        \ Copy next line from FRAMEBUFFER to RAM_CHR
        \ YLINE c@ dup >r
       
        \ \ dup d# 3 rshift CHR_BLOCK_SIZE * \ char block (quantitized to 8)
        \ \ FRAMEBUFFER + \ src
        \ CHR_LINE_SIZE * \ offset
        \ dup FRAMEBUFFER + \ src

        \ swap \ offset <--> src
        
        
        \ \ swap CHR_LINE_SIZE *  \ block offset over negate +
        
        \ r> d# 3 rshift CHR_BLOCK_SIZE * \ char block (quantitized to 8)
        \ negate + \ offset + -charblock
        \ RAM_CHR + \ dest

        \ CHR_LINE_SIZE \ size
        
        ( YLINE c@ d# 3 rshift CHR_BLOCK_SIZE * \ char block
        FRAMEBUFFER + \ src
        RAM_CHR \ src
        
        CHR_BLOCK_SIZE \ size )
        
        
        YLINE c@
        dup d# 3 rshift CHR_BLOCK_SIZE * \ char block
        FRAMEBUFFER + \ src
        
        swap d# 3 rshift d# 1 and \ 0 = even ch line, 1 = odd ch line
        \ d# 1 swap negate + \ 0 = odd, 1 = even
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
