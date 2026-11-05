    .section .text._reset,"ax",@progbits
    .globl _rst
    .type _rst,@function
_rst:
    cpy dr13, dr0

    cpy tdr0, loc
    add i tdr0, %pcrel(ldg_sys_handover_set)
    call tdr0

    cpy i tdr1, 0
    jmp rel ne dr0, tdr1, .Lhang

    ld q dr1, [dr13 + 32]
    ld du dr0, [dr13 + 40]

    cpy tdr0, loc
    add i tdr0, %pcrel(main)
    call tdr0

    cpy tdr0, loc
    add i tdr0, %pcrel(ldg_sys_ctx_end)
    call tdr0

.Lhang:
    jmp rel al .Lhang

.Lrst_end:
    .size _rst, .Lrst_end-_rst
