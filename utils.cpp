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