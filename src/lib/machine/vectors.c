#include <stdio.h>
#include <vectors.h>

unsigned int get_isr_vector(int vector_number) {
    unsigned int *vector_base;

    if (vector_number > 255) {
        return 0xffffffff;
    }

    vector_base = _get_vectors_base(); 

    printf("VBR contains %08x\r\n", (unsigned int)vector_base);

    return vector_base[vector_number];
}

unsigned int set_isr_vector(int vector_number, void isr(void)) {
    unsigned int *vector_base;
    unsigned int old_isr;

    if (vector_number > 255) {
        return 0xffffffff;
    }

    vector_base = _get_vectors_base(); 

    old_isr = vector_base[vector_number];
    vector_base[vector_number] = (unsigned int)isr;

    return old_isr;
}