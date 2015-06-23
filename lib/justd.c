#include "jupiter.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define die_unimpl(s) \
	fprintf(stderr, "RUNTIME ERROR: unimplemented: \"" s "\"\n"), \
	exit(-1), ju_null

#define to_bool(c) ((c) ? ju_true : ju_false)

;




//////////////////////////////// Int ////////////////////////////////
juc juStd_succInt (juc ca) { return (juc) ((ju_int) ca + 2); }
juc juStd_predInt (juc ca) { return (juc) ((ju_int) ca - 2); }
juc juStd_addInt (juc ca, juc cb)
{
	return (juc)
		((ju_int) ca + (ju_int) cb - 1); // hacky things
}
juc juStd_mulInt (juc ca, juc cb)
{
	return ju_from_int(ju_to_int(ca) * ju_to_int(cb));
}
juc juStd_divInt (juc ca, juc cb)
{
	// if (ju_to_int(cb) == 0) ?

	return ju_from_int(ju_to_int(ca) / ju_to_int(cb));
}
juc juStd_modInt (juc ca, juc cb)
{
	// if (ju_to_int(cb) == 0) ?

	return ju_from_int(ju_to_int(ca) % ju_to_int(cb));
}
juc juStd_negInt (juc ca)
{
	return (juc) (2 - (ju_int) ca); // more hacky things
}
juc juStd_ltInt (juc ca, juc cb)
{
	const ju_int a = (ju_int) ca;
	const ju_int b = (ju_int) cb;

	return to_bool(((a - b) >> 1) < 0);
}
juc juStd_eqInt (juc a, juc b)
{
	return to_bool(a == b);
}
juc juStd_intReal (juc r)
{
	return ju_from_int((ju_int) ju_get_real(r));
}



//////////////////////////////// Real ////////////////////////////////
juc juStd_succReal (juc ca) { return ju_make_real(ju_get_real(ca) + 1.0); }
juc juStd_predReal (juc ca) { return ju_make_real(ju_get_real(ca) - 1.0); }
juc juStd_addReal (juc ca, juc cb)
{
	return ju_make_real(ju_get_real(ca) + ju_get_real(cb));
}
juc juStd_mulReal (juc ca, juc cb)
{
	return ju_make_real(ju_get_real(ca) * ju_get_real(cb));
}
juc juStd_divReal (juc ca, juc cb)
{
	// if (ju_get_real(cb) == 0) ?
	return ju_make_real(ju_get_real(ca) / ju_get_real(cb));
}
juc juStd_negReal (juc ca)
{
	return ju_make_real(-ju_get_real(ca));
}
juc juStd_recipReal (juc ca)
{
	// if (ju_get_real(ca) == 0) ?
	return ju_make_real(1.0 / ju_get_real(ca));
}
juc juStd_ltReal (juc ca, juc cb)
{
	return to_bool(ju_get_real(ca) < ju_get_real(cb));
}
juc juStd_eqReal (juc ca, juc cb)
{
	return to_bool(ju_get_real(ca) == ju_get_real(cb));
}
juc juStd_realInt (juc ci)
{
	return ju_make_real((ju_real) ((ju_int) ci >> 1));
}





//////////////////////////////// Bool ////////////////////////////////
juc juStd_notBool (juc a)
{
	return a ? ju_false : ju_true;
}
juc juStd_addBool (juc a, juc b)
{
	return a ? ju_true : b;
}
juc juStd_mulBool (juc a, juc b)
{
	return a ? b : ju_false;
}




//////////////////////////////// print ////////////////////////////////
juc juStd_println ()
{
	putchar('\n');
	fflush(stdout);

	return ju_unit;
}
juc juStd_printStr (juc a)
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



//////////////////////////////// Str ////////////////////////////////
juc juStd_strInt (juc ca)
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

	return ju_make_str(buf + BUF_SIZE - len, len);
}
juc juStd_strBool (juc ca)
{
	return (ca == ju_false) ?
		ju_make_str("false", 5) :
		ju_make_str("true", 4);
}
juc juStd_strReal (juc ca)
{
	return die_unimpl("str(Real)");	
}
juc juStd_lenStr (juc a)
{
	return
		ju_from_int((ju_int)
			ju_get_length(a));
}
juc juStd_appStrStr (juc a, juc b)
{
	size_t la, lb;
	char* ba, *bb, *buf;
	juc res;

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
