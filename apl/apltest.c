// apltest.c
// display alternative character set for apl
//
// usage: tek4010 -noexit ./apltest

#include <stdio.h>

int main (int argc, char *argv[])
{
	int ch;

	printf("\nStandard character set:\n");
	for (ch = 32; ch <= 126; ch++) {
		if ((ch % 32) == 0) {
			printf("\n%02X ", ch);
		}
		printf("%c", ch);
	}
	printf("\n");
 	
        printf("\nAPL character set:\n");
	putchar(27); putchar(14);  // switch to alternative character set
	for (ch = 32; ch <= 126; ch++) {
		if ((ch % 32) == 0) {
			printf("\n%02X ", ch);
		}
		printf("%c", ch);
	}
        printf("\n\noverstrike test");
        putchar(62); putchar(32);
        putchar(79); putchar(8); putchar(63); putchar(32);
        putchar(76); putchar(8); putchar(43);
	putchar(27); putchar(15);  // switch back to standard character set
	printf("\n");
}
