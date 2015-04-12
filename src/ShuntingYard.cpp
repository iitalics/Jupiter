#include "Desugar.h"


struct SYard
{
	SYard (Desugar& _des)
		: des(_des) {}

	Desugar& des;
	std::vector<ExpPtr> ops;
	std::vector<ExpPtr> vals;

	void reduce ()
	{
		auto b = vals.back();
		vals.pop_back();

		auto op = ops.back();
		ops.pop_back();

		auto a = vals.back();
		vals.pop_back();

		vals.push_back(Exp::make(eCall, { op, a, b }, a->span + b->span));
	}

	int precOf (ExpPtr e) const
	{
		return std::get<1>(des.global.getPrecedence(e->getString()));
	}
	Assoc assocOf (ExpPtr e) const
	{
		return std::get<2>(des.global.getPrecedence(e->getString()));
	}

	void process (ExpPtr op, ExpPtr val)
	{
		auto prec = precOf(op);
		auto assoc = assocOf(op);

		while (ops.size() > 0 && (
				assoc == Assoc::Left ?
					precOf(ops.back()) <= prec :
					precOf(ops.back()) < prec))
			reduce();

		vals.push_back(val);
		ops.push_back(op);
	}

	ExpPtr result ()
	{
		while (ops.size() > 0)
			reduce();

		return vals[0];
	}
};

ExpPtr Desugar::desugarInfix (ExpPtr e, LocEnvPtr lenv)
{
	SYard syard(*this);
	syard.vals.push_back(desugar(e->subexps[0], lenv));

	for (size_t i = 1, len = e->subexps.size(); i < len; i += 2)
		syard.process(desugarGlobal(e->subexps[i]),
			          desugar(e->subexps[i + 1], lenv));

	return syard.result();
}


