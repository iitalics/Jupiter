#include "Compiler.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <fstream>
#include "Compiler.cc"

static bool needs_escape (char c)
{
	static const char list[] = 
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789`-=[];',./""~!@#"
		"$%^&*()_+{}|:<>? ";

	for (size_t i = 0; i < sizeof(list) - 1; i++)
		if (list[i] == c)
			return false;
	return true;
}

static char hex_char (int k)
{
	if (k < 10)
		return char('0' + k);
	else
		return char('a' + k - 10);
}

static std::string escapeString (const std::string& str)
{
	std::ostringstream ss;
	for (size_t i = 0, len = str.size(); i < len; i++)
		if (needs_escape(str[i]))
			ss << "\\"
			   << hex_char(int(str[i]) / 16)
			   << hex_char(int(str[i]) & 0xf);
		else
			ss << str[i];
	return ss.str();
}


std::string Compiler::mangle (const std::string& ident)
{
	std::ostringstream ss;

	for (auto c : ident)
		if (c == '_') ss << "__";
		else if (c == '-') ss << "_mn";
		else if (c == '+') ss << "_pl";
		else if (c == '*') ss << "_st";
		else if (c == '/') ss << "_sl";
		else if (c == '<') ss << "_ls";
		else if (c == '>') ss << "_gr";
		else if (c == '=') ss << "_eq";
		else if (c == '?') ss << "_is";
		else if (c == '\'') ss << "_pr";
		else if (c == '#') ss << "_J";
		else if (!(isalpha(c) || isdigit(c)))
			ss << "_";
		else
			ss << c;

	return ss.str();
}




Compiler::Compiler ()
	: _uniquePrefix(""), _nameId(0), _needsHeader(true), _entry(nullptr) {}

Compiler::~Compiler ()
{
	for (auto cu : _units)
		delete cu;
}

void Compiler::addExternal (const std::string& name)
{
	if (_externals.insert(name).second)
	{
		_ssPrefix << "declare i8* @" << name << " (...)" << std::endl;
		_addedExternal(name);
	}
}
void Compiler::addInclude (CompileUnit* cunit)
{
	if (_includes.insert(cunit->internalName).second)
	{
		auto over = cunit->overload;
		_addedInclude(cunit->internalName,
			over->signature->args.size() +
				(over->hasEnv ? 1 : 0));
	}
}

void Compiler::setUniquePrefix (const std::string& prefix)
{
	_uniquePrefix = prefix + ".";
}
std::string Compiler::genUniqueName (const std::string& prefix)
{
	std::ostringstream ss;
	ss << _uniquePrefix << prefix << "_" << (_nameId++);
	return ss.str();
}

CompileUnit* Compiler::compile (OverloadPtr overload, SigPtr sig)
{
	auto cu = new CompileUnit(this, overload, sig);
	_units.push_back(cu);
	return cu;
}
CompileUnit* Compiler::bake (OverloadPtr overload,
                              SigPtr sig, TyPtr ret,
                              const std::string& intName)
{
	auto cu = new CompileUnit(this, overload, sig, ret, intName);
	cu->funcInst.cunit = cu;
	_units.push_back(cu);
	return cu;
}
void Compiler::entryPoint (CompileUnit* cunit)
{
	if (_entry != nullptr)
		throw cunit->overload->signature->
				span.die("only one entry point per program allowed");

	_entry = cunit;
}
void Compiler::output (std::ostream& os)
{
	if (_needsHeader)
		outputRuntimeHeader(os);

	os << _ssPrefix.str();

	for (auto cu : _units)
		cu->output(os);

	if (_entry != nullptr)
		outputEntryPoint(os);
}
void Compiler::outputRuntimeHeader (std::ostream& os)
{
	std::ifstream libfs(JUP_LIB_PATH("runtime.ll"));

	if (!libfs)
		throw Span().die("fatal: unable to access runtime header '"
		                   JUP_LIB_PATH("runtime.ll") "'");

	os << libfs.rdbuf() << std::endl;
}
void Compiler::outputEntryPoint (std::ostream& os)
{
	os << std::endl
	   << ";;;   < jupiter entry point >" << std::endl
	   << "define ccc i32 @main (i32 %argc, i8** %argv)" << std::endl << "{" << std::endl
	   << "call void @ju_init ()" << std::endl
	   << "call " JUP_CCONV " i8* @" << _entry->internalName << " ()" << std::endl
	   << "call void @ju_destroy ()" << std::endl
	   << "ret i32 0" << std::endl
	   << "}" << std::endl;
}




// normal
CompileUnit::CompileUnit (Compiler* comp, OverloadPtr overload, SigPtr sig)
	: compiler(comp),
	  overload(overload),
	  funcInst(this, sig),
	  finishedInfer(false),

	  lifetime(0),
	  nroots(0)
{
	internalName =
		comp->genUniqueName(Compiler::mangle(overload->name));
}

// baked
CompileUnit::CompileUnit (Compiler* comp, OverloadPtr over,
                            SigPtr sig, TyPtr ret,
                            const std::string& intName)
	: compiler(comp),
	  overload(over),
	  internalName(intName),
	  funcInst(this, sig, ret),
	  finishedInfer(true)
{}


void CompileUnit::output (std::ostream& out)
{
	out << ssPrefix.str() << ssBody.str() << ssEnd.str();
}





CompileUnit::EnvPtr CompileUnit::makeEnv (EnvPtr parent)
{
	return std::make_shared<Env>(this, parent);
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





std::string CompileUnit::makeUnique (const std::string& str)
{
	auto res = str;

	for (int k = 2; ; k++)
		if (nonUnique.find(res) == nonUnique.end())
		{
			nonUnique.insert(res);
			return "%" + res;
		}
		else
		{
			std::ostringstream ss;
			if (str[0] == '.')
				ss << str << k;
			else
				ss << str << '.' << k;
			res = ss.str();
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
void CompileUnit::writePrefix (EnvPtr env)
{
	ssPrefix << std::endl << std::endl << std::endl
	         << ";;;   " << funcInst.name << " "
	         << funcInst.type()->string() << std::endl
	         << "define " JUP_CCONV " i8* @"
	         << internalName << " (";

	auto nargs = env->vars.size() + (overload->hasEnv ? 1 : 0);

	for (size_t i = 0, len = env->vars.size(); i < nargs; i++)
	{
		if (i > 0)
			ssPrefix << ", ";

		ssPrefix << "i8* " << (i >= len ? ENV_VAR : env->vars[i].internal);
	}

	ssPrefix << ") unnamed_addr" << std::endl
	         << "{" << std::endl;
}
void CompileUnit::writeEnd ()
{
	ssEnd << "}" << std::endl;
}
void CompileUnit::writeUnroot ()
{
	auto v = makeUnique(".v");
	ssBody << v << " = load i32* %.nroots" << std::endl
	       << "call void @juGC_unroot (i32 " << v << ")" << std::endl;
}
void CompileUnit::stackAlloc (const std::string& name)
{
	ssPrefix << name << " = alloca i8*" << std::endl
	         << "call void @juGC_root (i8** " << name << ")" << std::endl;

	nroots++;
}
void CompileUnit::stackStore (const std::string& name, 
                                const std::string& value)
{
	ssBody << "call void @juGC_store (i8** " << name << ", i8* " << value << ")" << std::endl;
}






void CompileUnit::findTailCalls (ExpPtr exp)
{
	switch (exp->kind)
	{
	case eCall:
		{
			auto fn = exp->subexps[0];
			auto it = special.find(fn);

			if (it != special.end() && it->second == this)
				tailCalls.insert(exp);
			break;
		}

	case eCond:
		findTailCalls(exp->subexps[1]);
		findTailCalls(exp->subexps[2]);
		break;

	case eBlock:
		if (!exp->subexps.empty())
			findTailCalls(exp->subexps.back());
		break;

	default: break;
	}
}
bool CompileUnit::doesTailCall (ExpPtr exp) const
{
	switch (exp->kind)
	{
	case eCall:
		return tailCalls.find(exp) != tailCalls.end();
	// for the love of god tail call these
	case eCond:
		return doesTailCall(exp->subexps[1]) || doesTailCall(exp->subexps[2]);
	case eBlock:
		if (exp->subexps.empty())
			return false;
		else
			return doesTailCall(exp->subexps.back());
	default: return false;
	}
}
bool CompileUnit::needsRetain (ExpPtr exp)
{
	switch (exp->kind)
	{
	case eInt:
	case eBool:
	case eiEnv:
		return false;

	case eVar:
		// access to global functions needs to be retained
		return exp->get<bool>();

	default: return true;
	}
}



void CompileUnit::compile ()
{
	if (finishedInfer)
		return;
	
	auto& sig = funcInst.signature;

	// do type inference
	Infer inf(this, sig);
	finishedInfer = true;

	// create arguments
	auto env = makeEnv();
	for (size_t i = 0, len = sig->args.size(); i < len; i++)
		env->vars.push_back({
			sig->args[i].first,
			makeUnique(Compiler::mangle(sig->args[i].first)),
			false });

	// write basic stuff
	writePrefix(env);
	writeEnd();

	ssBody << std::endl;

	// find tail calls beforehand
	findTailCalls(overload->body);
	auto res = compile(overload->body, env, false);

	if (!tailCalls.empty())
	{
		// root all of the arguments in the case of
		//  a tail call

		std::vector<std::string> tmps;
		tmps.reserve(env->vars.size());
		for (auto& v : env->vars)
		{
			auto tmp = makeUnique(v.name);
			stackAlloc(tmp);
			tmps.push_back(tmp);
		}

		for (size_t i = 0, len = env->vars.size(); i < len; i++)
			ssPrefix << "call void @juGC_store (i8** " << tmps[i]
			         << ", i8* " << env->vars[i].internal << ")" << std::endl;
	}

	// create variable for number of gc roots on stack
	if (nroots > 0 || !tailCalls.empty())
	{
		// 'opt -mem2reg' will convert this stack allocation
		// to a constant
		ssPrefix << "%.nroots = alloca i32" << std::endl
		         << "store i32 " << nroots << ", i32* %.nroots";
	}

	if (nroots > 0)
		writeUnroot();
	
	ssBody << "ret i8* " << res << std::endl;
}


std::string CompileUnit::compile (ExpPtr e, EnvPtr env, bool retain)
{
	// retain the result if needed
	if (retain && needsRetain(e))
	{
		auto res = compile(e, env, false);
		auto temp = getTemp();
		stackStore(temp, res);
		return res;
	}

	std::ostringstream ss;

	switch (e->kind)
	{
	case eInt:
		ss << "inttoptr (i32 "
		   << (1 | (e->get<int_t>() << 1))
		   << " to i8*)";
		return ss.str();

	case eBool:
		if (e->get<bool>())
			return "inttoptr (i32 1 to i8*)";
		else
			return "null";

	case eVar:    return compileVar(e, env);
	case eString: return compileString(e, env);
	case eReal:   return compileReal(e, env);
	case eCall:   return compileCall(e, env);
	case eCond:   return compileCond(e, env);
	case eLambda: return compileLambda(e, env);
	case eAssign: return compileAssign(e, env);
	case eLoop:   return compileLoop(e, env);
	case eBlock:  return compileBlock(e, env);
	case eLet:    return compileLet(e, env);
	case eList:   return compileList(e, env);
	case eiGet:   return compileiGet(e, env);
	case eiTag:   return compileiTag(e, env);
	case eiEnv:   return ENV_VAR;
	case eTuple:
		return "null";

	case eiMake:
	case eiCall:
		throw e->span.die("expression must be called as function"); 


	default:
		throw e->span.die("cannot compile expression: " + e->string());
	}
}

std::string CompileUnit::makeGlobalString (const std::string& str, bool nullterm)
{
	auto strConst = compiler->genUniqueName();

	std::ostringstream strType;
	strType << "[" << (str.size() + (nullterm ? 1 : 0)) << " x i8]";

	ssEnd << "@" << strConst
	      << " = private unnamed_addr constant "
	      << strType.str() << " c\"" << escapeString(str);

	if (nullterm)
		ssEnd << "\\00";

	ssEnd << "\"" << std::endl;

	std::ostringstream ss;
	ss << "getelementptr (" << strType.str() << "* @" << strConst
	   << ", i32 0, i32 0)";
	
	return ss.str();
}

std::string CompileUnit::compileString (ExpPtr e, EnvPtr env)
{
	auto str = makeGlobalString(e->getString());
	auto res = makeUnique(".str");

	ssBody << res << " = call i8* @ju_make_str (i8* "
	       << str << ", i32 " << e->getString().size()
	       << ")" << std::endl;
	
	return res;
}

std::string CompileUnit::compileReal (ExpPtr e, EnvPtr env)
{
	auto res = makeUnique(".real");
	auto val = e->get<real_t>();

	ssBody << res << " = call i8* @ju_make_real (double 0x";

	// hex representation of double
	int64_t i64cast = 0;
	*((double*) &i64cast) = val;
	for (size_t i = 16; i-- > 0; )
		ssBody << hex_char((i64cast >> (4 * i)) & 0xf);

	ssBody << ")" << std::endl;

	return res;
}

std::string CompileUnit::compileVar (ExpPtr e, EnvPtr env)
{
	if (e->get<bool>()) // global
	{
		auto val = makeUnique(".g");
		auto cunit = special[e];

		ssBody << val << " = call i8* (i8*, i32, ...)* @ju_closure ("
			   << "i8* bitcast (i8* ("
			   << joinCommas(cunit->overload->signature->args.size(), "i8*")
			   << ")* @" << cunit->internalName << " to i8*), i32 0)" << std::endl;
		
		return val;
	}
	else
	{
		auto var = env->get(e->getString());

		if (var.stackAlloc)
		{
			auto val = makeUnique(".v");
			ssBody << val << " = load i8** " << var.internal << std::endl;

			if (var.mut)
			{
				auto unbox = makeUnique(".ub");
				ssBody << unbox << " = call i8* @ju_get (i8* " << val << ", i32 0)" << std::endl;
				
				return unbox;
			}
			else
				return val;
		}
		else
			return var.internal;
	}
}

std::string CompileUnit::compileCall (ExpPtr e, EnvPtr env)
{
	std::ostringstream call;
	auto fn = e->subexps.front();
	std::string closure = "";

	size_t nargs = e->subexps.size() - 1;
	auto isTail = doesTailCall(e);

	if (fn->kind == eVar && fn->get<bool>())
	{
		// call global function
		auto cunit = special[fn];
		call << "call " JUP_CCONV " i8* @" << cunit->internalName << " (";
	}
	else if (fn->kind == eiMake)
	{
		// ^make construct for manually creating runtime objects
		auto tag = GlobEnv::getTag(fn->getString());

		call << "call i8* (i32, i32, i32, ...)* @ju_make_buf (i32 "
		     << tag << ", i32 0, i32 " << nargs;

		if (nargs > 0)
			call << ", ";
	}
	else if (fn->kind == eiCall)
	{
		// ^call construct for calling C runtime/stdlib functions
		call << "call ccc i8* bitcast (i8* (...)* @" << fn->getString()
			 << " to i8* (" << joinCommas(nargs, "i8*") << ")*) (";

		compiler->addExternal(fn->getString());
	}
	else
	{
		// call function value obtained from other expression
		auto data = makeUnique(".d");
		auto fnp = makeUnique(".fn");

		closure = compile(fn, env, true);
		std::ostringstream fntype;
		fntype << "i8* ("
			   << joinCommas(nargs + 1, "i8*")
			   << ")*";

		ssBody << data << " = call i8* @ju_get_fn (i8* " << closure << ")" << std::endl
		       << fnp << " = bitcast i8* " << data << " to " << fntype.str() << std::endl;
		call << "call " JUP_CCONV " " << fntype.str() << " " << fnp << "(";
	}

	pushLifetime();
	for (size_t i = 0; i < nargs; i++)
	{
		if (i > 0)
			call << ", ";

		call << "i8* " << compile(e->subexps[i + 1], env, true);
	}
	popLifetime();

	if (!closure.empty())
	{
		if (nargs > 0)
			call << ", ";

		call << "i8* " << closure;
	}
	call << ")";

	auto res = makeUnique(".r");
	if (isTail)
	{
		// unroot first, then call
		writeUnroot();
		ssBody << res << " = tail " << call.str() << std::endl
		       << "ret i8* " << res << std::endl
		       << std::endl
		       << makeUnique("Lunreach").substr(1) << ":" << std::endl;
	}
	else
		ssBody << res << " = " << call.str() << std::endl;

	return res;
}

std::string CompileUnit::compileLet (ExpPtr e, EnvPtr env)
{
	auto internal = makeUnique(Compiler::mangle(e->getString()));
	stackAlloc(internal);

	auto mut = e->get<bool>();

	// don't create a box for variable already boxed
	//  this is a super hacky way to solve this problem
	//  TODO: make this better
	auto unboxing =
		e->subexps[0]->kind == eiGet &&
		e->subexps[0]->subexps[0]->kind == eiEnv;

	auto boxing = mut && !unboxing;

	env->vars.push_back({
		e->getString(),
		internal,
		true,
		mut,
	});

	if (boxing) pushLifetime();

	auto res = compile(e->subexps[0], env, boxing);

	if (boxing)
	{
		auto box = makeUnique(".box");
		ssBody << box << " = call i8* @ju_make_box (i8* " << res << ")" << std::endl;
		res = box;

		popLifetime();
	}
	stackStore(internal, res);

	return "null"; // returns unit
}

std::string CompileUnit::compileBlock (ExpPtr e, EnvPtr penv)
{
	std::string res = "null";

	bool newEnv = false;
	for (auto e2 : e->subexps)
		if (e2->kind == eLet)
		{
			newEnv = true;
			break;
		}

	// conserve memory
	auto env = newEnv ? makeEnv(penv) : penv;

	for (auto e2 : e->subexps)
		res = compile(e2, env, false);

	return res;
}

std::string CompileUnit::compileCond (ExpPtr e, EnvPtr env)
{
	/*
		br <cond>, <then>, <else>
	*/
	auto res = makeUnique(".cond");
	auto tmp = makeUnique(".t");
	auto cmp = makeUnique(".cmp");
	auto lthen = makeUnique("Lthen");
	auto lelse = makeUnique("Lelse");
	auto lend = makeUnique("Lend");

	// by using standard 'alloca's, the optimizer
	//  can convert the stores into a 'phi' instruction
	// generating 'phi' here isn't an option because of the 
	//  "implications" of the unreachable blocks created by
	//  tail calls
	// there are currently some cases in which the optimizer
	//  generates code that makes tail calls impossible and
	//  i want to know how to mitigate that
	ssBody << tmp << " = alloca i8*" << std::endl
	       << "store i8* null, i8** " << tmp << std::endl;

	auto cond = compile(e->subexps[0], env, false);
	ssBody << cmp << " = icmp ne i8* "
	       << cond << ", null" << std::endl
	       << "br i1 " << cmp
	       << ", label " << lthen
	       << ", label " << lelse << std::endl
	       << std::endl
	       << lthen.substr(1) << ":" << std::endl;

	auto r1 = compile(e->subexps[1], env, false);
	ssBody << "store i8* " << r1 << ", i8** " << tmp << std::endl
	       << "br label " << lend << std::endl
	       << std::endl
	       << lelse.substr(1) << ":" << std::endl;

	auto r2 = compile(e->subexps[2], env, false);
	ssBody << "store i8* " << r2 << ", i8** " << tmp << std::endl
	       << "br label " << lend << std::endl
	       << std::endl
	       << lend.substr(1) << ":" << std::endl;

	ssBody << res << " = load i8** " << tmp << std::endl;
	return res;
}
std::string CompileUnit::compileLoop (ExpPtr e, EnvPtr penv)
{
	auto env = makeEnv(penv);
	auto lbegin = makeUnique("Lloop");
	auto lend = makeUnique("Lend");
	auto body = e->subexps[0];

	ssBody << "br label " << lbegin << std::endl
	       << std::endl
	       << lbegin.substr(1) << ":" << std::endl;

	if (e->subexps.size() > 1)
	{
		auto ldo = makeUnique("Ldo");
		auto cmp = makeUnique(".cond");
		auto cond = compile(e->subexps[0], penv, false);
		ssBody << cmp << " = icmp ne i8* " << cond << ", null" << std::endl
		       << "br i1 " << cmp
		       << ", label " << ldo
		       << ", label " << lend << std::endl
		       << std::endl
		       << ldo.substr(1) << ":" << std::endl;

		body = e->subexps[1];
	}

	env->loop = { true, lbegin, lend };
	compile(body, env, false);

	ssBody << "br label " << lbegin << std::endl
	       << std::endl
	       << lend.substr(1) << ":" << std::endl;

	return "null";
}

std::string CompileUnit::compileiGet (ExpPtr e, EnvPtr env)
{
	pushLifetime();
	auto res = makeUnique(".get");
	auto inp = compile(e->subexps[0], env, true);

	if (e->getString() == "")
		ssBody << res << " = call i8* @ju_get (i8* " << inp
		       << ", i32 " << e->get<int_t>() << ")" << std::endl;
	else
	{
		auto tag = GlobEnv::getTag(e->getString());
		auto str = makeGlobalString(e->getString(), true);

		ssBody << res << " = call i8* @ju_safe_get (i8* "
			   << inp << ", i8* " << str << ", i32 " << tag
			   << ", i32 " << e->get<int_t>() << ")" << std::endl;
	}
	popLifetime();
	return res;
}

std::string CompileUnit::compileiTag (ExpPtr e, EnvPtr env)
{
	auto tag = makeUnique(".tag");
	auto cmp = makeUnique(".cmp");
	auto res = makeUnique(".r");
	auto inp = compile(e->subexps[0], env, true);

	ssBody << tag << " = call i32 @ju_get_tag (i8* " << inp << ")" << std::endl
	       << cmp << " = icmp eq i32 " << tag << ", "
	       << GlobEnv::getTag(e->getString()) << std::endl
	       << res << " = inttoptr i1 " << cmp << " to i8* " << std::endl;

	return res;
}

std::string CompileUnit::compileLambda (ExpPtr e, EnvPtr env)
{
	auto cunit = special[e];
	auto res = makeUnique(".lm");

	std::ostringstream args;

	// make env from variables
	for (size_t i = 1, len = e->subexps.size(); i < len; i++)
	{
		auto name = e->subexps[i]->getString();
		auto var = env->get(name);

		if (var.stackAlloc)
		{
			auto tmp = makeUnique(".v");
			args << ", i8* " << tmp;
			ssBody << tmp << " = load i8** " << var.internal << std::endl;
		}
		else
			args << ", i8* " << var.internal;
	}

	ssBody << res << " = call i8* (i8*, i32, ...)* @ju_closure ("
		   << "i8* bitcast (i8* ("
		   << joinCommas(cunit->overload->signature->args.size() + 1, "i8*")
		   << ")* @" << cunit->internalName
		   << " to i8*), i32 " << (e->subexps.size() - 1) << args.str() << ")" << std::endl;

	return res;
}

std::string CompileUnit::compileAssign (ExpPtr e, EnvPtr env)
{
	auto var = env->get(e->subexps[0]->getString());
	auto res = compile(e->subexps[1], env, false);
	auto box = makeUnique(".box");

	ssBody << box << " = load i8** " << var.internal << std::endl
	       << "call void @ju_put (i8* " << box << ", i32 0, i8* " << res << ")" << std::endl;

	return "null";
}

std::string CompileUnit::compileList (ExpPtr e, EnvPtr env)
{
	static auto tag_nil = GlobEnv::getTag("nil");
	static auto tag_cons = GlobEnv::getTag("cons");

	std::vector<std::string> items;
	items.reserve(e->subexps.size());

	pushLifetime();

	for (auto& e2 : e->subexps)
		items.push_back(compile(e2, env, true));

	auto res = makeUnique(".l");
	std::string temp = "";

	// make 'nil'
	ssBody << res << " = call i8* (i32, i32, i32, ...)* @ju_make_buf(i32 "
	       << tag_nil << ", i32 0, i32 0)" << std::endl;

	// iterate cells backwards
	// let x = [1, 2, 3]
	//    l = nil()
	//    l2 = cons(3, l)
	//    l3 = cons(2, l2)
	//    x = cons(1, l3)
	for (auto it = items.rbegin(); it != items.rend(); ++it)
	{
		auto prev = res;
		res = makeUnique(".l");

		// root prev cell
		if (temp.empty())
			temp = getTemp();
		stackStore(temp, prev);

		// make 'cons'
		ssBody << res << " = call i8* (i32, i32, i32, ...)* @ju_make_buf(i32 "
		       << tag_cons << ", i32 0, i32 2, i8* "
		       << *it << ", i8* " << prev << ")" << std::endl;
	}

	popLifetime();

	return res;
}
