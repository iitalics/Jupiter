#pragma once
#include "Env.h"
#include "Infer.h"


class Compiler;


struct CompileUnit
{
	Compiler* compiler;

	FuncOverload& overload;
	std::string internalName;
	FuncInstance funcInst;

private:
	friend class Compiler;
	CompileUnit (Compiler* comp, FuncOverload& over, SigPtr sig);
	CompileUnit (Compiler* comp, FuncOverload& over,
	               SigPtr sig, TyPtr ret,
	               const std::string& intName);
};



class Compiler
{
public:
	Compiler ();
	~Compiler ();

	std::string genUniqueName (const std::string& prefix = "");

	CompileUnit* compile (FuncOverload& overload, SigPtr sig);
	CompileUnit* bake (FuncOverload& overload,
						SigPtr sig, TyPtr ret,
						const std::string& internalName);
private:
	std::vector<CompileUnit*> units;
	int nameId;
};

