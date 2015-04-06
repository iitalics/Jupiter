#pragma once
#include "Jupiter.h"
#include <sstream>


class Ty;
using TyPtr = std::shared_ptr<Ty>;
using TyList = std::vector<TyPtr>;


enum TyKind
{
	tyInvalid = 0,
	tyConcrete,
	tyPoly,
	tyPolyNamed,
	tyOverloaded,
	tyWildcard
};

class Ty
{
public:
	static TyPtr makeConcrete (const std::string& t,
						const TyList& sub = {});
	static TyPtr makePoly (int idx);
	static TyPtr makePolyNamed (const std::string& name);
	static inline TyPtr makeWildcard ()
	{
		return std::make_shared<Ty>(tyWildcard);
	}

	explicit Ty (TyKind k = tyInvalid);
	~Ty ();

	TyKind kind;
	TyList subtypes;
	std::string name;
	int idx;

	inline bool operator== (TyKind k) const
	{ return kind == k; }
	inline bool operator!= (TyKind k) const
	{ return kind != k; }

	std::string string () const;
private:
	void _string (std::ostringstream& ss) const;
};