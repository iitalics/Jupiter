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
	std::vector<Lifetime*> regs;
	int temps;

	struct Lifetime
	{
		CompileUnit* cunit;
		std::vector<int> regs;

		explicit Lifetime (CompileUnit* cunit);
		~Lifetime ();

		void relinquish ();
		int claim (int idx);
	};
	struct Var
	{
		std::string name;
		int idx;
	};
	struct Env
	{
		EnvPtr parent;
		std::vector<Var> vars;
		Lifetime life;

		explicit Env (CompileUnit* cunit, EnvPtr parent);
		Var get (const std::string& name) const;
	};

	enum OpKind
	{
		opVar,
		opLit,
		opArg
	};
	struct Operand
	{
		OpKind kind;

		int idx;
		ExpPtr src;
	};

	CompileUnit (Compiler* comp, OverloadPtr overload, SigPtr sig);
	CompileUnit (Compiler* comp, OverloadPtr overload,
	               SigPtr sig, TyPtr ret,
	               const std::string& intName);

	void writePrefix ();
	void writeEnd ();
	void output (std::ostream& out);

	std::string regString (int idx) const;
	std::string argString (int idx) const;
	std::string tempString (int id = 0) const;

	EnvPtr makeEnv (EnvPtr parent = nullptr);
	int findRegister (Lifetime* life);

	std::string compileOp (const Operand& op);
	Operand compile (ExpPtr e, EnvPtr env, Lifetime* life);
	Operand compileVar (ExpPtr e, EnvPtr env, Lifetime* life);
	Operand compileBlock (ExpPtr e, EnvPtr env, Lifetime* life);
	Operand compileCall (ExpPtr e, EnvPtr env, Lifetime* life);
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

