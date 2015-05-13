#include "jupiter.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define die_unimpl(s) \
	fprintf(stderr, "RUNTIME ERROR: unimplemented: \"" s "\"\n"), \ 
	exit(-1), ju_null



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



juc juStd_stringInt (juc ca)
{
	const size_t BUF_SIZE = 16;
	char buf[BUF_SIZE];

	ju_int a = ju_to_int(ca);
	size_t len = 0;
	do
	{
		buf[BUF_SIZE - (++len)] = '0' + (a % 10);
		a /= 10;
	} while (a > 0);

	return ju_make_str(buf + BUF_SIZE - len - 1, len);
}
juc juStd_stringBool (juc ca)
{
	return (ca == ju_false) ?
		ju_make_str("false", 5) :
		ju_make_str("true", 4);
}
juc juStd_stringReal (juc ca)
{
	return die_unimpl("string (Real) -> String");	
}
juc juStd_stringStringString (juc a, juc b)
{
	size_t la, lb;
	char* ba, *bb, *buf;

	la = ju_get_length(a);
	lb = ju_get_length(b);
	ba = ju_get_buffer(a);
	bb = ju_get_buffer(b);

	res = ju_make_str(NULL, la + lb);
	buf = ju_get_buffer(res);

	memcpy(buf, ba, la);
	memcpy(buf + la, bb, lb);

	return res;
}