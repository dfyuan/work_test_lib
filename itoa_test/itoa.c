#include <stdio.h>

typedef int u32;

static int my_itoa(int num,  char *str,  int base)
{
	char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	u32 unum;
	int i = 0, j, k;

	if (base == 10 && num < 0) {
		unum = (u32)-num;
		str[i++] = '-';
	} else {
		unum = (u32)num;
	}

	do {
		str[i++] = index[unum % (u32)base];
		unum /= base;
	} while (unum);
	str[i] = '\0';

	if (str[0] == '-') {
		k = 1;
	} else {
		k = 0;
	}
	for (j = k; j <= (i - 1) / 2 + k; ++j) {
		num = str[j];
		str[j] = str[i-j-1+k];
		str[i-j-1+k] = num;
	}

	return 0;
}
void main(void)
{
	char duty[50];
	printf("start to test\n");
	my_itoa(-820,duty,10);
	printf("duty=%s \n",duty);
}