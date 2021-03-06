#pragma once
#include "Jupiter.h"
#include "Ast.h"
#include <tuple>
#include <set>

class Compiler;
class Overload;
class LocEnv;
class GlobEnv;
struct GlobFunc;
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
	std::vector<CompileUnit*> instances;
	bool hasEnv;
	bool isPublic;
	bool isDesugared;

	static OverloadPtr make (GlobEnv& env, const std::string& name,
	                           SigPtr sig, ExpPtr body, bool isPub);
	static FuncInstance inst (OverloadPtr overload, SigPtr sig,
	                            Compiler* origin);
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

struct GlobFunc
{
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
	static int_t getTag (const std::string& ident);

	Compiler* compiler;
	std::vector<OpPrecedence> operators;
	std::vector<GlobFuncPtr> functions;
	std::vector<TypeInfo*> types;
	GlobProto proto;

	GlobEnv (const GlobProto& proto);
	~GlobEnv ();

	OpPrecedence getPrecedence (const std::string& oper) const;
	GlobFuncPtr getFunc (const std::string& name) const;
	GlobFuncPtr addFunc (const std::string& name);
	void addType (const TypeInfo& tyi);
	TypeInfo* getType (const std::string& name) const;

	void loadBuiltinTypes ();
private:
	void generateType (TypeDecl& tydecl);
	void loadToplevel ();
};




class LocEnv
{
public:
	using Counter = std::shared_ptr<int>;
	using UseSet = std::set<std::string>;
	using UseSetPtr = UseSet*;

	static LocEnvPtr make ();
	static LocEnvPtr make (LocEnvPtr parent);

	~LocEnv ();

	struct Var
	{
		std::string name;
		TyPtr ty;
		bool mut;
	};
	using VarPtr = Var*;

	LocEnvPtr parent;
	std::vector<VarPtr> vars;
	UseSetPtr uses;

	VarPtr newVar (TyPtr ty = nullptr);
	VarPtr newVar (const std::string& name, TyPtr ty = nullptr);

	VarPtr get (const std::string& name);

	bool has (const std::string& name);

private:
	LocEnv (LocEnvPtr _parent, Counter _c);
	
	Counter _count;
};