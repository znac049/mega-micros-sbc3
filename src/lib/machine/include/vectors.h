#pragma once

unsigned int* _get_vectors_base(void);
unsigned int get_isr_vector(int vector_number);
unsigned int set_isr_vector(int vector_number, void isr(void));