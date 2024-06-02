// apltest.c
// display alternative character set for apl
//
// usage: tek4010 -noexit ./apltest

#include <stdio.h>

int main (int argc, char *argv[])
{
        int ch, i;

        printf("\nStandard character set:\n");
        printf("    ");
        for (i = 0; i < 10; i++) printf("%1d ",i);
        for (ch = 30; ch <= 126; ch++) {
                if ((ch % 10) == 0) {
                        printf("\n%03d ", ch);
                }
                if (ch > 32)
                        printf("%c ", ch);
                else
                        printf("  ");
        }
        printf("\n");

        printf("\nTektronix APL character set:\n");
        putchar(27); putchar(14);  // switch to alternative character set
        printf("    ");
        for (i = 0; i < 10; i++) printf("%1d ",i);
        for (ch = 30; ch <= 126; ch++) {
                if ((ch % 10) == 0) {
                        printf("\n%03d ", ch);
                }
                if (ch >32)
                        printf("%c ", ch);
                else
                        printf("  ");
        }
        printf("\n\noverstrike test");
        putchar(62); putchar(32);
        putchar(79); putchar(8); putchar(63); putchar(32);
        putchar(76); putchar(8); putchar(43);
        putchar(27); putchar(15);  // switch back to standard character set
        printf("\n");
}
