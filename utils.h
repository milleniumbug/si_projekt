#ifndef UTILS_H
#define UTILS_H

#include <atomic>

// interlocked_increase
// atomowo zwi�ksza warto�� `value` o `what`
// zwraca star� warto�� `value` przed inkrementacj�
double interlocked_increase(std::atomic<double>& value, double what);

// interlocked_increase_with_saturation
// atomowo albo zwi�ksza `value` o `what` albo ustawia `value` na `max`, w zale�no�ci od tego kt�ra jest wi�ksza
// zwraca star� warto�� `value` przed inkrementacj�
double interlocked_increase_with_saturation(std::atomic<double>& value, double max, double what);

template<typename Zbior, typename Element>
bool jest_w_zbiorze(const Zbior& zb, const Element& el)
{
	return zb.find(el) != zb.end();
}

#endif