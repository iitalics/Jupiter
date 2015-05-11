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
juc juStd_mulInt (juc ca, juc cb)
{
	const ju_int a = ju_to_int(ca);
	const ju_int b = ju_to_int(cb);

	return ju_from_int(a * b);
}
juc juStd_divInt (juc ca, juc cb)
{
	const ju_int a = ju_to_int(ca);
	const ju_int b = ju_to_int(cb);

	// if (b == 0) ?

	return ju_from_int(a / b);
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
juc juStd_notBool (juc a)
{
	return a ? ju_false : ju_true;
}
juc juStd_println ()
{
	putchar('\n');
	fflush(stdout);

	return ju_unit;
}
juc juStd_printString (juc a)
{
	fwrite(ju_get_buffer(a),
			1,
			ju_get_length(a),
			stdout);

	return ju_unit;
}
juc juStd_printInt (juc ca)
{
	printf("%d", ju_to_int(ca));

	return ju_unit;
}
juc juStd_printReal (juc ca)
{
	// TODO: improve this
	printf("%.6f", ju_get_real(ca));

	return ju_unit;
}
juc juStd_printBool (juc cell)
{
	if (cell == ju_false)
		printf("false");
	else
		printf("true");

	return ju_unit;
}
