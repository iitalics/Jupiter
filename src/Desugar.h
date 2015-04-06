#pragma once
#include "Env.h"


struct Desugar
{
	explicit inline Desugar (GlobEnvPtr _glob)
		: global(_glob) {}

	~Desugar ();

	GlobEnvPtr global;

	ExpPtr desugarFunction (ExpPtr exp, SigPtr sig);
	ExpPtr desugar (ExpPtr exp, LocEnvPtr lenv);
	ExpPtr desugarSubexps (ExpPtr e, LocEnvPtr lenv);

	ExpPtr desugarVar (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarTuple (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarCall (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarInfix (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarCond (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarBlock (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarLet (ExpPtr e, LocEnvPtr lenv);
};