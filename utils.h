#ifndef UTILS_H
#define UTILS_H

#include <atomic>

// interlocked_increase
// atomowo zwi�ksza warto�� `value` o `what`
// zwraca star� warto�� `value` przed inkrementacj�
double interlocked_increase(std::atomic<double>& value, double what);

template<typename Zbior, typename Element>
bool jest_w_zbiorze(const Zbior& zb, const Element& el)
{
	return zb.find(el) != zb.end();
}

#endif