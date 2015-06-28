#pragma once
#include "Env.h"
#include "Infer.h"
#include <map>

struct Desugar
{
	explicit inline Desugar (GlobEnv& _glob)
		: global(_glob), vars(0) {} 

	~Desugar ();

	GlobEnv& global;
	std::map<std::string, TyPtr> polynames;
	int vars;

	SigPtr desugar (SigPtr sig);
	void desugar (OverloadPtr overload);

	ExpPtr desugar (ExpPtr exp, LocEnvPtr lenv);
	ExpPtr desugarSubexps (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarVar (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarTuple (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarCall (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarInfix (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarCond (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarLambda (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarBlock (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarLet (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarGlobal (ExpPtr e);

	// code generation
	ExpPtr desugarAssign (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarMem (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarForEach (ExpPtr e, LocEnvPtr lenv);
	ExpPtr desugarForRange (ExpPtr e, LocEnvPtr lenv);

	ExpPtr genVar (ExpPtr block, ExpPtr initial,
	                 TyPtr ty = Ty::makeWildcard());


	TyPtr desugar (TyPtr ty, const Span& span);
};