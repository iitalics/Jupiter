#pragma once
#include "Env.h"
#include "Compiler.h"
#include <set>
#include <map>

struct Module;
using ModulePtr = std::shared_ptr<Module>;


struct Module
{
	enum { WeakImport = 1, StrongImport = 2 };
	static std::string mangle (const std::string& name);

	Module (const std::string& name,
		const GlobProto& proto);

	void finishImport ();
	void desugarAll ();

	std::string name;
	GlobEnv env;
	std::map<ModulePtr, int> importHistory;
	std::vector<OverloadPtr> importOverloads;
	std::vector<TypeInfo*> importTypes;
};

class Build
{
public:
	enum CompileMode { Single, Project };

	Build (const std::string& buildFolder = "");
	~Build ();

	void addPath (const std::string& path);

	ModulePtr loadModule (const std::string& name);

	void finishModuleLoad ();
	void compile (std::ostream& out);
	void createEntryPoint (const std::string& moduleName);

private:
	CompileMode _compileMode;
	std::string _buildFolder;
	std::set<std::string> _paths;
	std::map<std::string, ModulePtr> _modules;
	std::vector<Compiler*> _compilers;
	ModulePtr _entry;

	Compiler* _getCompiler (ModulePtr mod);
	void _output (ModulePtr mod, std::ostream& log);

	void import (ModulePtr dest, ModulePtr src, int str);
	std::string findPath (const std::string& name);
};