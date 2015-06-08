#include "stdafx.h"

double interlocked_increase(std::atomic<double>& value, double what)
{
	double stare = value.load();
	double nowe;
	do
	{
		nowe = stare + what;
	} while(!value.compare_exchange_weak(stare, nowe));
	return stare;
}

double interlocked_increase_with_saturation(std::atomic<double>& value, const double max, double what)
{
	double stare = value.load();
	double nowe;
	do
	{
		nowe = stare + what;
		nowe = std::min(nowe, max);
	} while(!value.compare_exchange_weak(stare, nowe));
	return stare;
}