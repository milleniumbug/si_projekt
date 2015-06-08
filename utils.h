#ifndef UTILS_H
#define UTILS_H

#include <atomic>

// interlocked_increase
// atomowo zwiêksza wartoœæ `value` o `what`
// zwraca star¹ wartoœæ `value` przed inkrementacj¹
double interlocked_increase(std::atomic<double>& value, double what);

// interlocked_increase_with_saturation
// atomowo albo zwiêksza `value` o `what` albo ustawia `value` na `max`, w zale¿noœci od tego która jest wiêksza
// zwraca star¹ wartoœæ `value` przed inkrementacj¹
double interlocked_increase_with_saturation(std::atomic<double>& value, double max, double what);

template<typename Zbior, typename Element>
bool jest_w_zbiorze(const Zbior& zb, const Element& el)
{
	return zb.find(el) != zb.end();
}

#endif