#include "stdarg.h"
#include "stdlib.h"

int printf(const char* format, ...)
{
    int len = 0; /* Keep track of the number of characters printed */
    va_list arg_list; /* Declare a list of all variable arguments */
    va_start(arg_list, format); /* Initialize starting position */

    /* Loop through the format string */
    for (int i = 0; format[i] != '\0'; i++)
    {
        if (format[i] == '%')/* if a directive ('%') is encountered */
        {
            i++; /* Move to the next character after '%' */
            if (format[i] == 's')//Check for the format specifier (e.g., %s)
            {
                char *str = va_arg(arg_list, char *);/*Access the str arguments*/
                int j;
                /* Loop through the string and print its characters */
                for (j = 0; str[j] != '\0'; j++)
                {
                    putchar(str[j]); 
                    len++; /* Update len with the number of chars printed */
                }
            }
            /*Implement other format specifiers for different data types -*/
                 /* (e.g., %c, %d) similarly. */

           /* Update len accordingly for each specifier */
        }
        else
        {
            putchar(format[i]); /* Print regular characters */
            len++; /* Update len for each character printed */
        }
    }

    /* Clean up the list when you're done formatting and printing */
    va_end(arg_list);

    /* Return the total len of every character printed to the console */
     return len;
}