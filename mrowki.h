#ifndef MROWKI_H
#define MROWKI_H

#include <vector>
#include <algorithm>
#include <boost/random.hpp>
#include "graf.h"
#include "utils.h"

inline double obecny_poziom_feromonu(double zawartosc, int nr_tury)
{
	return zawartosc * std::pow(0.999, nr_tury);
}

template<typename Derived, typename RandomNumberGenerator>
class MrowkaBase
{
private:
	GrafZFeromonami::vertex_descriptor stara_pozycja_;
	GrafZFeromonami::vertex_descriptor pozycja_;
	GrafZFeromonami* graf_;
	RandomNumberGenerator ran_;

	int nr_tury_;

	void ustaw_nowa_pozycje(GrafZFeromonami::vertex_descriptor nowa_pozycja)
	{
		stara_pozycja_ = pozycja_;
		pozycja_ = nowa_pozycja;
	}

protected:
	const Derived& derived() const
	{
		return *static_cast<const Derived*>(this);
	}
	Derived& derived()
	{
		return *static_cast<Derived*>(this);
	}
public:
	MrowkaBase(GrafZFeromonami& graf, GrafZFeromonami::vertex_descriptor pozycja, RandomNumberGenerator ran) :
		stara_pozycja_(pozycja),
		pozycja_(pozycja),
		graf_(&graf),
		ran_(ran),
		nr_tury_(0)
	{

	}

	bool nastepny_ruch()
	{
		auto& zmienialny_graf = *graf_;
		const auto& graf = zmienialny_graf;

		auto list = boost::out_edges(pozycja_, graf);
		std::vector<GrafZFeromonami::edge_descriptor> edges(list.first, list.second);

		if(edges.empty())
			return false;

		std::vector<double> wartosci_feromonow;
		std::transform(edges.begin(), edges.end(), std::back_inserter(wartosci_feromonow), [&](GrafZFeromonami::edge_descriptor e)
		{
			if(boost::target(e, graf) == stara_pozycja_)
				return 0.0;

			double poziom_feromonu = zmienialny_graf[e].feromony.load();
			poziom_feromonu = obecny_poziom_feromonu(poziom_feromonu, nr_tury_);
			poziom_feromonu = std::pow(poziom_feromonu, 1.1) + 1;
			return poziom_feromonu;
		});

		// wyl¹duj na docelowym wierzcho³ku w zale¿noœci od wartoœci feromonu
		boost::random::discrete_distribution<> dist(wartosci_feromonow.begin(), wartosci_feromonow.end());
		GrafZFeromonami::edge_descriptor wylosowana_krawedz = edges[dist(ran_)];

		ustaw_nowa_pozycje(boost::target(wylosowana_krawedz, graf));

		std::vector<GrafZFeromonami::vertex_descriptor> docelowe;
		{
			auto list = boost::out_edges(pozycja_, graf);
			std::transform(list.first, list.second, std::back_inserter(docelowe), [&graf](GrafZFeromonami::edge_descriptor e)
			{
				return boost::target(e, graf);
			});
		}

		interlocked_increase(zmienialny_graf[wylosowana_krawedz].feromony, derived().ocen_wierzcholek(pozycja_, docelowe));

		++nr_tury_;
		return true;
	}

	int wykonano_ruchow() const
	{
		return nr_tury_;
	}

	typedef RandomNumberGenerator random_number_generator;

	// G£UPOTA VISUAL STUDIO
	// to powinno dzia³aæ, bo C++11, ale to VS2013, jeszcze nie dojrza³ na tyle.
	//MrowkaBase(const MrowkaBase&) = delete;
	//MrowkaBase& operator=(const MrowkaBase&) = delete;
	//MrowkaBase(MrowkaBase&&) = default;
	//MrowkaBase& operator=(MrowkaBase&&) = default;
};

class MrowkaKlika : public MrowkaBase<MrowkaKlika, boost::random::minstd_rand>
{
private:
	std::vector<GrafZFeromonami::vertex_descriptor> poprzedni_zbior;
	std::vector<GrafZFeromonami::vertex_descriptor> poprzedni_wspolni;
public:
	MrowkaKlika(GrafZFeromonami& graf, GrafZFeromonami::vertex_descriptor pozycja, random_number_generator ran) :
		MrowkaBase(graf, pozycja, ran)
	{

	}

	double ocen_wierzcholek(GrafZFeromonami::vertex_descriptor ten_wierzcholek, const std::vector<GrafZFeromonami::vertex_descriptor>& sasiedzi)
	{
		assert(!sasiedzi.empty());
		double ile_wspolnych_twoback = std::count_if(sasiedzi.begin(), sasiedzi.end(), [&](GrafZFeromonami::vertex_descriptor v)
		{
			return std::find(poprzedni_wspolni.begin(), poprzedni_wspolni.end(), v) != poprzedni_zbior.end();
		});//Tutaj patrzymy na poprzedni wspolny zbior i na nowy zbior, powinno zapobiegac znajdowaniu bliskich klik.
		poprzedni_wspolni.clear();
		double ile_wspolnych = std::count_if(sasiedzi.begin(), sasiedzi.end(), [&](GrafZFeromonami::vertex_descriptor v)
		{
			bool b = std::find(poprzedni_zbior.begin(), poprzedni_zbior.end(), v) != poprzedni_zbior.end();
			if(b) poprzedni_wspolni.push_back(v);
			return b;
		});//Tutaj patrzymy na poprzedni zbior i na nowy zbior, dodajac do zbioru wspolnych ktory bedzie uzyty w nastepnym ruchu.
		double ilosc_sasiadow = sasiedzi.size();
		poprzedni_zbior = sasiedzi;
		return (ile_wspolnych_twoback / ilosc_sasiadow) * 200 * std::pow(ilosc_sasiadow, 0.75) + (ile_wspolnych / ilosc_sasiadow) * 80 * std::pow(ilosc_sasiadow, 0.5);
	}
};

template<typename RandomNumberGenerator>
void mrowki(GrafZFeromonami& graf, RandomNumberGenerator& rng, const int ilosc_mrowek, const int ilosc_watkow = std::thread::hardware_concurrency())
{
	typedef MrowkaKlika Mrowka;
	const int ilosc_ruchow = 5000;
	assert(ilosc_watkow > 0);

	std::vector<std::vector<Mrowka>> mrowiska(ilosc_watkow);

	auto wypeln_mrowiska = [&mrowiska, &graf, &rng, &ilosc_mrowek]()
	{
		for(int i = 0; i < ilosc_mrowek; ++i)
		{
			const int obecne_mrowisko = i % mrowiska.size();
			mrowiska[obecne_mrowisko].push_back(Mrowka(graf, boost::random_vertex(graf, rng), Mrowka::random_number_generator(rng())));
		}
	};
	wypeln_mrowiska();

	auto symuluj_akcje_mrowek = [&graf, &mrowiska, &ilosc_watkow, &ilosc_ruchow]()
	{
		std::vector<std::future<void>> wyniki(ilosc_watkow);
		for(int i = 0; i < ilosc_watkow; ++i)
		{
			wyniki[i] = std::async(std::launch::async, [](std::vector<Mrowka>& mrowisko, const int ile_ruchow)
			{
				for(int j = 0; j < ile_ruchow; ++j)
					for(auto& mrowka : mrowisko)
						mrowka.nastepny_ruch();
			}, std::ref(mrowiska[i]), ilosc_ruchow);
		}
		for(auto& wynik : wyniki)
			wynik.wait();
	};
	symuluj_akcje_mrowek();

	auto ustaw_oczekiwane_wartosci_feromonu = [&graf, &ilosc_ruchow]()
	{
		auto edges = boost::edges(graf);
		std::for_each(edges.first, edges.second, [&](GrafZFeromonami::edge_descriptor v)
		{
			double x = graf[v].feromony.load();
			x = obecny_poziom_feromonu(x, ilosc_ruchow);
			graf[v].feromony.store(x);
		});
	};
	ustaw_oczekiwane_wartosci_feromonu();
}

#endif
