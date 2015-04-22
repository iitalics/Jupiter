#include "jupiter.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>



juc juStd_addInt (juc ca, juc cb)
{
	const ju_int a = (ju_int) ca;
	const ju_int b = (ju_int) cb;

	return (juc)
		(a + b - 1); // hacky things
}
juc juStd_negInt (juc ca)
{
	const ju_int a = (ju_int) ca;

	return (juc) (2 - a); // more hacky things
}
juc juStd_ltInt (juc ca, juc cb)
{
	const ju_int a = (ju_int) ca;
	const ju_int b = (ju_int) cb;

	return ((a - b) >> 1) < 0 ? ju_true : ju_false;
}
juc juStd_eqInt (juc a, juc b)
{
	return (a == b) ? ju_true : ju_false;
}
juc juStd_println (juc a)
{
	fwrite(ju_get_buffer(a),
			1,
			ju_get_length(a),
			stdout);
	putchar('\n');
	fflush(stdout);

	return ju_unit;
}
