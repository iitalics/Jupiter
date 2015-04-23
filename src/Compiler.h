#pragma once
#include "Env.h"
#include "Infer.h"
#include <sstream>
#include <map>

class Compiler;


struct CompileUnit
{
	struct Lifetime;
	struct Env;
	using EnvPtr = std::shared_ptr<Env>;

	Compiler* compiler;

	OverloadPtr overload;
	std::string internalName;
	FuncInstance funcInst;

	std::ostringstream ssPrefix;
	std::ostringstream ssBody;
	std::ostringstream ssEnd;


	std::map<ExpPtr, std::string> special;
	std::vector<std::string> nonUnique;
	std::vector<int> tempLifetimes;
	int lifetime;

	struct Var
	{
		std::string name;
		std::string internal;
		bool stackAlloc;
	};
	struct Env
	{
		EnvPtr parent;
		std::vector<Var> vars;

		Env (CompileUnit* cunit, EnvPtr parent);
		Var get (const std::string& name) const;
	};

	CompileUnit (Compiler* comp, OverloadPtr overload, SigPtr sig);
	CompileUnit (Compiler* comp, OverloadPtr overload,
	               SigPtr sig, TyPtr ret,
	               const std::string& intName);

	void writePrefix (EnvPtr env);
	void writeEnd ();
	void output (std::ostream& out);
	
	void stackAlloc (const std::string& name);

	EnvPtr makeEnv (EnvPtr parent = nullptr);
	std::string makeUnique (const std::string& str);

	std::string getTemp ();
	void pushLifetime ();
	void popLifetime ();

	std::string compile (ExpPtr exp, EnvPtr env,
					bool retain = true);

	std::string compileVar (ExpPtr e, EnvPtr env);
	std::string compileCall (ExpPtr e, EnvPtr env);
	std::string compileLet (ExpPtr e, EnvPtr env);
	std::string compileBlock (ExpPtr e, EnvPtr env);
};



class Compiler
{
public:
	Compiler ();
	~Compiler ();

	std::string genUniqueName (const std::string& prefix = "");

	CompileUnit* compile (OverloadPtr overload, SigPtr sig);
	CompileUnit* bake (OverloadPtr overload,
						SigPtr sig, TyPtr ret,
						const std::string& internalName);

	void output (std::ostream& os);
private:
	std::vector<CompileUnit*> units;
	int nameId;
};

