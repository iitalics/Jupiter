#pragma once
#include "Compiler.h"

class LocEnv;
class FuncOverload;
struct FuncInstance;
class GlobFunc;
class LocEnv;
using GlobFuncPtr = std::shared_ptr<GlobFunc>;
using LocEnvPtr = std::shared_ptr<LocEnv>;


class FuncOverload
{
public:
	Sig signature;
	ExpPtr source;
};

struct FuncInstance
{
	GlobFunc* parent;
	Sig signature;
	TyPtr returnType;
	std::string internalName;
};

class GlobFunc
{
public:
	std::string name;
	std::vector<FuncOverload> overloads;
};


class GlobEnv
{
public:
	std::vector<GlobFuncPtr> functions;
	// type declarations
	// modules
	// utility functions

	GlobFuncPtr getFunc (const std::string& name);
};




class LocEnv
{
public:
	using Counter = std::shared_ptr<int>;

	static LocEnvPtr make ();
	static LocEnvPtr make (LocEnvPtr parent);

	~LocEnv ();

	struct Var
	{
		std::string name;
		TyPtr ty;
		int idx;
	};
	using VarPtr = Var*;

	LocEnvPtr parent;
	std::vector<VarPtr> vars;

	VarPtr newVar (TyPtr ty = nullptr);
	VarPtr newVar (const std::string& name, TyPtr ty = nullptr);

	VarPtr get (const std::string& name);
	VarPtr get (int idx);

	bool has (const std::string& name);

private:
	LocEnv (LocEnvPtr _parent, Counter _c);
	
	Counter _count;
};