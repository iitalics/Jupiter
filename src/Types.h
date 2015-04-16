#pragma once
#include "Jupiter.h"
#include <sstream>


struct Subs;
class Ty;
using TyPtr = std::shared_ptr<Ty>;
using TyList = list<TyPtr>;


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
	static TyPtr makeOverloaded (const std::string& name);
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

	inline bool operator== (TyKind k) const
	{ return kind == k; }
	inline bool operator!= (TyKind k) const
	{ return kind != k; }

	bool aEquiv (TyPtr other) const;
	std::string string () const;
private:
	void _string (std::ostringstream& ss) const;
	void _concreteString (std::ostringstream& ss) const;

	static TyPtr newPoly (TyPtr ty, Subs& subs);
};