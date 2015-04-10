#pragma once
#include "Compiler.h"
#include <tuple>

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
	SigPtr signature;
	ExpPtr source;
};

struct FuncInstance
{
	GlobFunc* parent;
	SigPtr signature;
	TyPtr returnType;
	std::string internalName;
};

class GlobFunc
{
public:
	explicit inline GlobFunc(const std::string& _name)
		: name(_name) {}

	std::string name;
	std::vector<FuncOverload> overloads;
	std::vector<FuncInstance> instances;
};


enum class Assoc { Left, Right };

class GlobEnv
{
public:
	using OpPrecedence = std::tuple<std::string, int, Assoc>;

	std::vector<OpPrecedence> operators;
	std::vector<GlobFuncPtr> functions;
	// type declarations
	// modules
	// utility functions

	OpPrecedence getPrecedence (const std::string& oper) const;
	GlobFuncPtr getFunc (const std::string& name) const;

	GlobFuncPtr addFunc (const std::string& name);
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