// Wikipedia_finder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "container.h"
#include "loader.h"
#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
	References* ref = LoadFile("H:\\wikibulk\\res\\temp-po_linkach-lista-simple-201201042.txt");

	size_t* p = ref->Find(293800);
	while (*p != NULL)
	{
		std::cout << (*p) << std::endl;
		p++;
	}

	return 0;
}

