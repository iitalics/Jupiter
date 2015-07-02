#pragma once
#include "Env.h"
#include "Infer.h"
#include <sstream>
#include <map>
#include <set>

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

	bool finishedInfer;
	std::map<ExpPtr, CompileUnit*> special;
	std::set<std::string> nonUnique;
	std::vector<int> tempLifetimes;
	std::set<ExpPtr> tailCalls;
	int lifetime;
	size_t nroots;

	struct Loop
	{
		bool any;
		std::string labelBegin, labelEnd;
	};
	struct Var
	{
		std::string name;
		std::string internal;
		bool stackAlloc;
		bool mut;
	};
	struct Env
	{
		EnvPtr parent;
		std::vector<Var> vars;
		Loop loop;

		Env (CompileUnit* cunit, EnvPtr parent);
		Var get (const std::string& name) const;
	};

	CompileUnit (Compiler* comp, OverloadPtr overload, SigPtr sig);
	CompileUnit (Compiler* comp, OverloadPtr overload,
	               SigPtr sig, TyPtr ret,
	               const std::string& intName);

	void compile ();

	void writePrefix (EnvPtr env);
	void writeEnd ();
	void writeUnroot ();
	void output (std::ostream& out);
	
	void stackAlloc (const std::string& name);
	void stackStore (const std::string& name, 
						const std::string& value);

	EnvPtr makeEnv (EnvPtr parent = nullptr);
	std::string makeUnique (const std::string& str);
	std::string makeGlobalString (const std::string& str,
	                                bool nullterm = false);

	std::string getTemp ();
	void pushLifetime ();
	void popLifetime ();

	bool needsRetain (ExpPtr exp);
	bool doesTailCall (ExpPtr exp) const;
	void findTailCalls (ExpPtr exp);

	std::string compile (ExpPtr exp, EnvPtr env,
					bool retain = true);

	std::string compileString (ExpPtr e, EnvPtr env);
	std::string compileReal (ExpPtr e, EnvPtr env);
	std::string compileVar (ExpPtr e, EnvPtr env);
	std::string compileCall (ExpPtr e, EnvPtr env);
	std::string compileLet (ExpPtr e, EnvPtr env);
	std::string compileBlock (ExpPtr e, EnvPtr env);
	std::string compileCond (ExpPtr e, EnvPtr env);
	std::string compileLambda (ExpPtr e, EnvPtr env);
	std::string compileAssign (ExpPtr e, EnvPtr env);
	std::string compileLoop (ExpPtr e, EnvPtr env);
	std::string compileList (ExpPtr e, EnvPtr env);
	std::string compileiGet (ExpPtr e, EnvPtr env);
	std::string compileiTag (ExpPtr e, EnvPtr env);
};



class Compiler
{
public:
	Compiler ();
	~Compiler ();

	static std::string mangle (const std::string& ident);
	void setUniquePrefix (const std::string& prefix);
	std::string genUniqueName (const std::string& name = "");

	CompileUnit* compile (OverloadPtr overload, SigPtr sig);
	CompileUnit* bake (OverloadPtr overload,
						SigPtr sig, TyPtr ret,
						const std::string& internalName);

	void entryPoint (CompileUnit* cunit);
	void output (std::ostream& os);

	void addExternal (const std::string& name);
	void addInclude (CompileUnit* cunit);
private:
	std::ostringstream _ssPrefix;
	std::set<std::string> _externals;
	std::set<std::string> _includes;
	std::vector<CompileUnit*> _units;
	std::string _uniquePrefix;
	int _nameId;

	CompileUnit* _entry;

	void _addInclude (const std::string& name, size_t nargs);

	void outputRuntimeHeader (std::ostream& os); 
	void outputEntryPoint (std::ostream& os);
};

