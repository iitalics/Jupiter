#include "Build.h"
#include "Desugar.h"
#include <fstream>


static char hex_char (int k)
{
	if (k < 10)
		return char('0' + k);
	else
		return char('a' + k - 10);
}
std::string Module::mangle (const std::string& name)
{
	// NOTE: different from Compiler::mangle()
	std::ostringstream ss;

	for (auto c : name)
		if (c == '_')      ss << "_u";
		else if (c == '+') ss << "_p";
		else if (c == '*') ss << "_t";
		else if (c == '<') ss << "_l";
		else if (c == '>') ss << "_g";
		else if (c == '?') ss << "_i";
		else if (c == '#') ss << "_j";
		else if (c == '/') ss << ".";
		else if (c == '.') ss << "_.";
		else if (c == '=' || c == '-')
			ss << c;
		else if (!(isalpha(c) || isdigit(c)))
			ss << "_" << hex_char(c >> 4) << hex_char(c & 0xf);
		else
			ss << c;

	return ss.str();
}
static std::string makeFolder (const std::string& str)
{
	if (str.back() != '/')
		return str + "/";
	else
		return str;
}
static bool fileExists (const std::string& path)
{
	std::ifstream fs(path);
	if (fs.good())
	{
		fs.close();
		return true;
	}
	else
		return false;
}



Build::Build (const std::string& buildFolder)
	: _compileMode(buildFolder.empty() ? Single : Project),
	  _buildFolder(makeFolder(buildFolder)),
	  _entry(nullptr)
{
	_paths.insert(JUP_LIB_PATH(""));
	_paths.insert("./");
}
Build::~Build ()
{
	for (auto& comp : _compilers)
		delete comp;
}



void Build::addPath (const std::string& path)
{
	_paths.insert(makeFolder(path));
}



std::string Build::findPath (const std::string& name)
{
	if (name.find("#") != std::string::npos)
		return "";

	for (auto& prefix : _paths)
	{
		auto fullpath1 = prefix + name;
		auto fullpath2 = prefix + name + ".j";

		if (fileExists(fullpath1))
			return fullpath1;
		if (fileExists(fullpath2))
			return fullpath2;
	}
	return "";
}
ModulePtr Build::loadModule (const std::string& name)
{
	auto it = _modules.find(name);
	if (it != _modules.end())
		return it->second;

	auto fullpath = findPath(name);
	if (fullpath.empty())
	{
		std::ostringstream ss;
		ss << "could not locate module '" << name << "'";
		throw Span().die(ss.str());
	}

	Lexer lex;
	lex.openFile(fullpath);
	auto proto = Parse::parseToplevel(lex);
	auto module = std::make_shared<Module>(name, proto);
	_modules[name] = module;

	for (auto& imp : proto.imports)
	{
		auto src = loadModule(imp.name);
		import(module, src, Module::StrongImport);
	}

	auto infopath = module->infodataPath(_buildFolder);
	if (fileExists(infopath))
		_getCompiler(module)->readInfodata(infopath);

	return module;
}

void Build::import (ModulePtr dest, ModulePtr src, int str)
{
	if (dest == src)
		return;

	bool importTypes = true;
	bool importFuncs = (str >= Module::StrongImport);

	auto it = dest->importHistory.find(src);
	if (it != dest->importHistory.end())
	{
		// already imported everything
		if (it->second >= str)
			return;

		importTypes = false;
		it->second = str;
	}
	else
		dest->importHistory[src] = str;

	if (importTypes)
		for (auto& tyi : src->env.types)
			dest->importTypes.push_back(tyi);
	
	if (importFuncs)
		for (auto& fn : src->env.functions)
			for (auto& overload : fn->overloads)
				if (overload->isPublic)
					dest->importOverloads.push_back(overload);

	for (auto& imp : src->env.proto.imports)
	{
		auto module = loadModule(imp.name);

		if (str == Module::StrongImport)
			import(dest, module,
				imp.isPublic ?
					Module::StrongImport :
					Module::WeakImport);
		else
			import(dest, module, Module::WeakImport);
	}
}


#define ENTRY_NAME "#entry"
void Build::createEntryPoint (const std::string& moduleName)
{
	if (_entry != nullptr)
		throw Span().die("entry point already defined");

	auto entryBody = Exp::make(eCall, {
			Exp::make(eVar, std::string("main"))
		});
	auto proto = GlobProto {
		.funcs = {
			FuncDecl { true, ENTRY_NAME, Sig::make({ }), entryBody, Span() }
		},
		.types = {},
		.imports = {}
	};
	auto entryModule = std::make_shared<Module>(ENTRY_NAME, proto);
	auto target = loadModule(moduleName);	

	import(entryModule, target, Module::StrongImport);

	_entry = entryModule;
}

void Build::finishModuleLoad ()
{
	for (auto& pair : _modules)
		pair.second->finishImport();

	for (auto& pair : _modules)
		pair.second->desugarAll();

	if (_entry != nullptr)
	{
		_entry->finishImport();
		_entry->desugarAll();
	}
}
Compiler* Build::_getCompiler (ModulePtr mod)
{
	if (mod->env.compiler != nullptr)
		return mod->env.compiler;
	else if (_compileMode == Build::Single)
		return mod->env.compiler = _compilers[0];
	else
	{
		auto comp = new Compiler();
		comp->setUniquePrefix(Module::mangle(mod->name));
		_compilers.push_back(comp);
		return (mod->env.compiler = comp);
	}
}
void Build::_output (ModulePtr mod, std::ostream& log)
{
	auto compiler = _getCompiler(mod);
	auto path = mod->outputPath(_buildFolder);
	auto infopath = mod->infodataPath(_buildFolder);

	std::ofstream fs(path);
	if (!fs.good())
		throw Span().die("cannot write to '" + path + "'");
	compiler->output(fs);
	fs.close();

	if (mod != _entry)
	{
		std::ofstream infofs(infopath);
		if (infofs.good())
		{
			compiler->outputInfodata(infofs);
			infofs.close();
		}
	}

	// log which output files were written to
	log << path << std::endl;
}
void Build::compile (std::ostream& out)
{
	if (_entry == nullptr)
		throw Span().die("cannot compile with no entry point!");

	// initialize compiler objects
	if (_compileMode == Build::Single)
	{
		_compilers.push_back(new Compiler());
		_compilers[0]->setUniquePrefix("main");
	}

	for (auto& pair : _modules)
		_getCompiler(pair.second);

	// call entry point code
	auto entryCompiler = _getCompiler(_entry);
	auto entryOverload = _entry->env.getFunc(ENTRY_NAME)->overloads[0];
	auto entryInst = Overload::inst(entryOverload, Sig::make({ }), entryCompiler);
	auto entryCUnit = entryInst.cunit;

	// generate code for C 'main()'
	entryCompiler->entryPoint(entryCUnit);

	// write the code to files/stdout
	if (_compileMode == Build::Single)
		_compilers[0]->output(out);
	else
	{
		for (auto& pair : _modules)
			_output(pair.second, out);

		_output(_entry, out);
	}
}





Module::Module (const std::string& _name, const GlobProto& proto)
	: name(_name), env(proto)
{}

std::string Module::outputPath (const std::string& buildFolder)
{
	return buildFolder + mangle(name) + ".ll";
}
std::string Module::infodataPath (const std::string& buildFolder)
{
	return buildFolder + mangle(name) + ".ll.summary";
}

void Module::finishImport ()
{
	// actually import everything into the global env
	for (auto& ov : importOverloads)
		env.addFunc(ov->name)->overloads.push_back(ov);

	for (auto& tyi : importTypes)
		env.addType(*tyi);

	env.loadBuiltinTypes();

	importHistory.clear();
	importOverloads.clear();
	importTypes.clear();

	// check for duplicates types
	for (size_t i = 0, len1 = env.proto.types.size(); i < len1; i++)
		for (size_t j = 0, len2 = env.types.size(); j < len2; j++)
		{
			// this is hacky because it assumes that all of the
			//   type protos in `env.proto.types` get added to
			//   `env.types` in order: in that case, the type
			//   at `env.types[i]` will be the "real" version of
			//   the prototype at `env.proto.types[i]` for each 'i'
			if (i == j) continue;

			if (env.proto.types[i].name == env.types[j]->name)
			{
				std::ostringstream ss;
				ss << "duplicate type '" << env.proto.types[i].name << "'";
				throw env.proto.types[i].span.die(ss.str());
			}
		}

	// TODO: check for a-equiv function declarations??
}
void Module::desugarAll ()
{
	for (auto& fn : env.functions)
		for (auto& ov : fn->overloads)
		{
			Desugar desu(env);
			desu.desugar(ov);
		}
}
