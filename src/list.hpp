#pragma once
#include <initializer_list>
#include <iterator>
#include <vector>

template <typename T>
class list
{
public:
	struct pair
	{
		T head;
		std::shared_ptr<pair> tail;

		pair (T _head, const std::shared_ptr<pair>& _tail = nullptr)
			: head(_head), tail(_tail) {}
	};
	using pairPtr = std::shared_ptr<pair>;
	struct iterator
	{
		T operator* () const { return _pair->head; }
		bool operator!= (const iterator& other) const { return _pair != other._pair; }
		iterator& operator++ () { _pair = _pair->tail; return *this; }

	private:
		friend class list;
		iterator (pairPtr p)
			: _pair(p) {}
		pairPtr _pair;
	};

	inline list ()
		: _pair(nullptr) {}
	inline explicit list (T hd, const list& tail = list())
		: _pair(std::make_shared<pair>(hd, tail._pair)) {}
	inline list (const list& other)
		: _pair(other._pair) {}
	list (const std::initializer_list<T>& init)
		: _pair(nullptr)
	{
		for (auto it = init.end(); it-- != init.begin(); )
			_pair = std::make_shared<pair>(*it, _pair);
	}
	list (const std::vector<T>& init)
		: _pair(nullptr)
	{
		for (auto it = init.end(); it-- != init.begin(); )
			_pair = std::make_shared<pair>(*it, _pair);
	}

	~list () {}

	bool nil () const { return _pair == nullptr; }
	T head () const
	{
		if (nil())
			throw std::runtime_error("nil has no head");
		else
			return _pair->head;
	}
	list tail () const
	{
		if (nil())
			throw std::runtime_error("nil has no tail");
		else
			return list(_pair->tail);
	}

	iterator begin () const { return iterator(_pair); }
	iterator end () const { return iterator(nullptr); }
private:
	list (const pairPtr& p)
		: _pair(p) {}

	pairPtr _pair;
};

template <typename T>
std::ostream& operator<< (std::ostream& s, const list<T>& lst)
{
	bool first = true;
	s << "[";
	for (auto t : lst)
	{
		if (first)
		{
			s << " ";
			first = false;
		}
		else
			s << ", ";

		s << t;
	}
	if (!first)
		s << " ";
	return s << "]";
}