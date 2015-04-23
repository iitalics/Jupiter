#include "Compiler.h"
#include <sstream>
#include <iomanip>
#include <ctime>


Compiler::Compiler ()
	: nameId(0) {}

Compiler::~Compiler ()
{
	for (auto cu : units)
		delete cu;
}

std::string Compiler::genUniqueName (const std::string& prefix)
{
	std::ostringstream ss;
	ss << prefix << "_u" << (nameId++);
	return ss.str();
}

CompileUnit* Compiler::compile (OverloadPtr overload, SigPtr sig)
{
	auto cu = new CompileUnit(this, overload, sig);
	units.push_back(cu);
	return cu;
}
CompileUnit* Compiler::bake (OverloadPtr overload,
                              SigPtr sig, TyPtr ret,
                              const std::string& intName)
{
	auto cu = new CompileUnit(this, overload, sig, ret, intName);
	cu->funcInst.cunit = cu;
	units.push_back(cu);
	return cu;
}
void Compiler::output (std::ostream& os)
{
    auto now = std::time(nullptr);

	os << "; compiled: "
	   << std::asctime(std::localtime(&now))
	   << std::endl << std::endl;
	for (auto cu : units)
		cu->output(os);
}










CompileUnit::Env::Env (CompileUnit* cunit, EnvPtr _parent)
	: parent(_parent) {}

CompileUnit::Var CompileUnit::Env::get (const std::string& name) const
{
	for (auto& v : vars)
		if (v.name == name)
			return v;
	return parent->get(name);
}



// baked
CompileUnit::CompileUnit (Compiler* comp, OverloadPtr over,
                            SigPtr sig, TyPtr ret,
                            const std::string& intName)
	: compiler(comp),
	  overload(over),
	  internalName(intName),
	  funcInst(this, sig, ret)
{
	// declare baked signature
	ssPrefix << "declare i8* @" << intName
	         << " (";

	for (size_t i = 0, len = sig->args.size(); i < len; i++)
	{
		if (i > 0)
			ssPrefix << ", ";

		ssPrefix << "i8*";
	}
	ssPrefix << ")" << std::endl;
}



CompileUnit::EnvPtr CompileUnit::makeEnv (EnvPtr parent)
{
	return std::make_shared<Env>(this, parent);
}
std::string CompileUnit::makeUnique (const std::string& str)
{
	// TODO: binary search? or too much
	bool unique;
	std::string res = str;

	for (int k = 2; ; k++)
	{
		unique = true;
		for (auto& nu : nonUnique)
			if (nu == res)
			{
				unique = false;
				break;
			}

		if (unique)
		{
			nonUnique.push_back(res);
			return "%" + res;
		}
		else
		{
			std::ostringstream ss;
			ss << str << k;
			res = ss.str();
		}
	}
}
std::string CompileUnit::getTemp ()
{
	size_t i, len;
	for (i = 0, len = tempLifetimes.size(); i < len; i++)
	{
		if (tempLifetimes[i] == -1)
			break;
	}
	
	if (i >= len)
		tempLifetimes.push_back(lifetime);
	else
		tempLifetimes[i] = lifetime;

	std::ostringstream ss;
	ss << "%.t" << i;

	if (i >= len)
		stackAlloc(ss.str());
	return ss.str();
}
void CompileUnit::pushLifetime () { lifetime++; }
void CompileUnit::popLifetime ()
{
	for (size_t i = 0, len = tempLifetimes.size(); i < len; i++)
		if (tempLifetimes[i] >= lifetime)
			tempLifetimes[i] = -1;

	lifetime--;
}





// normal
CompileUnit::CompileUnit (Compiler* comp, OverloadPtr overload, SigPtr sig)
	: compiler(comp),
	  overload(overload),
	  internalName(comp->genUniqueName("fn")),
	  funcInst(this, sig),

	  lifetime(0)
{
	Infer inf(this, sig);
	funcInst = inf.fn;

	// TODO: put in arguments 
	auto env = makeEnv();
	for (size_t i = 0, len = sig->args.size(); i < len; i++)
		env->vars.push_back({
			sig->args[i].first,
			makeUnique(sig->args[i].first),
			false });

	writePrefix(env);
	writeEnd();

	auto res = compile(overload->body, env, false);
	ssBody << "ret " << res << std::endl;
}

void CompileUnit::output (std::ostream& out)
{
	out << ssPrefix.str() << ssBody.str() << ssEnd.str();
}
void CompileUnit::writePrefix (EnvPtr env)
{
	ssPrefix << std::endl << std::endl << std::endl
	         << ";;;   " << funcInst.name << " "
	         << funcInst.type()->string() << std::endl
	         << "define fastcc i8* @"
	         << internalName << " (";

	for (size_t i = 0, len = env->vars.size(); i < len; i++)
	{
		if (i > 0)
			ssPrefix << ", ";

		ssPrefix << "i8* " << env->vars[i].internal;
	}
	ssPrefix << ") unnamed_addr" << std::endl
	         << "{" << std::endl;
}
void CompileUnit::writeEnd ()
{
	ssEnd << "}" << std::endl;
}
void CompileUnit::stackAlloc (const std::string& name)
{
	ssPrefix << name << " = alloca i8*" << std::endl;
}



std::string CompileUnit::compile (ExpPtr e, EnvPtr env, bool retain)
{
	/*
	non-retained:
		int
		bool
		var

	retained:
		everything else
	*/

	if (retain && e->kind != eInt && e->kind != eBool && e->kind != eVar)
	{
		auto res = compile(e, env, false);
		auto temp = getTemp();
		ssBody << "store " << res << ", i8** " << temp << std::endl;
		return res;
	}

	std::ostringstream ss;

	switch (e->kind)
	{
	case eInt:
		ss << "i8* inttoptr (i32 "
		   << (1 | (e->get<int_t>() << 1))
		   << " to i8*)";
		return ss.str();

	case eBool:
		if (e->get<bool>())
			return "i8* null";
		else
			return "i8* inttoptr (i32 1 to i8*)";

	case eVar:   return compileVar(e, env);
	case eCall:  return compileCall(e, env);
	case eLet: return   compileLet(e, env);
	case eBlock: return compileBlock(e, env);

	default:
		throw e->span.die("cannot compile expression: " + e->string());
	}
}

std::string CompileUnit::compileVar (ExpPtr e, EnvPtr env)
{
	if (e->get<bool>()) // global
		throw e->span.die("cannot compile 'naked' globals");
	else
	{
		auto var = env->get(e->getString());

		if (var.stackAlloc)
		{
			auto val = makeUnique(".v");
			ssBody << val << " = load i8** " << var.internal << std::endl;

			return "i8* " + val;
		}
		else
			return "i8* " + var.internal;
	}
}

std::string CompileUnit::compileCall (ExpPtr e, EnvPtr env)
{
	std::ostringstream args;
	auto fn = e->subexps.front();

	if (fn->kind == eVar && fn->get<bool>()) ;
	else
		throw fn->span.die("cannot call non-global");

	pushLifetime();
	for (size_t i = 1, len = e->subexps.size(); i < len; i++)
	{
		if (i > 1)
			args << ", ";

		args << compile(e->subexps[i], env, true);
	}
	popLifetime();

	auto res = makeUnique(".r");

	ssBody << res << " = call i8* @"
	       << special[fn] << "(" << args.str() << ")" << std::endl;

	return "i8* " + res;
}

std::string CompileUnit::compileLet (ExpPtr e, EnvPtr env)
{
	auto internal = makeUnique(e->getString());
	stackAlloc(internal);

	env->vars.push_back({
		e->getString(),
		internal,
		true
	});

	auto res = compile(e->subexps.front(), env, false);
	ssBody << "store " << res << ", i8** " << internal << std::endl;

	return "i8* null"; // returns unit
}

std::string CompileUnit::compileBlock (ExpPtr e, EnvPtr env)
{
	std::string res = "i8* null";

	for (auto e2 : e->subexps)
		res = compile(e2, env, false);

	return res;
}
