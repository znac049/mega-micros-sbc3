        section .text,code

_get_vectors_base::
        movec.l vbr,a0
        rts