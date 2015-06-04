#ifndef UTILS_H
#define UTILS_H

#include <atomic>

// interlocked_increase
// atomowo zwiêksza wartoœæ `value` o `what`
// zwraca star¹ wartoœæ `value` przed inkrementacj¹
double interlocked_increase(std::atomic<double>& value, double what);

template<typename Zbior, typename Element>
bool jest_w_zbiorze(const Zbior& zb, const Element& el)
{
	return zb.find(el) != zb.end();
}

#endif