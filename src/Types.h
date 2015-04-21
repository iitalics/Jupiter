#pragma once
#include "Jupiter.h"
#include <sstream>


struct Subs;
class Ty;
class Exp;
struct Sig;
using TyPtr = std::shared_ptr<Ty>;
using TyList = list<TyPtr>;
using ExpPtr = std::shared_ptr<Exp>;
using SigPtr = std::shared_ptr<Sig>;
using ExpList = std::vector<ExpPtr>;



enum TyKind
{
	tyInvalid = 0,
	tyConcrete,
	tyPoly,
	tyOverloaded,

	tyWildcard
};

class Ty
{
public:
	static TyPtr makeConcrete (const std::string& t,
						const TyList& sub = {});
	static TyPtr makePoly (const std::string& name = std::string());
	static TyPtr makeOverloaded (ExpPtr src, const std::string& name);
	static TyPtr makeWildcard ();
	static TyPtr makeInvalid ();
	static TyPtr makeUnit ();
	static inline TyPtr makeFn (const TyList& tys = {})
	{ return makeConcrete("Fn", tys); }

	// create new polytypes
	static TyPtr newPoly (TyPtr ty); 

	explicit Ty (TyKind k = tyInvalid);
	~Ty ();

	TyKind kind;
	TyList subtypes;
	std::string name;
	ExpPtr srcExp;

	inline bool operator== (TyKind k) const
	{ return kind == k; }
	inline bool operator!= (TyKind k) const
	{ return kind != k; }

	bool aEquiv (TyPtr other) const;
	std::string string () const;

	static std::vector<std::string> stringAll (const TyList& tys);
private:
	struct Pretty
	{
		std::ostringstream ss;
		std::vector<const Ty*> poly;
	};
	void _string (Pretty& pr) const;
	void _polyString (Pretty& pr) const;
	void _concreteString (Pretty& pr) const;

	static TyPtr newPoly (TyPtr ty, Subs& subs);
};