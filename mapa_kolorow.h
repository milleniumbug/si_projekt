#ifndef MAPA_KOLOROW_H
#define MAPA_KOLOROW_H

struct Color
{
	unsigned char r, g, b;
};

std::string as_hex(Color c);
Color daj_kolor(const double min, const double max, const double srodek, const double x);
#endif