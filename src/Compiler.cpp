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






CompileUnit::CompileUnit (Compiler* comp, OverloadPtr over,
                            SigPtr sig, TyPtr ret,
                            const std::string& intName)
	: compiler(comp),
	  overload(over),
	  internalName(intName),
	  funcInst(this, sig, ret)
{
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

CompileUnit::CompileUnit (Compiler* comp, OverloadPtr overload, SigPtr sig)
	: compiler(comp),
	  overload(overload),
	  internalName(comp->genUniqueName("fn")),
	  funcInst(this, sig),
	  
	  temps(0)
{
	Infer inf(this, sig);
	funcInst = inf.fn;

	writePrefix();
	writeEnd();

	// TODO: put in arguments 
	auto env = makeEnv();
	for (size_t i = 0, len = sig->args.size(); i < len; i++)
		env->vars.push_back({ sig->args[i].first, -int(i + 1) });

	auto life = Lifetime(this);
	auto res = compileOp(compile(overload->body, env, &life));
	ssBody << "ret i8* " << res << std::endl;
}


// Env
CompileUnit::Env::Env (CompileUnit* cunit, EnvPtr _parent)
	: parent(_parent),
	  life(cunit) {}
CompileUnit::Var CompileUnit::Env::get (const std::string& name) const
{
	for (auto& v : vars)
		if (v.name == name)
			return v;

	return parent->get(name);
}
CompileUnit::EnvPtr CompileUnit::makeEnv (EnvPtr parent)
{
	return std::make_shared<Env>(this, parent);
}

// Lifetime
CompileUnit::Lifetime::Lifetime (CompileUnit* _cunit)
	: cunit(_cunit) {}
CompileUnit::Lifetime::~Lifetime ()
{ relinquish(); }

void CompileUnit::Lifetime::relinquish ()
{
	for (auto idx : regs)
		cunit->regs[idx] = nullptr;
}
int CompileUnit::Lifetime::claim (int idx)
{
	cunit->regs[idx] = this;
	return idx;
}
int CompileUnit::findRegister (Lifetime* life)
{
	// look for available registers
	int idx, len = regs.size();
	for (idx = 0; idx < len; idx++)
		if (regs[idx] == nullptr)
			return life->claim(idx);

	// none found, add one
	regs.push_back(nullptr);


	// declare new register
	ssPrefix << regString(len) << " = alloca i8*" << std::endl;

	return life->claim(len);
}


/*   Compiling   */


void CompileUnit::writePrefix ()
{
	ssPrefix << std::endl << std::endl
	         << ";;; " << overload->name << " : "
	         << funcInst.type()->string() << std::endl
	         << "define fastcc i8* @" << internalName << " (";

	for (size_t i = 0, len = overload->signature->args.size(); i < len; i++)
	{
		if (i > 0)
			ssPrefix << ", ";

		ssPrefix << "i8* " << argString(i);
	}

	ssPrefix << ") {" << std::endl;
}
void CompileUnit::writeEnd ()
{
	ssEnd << "}" << std::endl;
}
void CompileUnit::output (std::ostream& out)
{
	out << ssPrefix.str() << ssBody.str() << ssEnd.str();
}

// \x25 = '%'
std::string CompileUnit::regString (int idx) const
{
	std::ostringstream ss; ss << '%' << "s" << idx;
	return ss.str();
}
std::string CompileUnit::argString (int idx) const
{
	std::ostringstream ss; ss << '%' << "arg" << idx;
	return ss.str();
}
std::string CompileUnit::tempString (int id) const
{
	std::ostringstream ss; ss << '%' << "t" << id;
	return ss.str();
}

std::string CompileUnit::compileOp (const Operand& op)
{
	switch (op.kind)
	{
	case opLit:
		switch (op.src->kind)
		{
		case eInt:
			ssBody << tempString(temps) << " = inttoptr i32 "
			       << ((op.src->get<int_t>() << 1) | 1) << " to i8*"
			       << "  ; int: " << op.src->string() << std::endl;
			break;

		case eBool:
			ssBody << tempString(temps) << "inttoptr i32 "
			       << ((op.src->get<bool>() ? 3 : 0)) << " to i8*"
			       << "  ; boolean: " << op.src->string() << std::endl;
			break;

		default:
			// eLit, eBlock, eTuple as "literal" results
			//  in the unit (represented as null to the runtime)
			return "i8* null";
		}
		break;

	case opVar:
		ssBody << tempString(temps) << " = load i8** "
		       << regString(op.idx) << std::endl;
		break;

	case opArg:
		return argString(op.idx);

	default:
		return "null";
	}

	return tempString(temps++);
}

CompileUnit::Operand CompileUnit::compile (ExpPtr e, EnvPtr env, Lifetime* life)
{
	switch (e->kind)
	{
	case eInt:
	case eBool:
		return { opLit, 0, e };

	case eVar:
		return compileVar(e, env, life);

	case eBlock:
		return compileBlock(e, env, life);

	case eLet:
		return compileLet(e, env, life);

	case eCall:
		return compileCall(e, env, life);

	default: break;
	}

	static std::vector<std::string> kinds = {
		"invalid", "int", "real",
		"string", "bool", "var",
		"tuple", "call", "infix",
		"cond", "lambda", "block",
		"let",
	};

	std::ostringstream ss;
	ss << "not sure how to compile '" << kinds[int(e->kind)] << "'";
	throw e->span.die(ss.str());
}

CompileUnit::Operand CompileUnit::compileVar (ExpPtr e, EnvPtr env, Lifetime* life)
{
	if (e->get<bool>())
		throw e->span.die("can only compile non-global variables");
	else
	{
		auto idx = env->get(e->getString()).idx;

		if (idx < 0) // argument
			return { opArg, -(idx + 1), e };
		else
			return { opVar, idx, e };
	}
}

CompileUnit::Operand CompileUnit::compileBlock (ExpPtr e, EnvPtr env, Lifetime* life)
{
	auto env2 = makeEnv(env);
	Operand op { opLit, 0, e };

	for (size_t i = 0, len = e->subexps.size(); i < len; i++)
	{
		op = compile(e->subexps[i], env2, life);

		// reduce all but last op
		if (i < len - 1)
			compileOp(op);
		// else tail call?
	}

	return op;
}

CompileUnit::Operand CompileUnit::compileCall (ExpPtr e, EnvPtr env, Lifetime* life)
{
	auto fn = e->subexps[0];

	if (fn->kind == eVar && fn->get<bool>()) ; // why is this even
	else
		throw e->span.die("can only compile calls to globals");

	Lifetime life2(this);

	std::vector<Operand> operands;
	std::vector<std::string> args;
	operands.reserve(e->subexps.size() - 1);
	args.reserve(e->subexps.size() - 1);

	for (size_t i = 1, len = e->subexps.size(); i < len; i++)
		operands.push_back(compile(e->subexps[i], env, &life2));

	life2.relinquish();
	auto resIdx = findRegister(life);

	for (auto& op : operands)
		args.push_back(compileOp(op));

	ssBody << tempString(temps) << " = call i8* @"
	       << special[fn] << " (";

	for (size_t i = 0, len = operands.size(); i < len; i++)
	{
		if (i > 0)
			ssBody << ", ";

		ssBody << "i8* " << args[i];
	}

	ssBody << ")" << std::endl
	       << "store i8* " << tempString(temps)
	       << ", i8** " << regString(resIdx) << std::endl;

	temps++;
	return { opVar, resIdx, e };
}


CompileUnit::Operand CompileUnit::compileLet (ExpPtr e, EnvPtr env, Lifetime* life)
{
	auto idx = findRegister(&env->life);
	env->vars.push_back({ e->getString(), idx });

	auto op = compile(e->subexps[0], env, life);
	ssBody << "store i8* " << compileOp(op) << ", i8** " << regString(idx) << std::endl;

	return { opLit, 0, e };
}
