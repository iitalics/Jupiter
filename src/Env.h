#pragma once
#include "Jupiter.h"
#include "Ast.h"
#include <tuple>

class Compiler;
class Overload;
class GlobFunc;
class LocEnv;
class GlobEnv;
struct FuncInstance;
struct CompileUnit;
using GlobFuncPtr = GlobFunc*;
using LocEnvPtr = std::shared_ptr<LocEnv>;
using OverloadPtr = std::shared_ptr<Overload>;


class Overload
{
public:
	GlobEnv& env;
	std::string name;
	SigPtr signature;
	ExpPtr body;
	std::vector<FuncInstance> instances;

	static OverloadPtr make (GlobEnv& env, const std::string& name,
	                           SigPtr sig, ExpPtr body);
	static FuncInstance inst (OverloadPtr overload,
	                            SigPtr sig, Compiler* comp);
};

struct FuncInstance
{
	std::string name;
	SigPtr signature;
	TyPtr returnType;

	CompileUnit* cunit;

	FuncInstance (CompileUnit* cunit,
					SigPtr sig, TyPtr ret = Ty::makePoly());

	TyPtr type () const;
};

class GlobFunc
{
public:
	explicit inline GlobFunc(GlobEnv& _env, const std::string& _name)
		: env(_env), name(_name) {}

	GlobEnv& env;
	std::string name;
	std::vector<OverloadPtr> overloads;
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

	~GlobEnv ();

	OpPrecedence getPrecedence (const std::string& oper) const;
	GlobFuncPtr getFunc (const std::string& name) const;
	GlobFuncPtr addFunc (const std::string& name);

	// creates a function and an instance
	void bake (Compiler* comp,
		        const std::string& intName,
	            const std::string& name,
	            const std::vector<TyPtr>& args,
	            TyPtr ret);

	void loadToplevel (const GlobProto& proto);
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
	};
	using VarPtr = Var*;

	LocEnvPtr parent;
	std::vector<VarPtr> vars;

	VarPtr newVar (TyPtr ty = nullptr);
	VarPtr newVar (const std::string& name, TyPtr ty = nullptr);

	VarPtr get (const std::string& name);

	bool has (const std::string& name);

private:
	LocEnv (LocEnvPtr _parent, Counter _c);
	
	Counter _count;
};