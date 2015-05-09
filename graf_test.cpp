#include <iostream>
#include <iomanip>
#include <atomic>
#include <deque>
#include <future>
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
//#include <boost/random/random_device.hpp>
#include <random>
#include <boost/random.hpp>

struct WlasnosciWierzcholka
{
	std::atomic<double> feromony;

	// Stupid workaround for lack support for noncopyable properties in Boost.Graph
	// WARNING, ATTENTION, ALERT, ACHTUNG, UWAGA: NON-ATOMIC
	WlasnosciWierzcholka(const WlasnosciWierzcholka& other)
	{
		this->feromony.store(other.feromony.load());
	}

	// WARNING, ATTENTION, ALERT, ACHTUNG, UWAGA: NON-ATOMIC
	WlasnosciWierzcholka& operator=(const WlasnosciWierzcholka& other)
	{
		this->feromony.store(other.feromony.load());
		return *this;
	}

	WlasnosciWierzcholka() :
		feromony(0.0)
	{
		
	}
};

double obecny_poziom_feromonu(double zawartosc, int nr_tury)
{
	//return zawartosc -= 0.1*nr_tury;
	return zawartosc * std::pow(0.99999, nr_tury);
}

typedef boost::adjacency_list<boost::vecS, 
                              boost::vecS,
                              boost::undirectedS,
							  WlasnosciWierzcholka> GrafZFeromonami;

template<typename Derived, typename RandomNumberGenerator>
class MrowkaBase
{
private:
	GrafZFeromonami::vertex_descriptor pozycja_;
	GrafZFeromonami* graf_;
	RandomNumberGenerator ran_;
	int nr_tury_;

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
		pozycja_(pozycja),
		graf_(&graf),
		ran_(ran),
		nr_tury_(0)
	{
		
	}

	bool nastepny_ruch()
	{
		auto& graf = *graf_;
		auto list = boost::out_edges(pozycja_, graf);
		if(list.first == list.second)
			return false;
		
		std::vector<GrafZFeromonami::vertex_descriptor> docelowe;
		std::vector<double> wartosci_feromonow;
		std::for_each(list.first, list.second, [&](GrafZFeromonami::edge_descriptor e)
		{
			auto docelowy = boost::target(e, graf);
			docelowe.push_back(docelowy);
			// UWAGA - tu dotykamy danych w grafie
			double poziom_feromonu = graf[docelowy].feromony.load();
			poziom_feromonu = obecny_poziom_feromonu(poziom_feromonu, nr_tury_);
			wartosci_feromonow.push_back(poziom_feromonu);
		});
		double ocena = derived().ocen_wierzcholek(pozycja_, docelowe, graf);
		// UWAGA - tu dotykamy danych w grafie
		{
			double stare = graf[pozycja_].feromony.load();
			double nowe;
			do
			{
				nowe = stare + ocena;
			} while(!graf[pozycja_].feromony.compare_exchange_weak(stare, nowe));
		}

		// wyl¹duj na docelowym wierzcho³ku w zale¿noœci od wartoœci feromonu
		boost::random::discrete_distribution<> dist(wartosci_feromonow.begin(), wartosci_feromonow.end());
		int wylosowany_indeks_wierzcholka = dist(ran_);
		pozycja_ = docelowe[wylosowany_indeks_wierzcholka];
		++nr_tury_;
		return true;
	}

	typedef RandomNumberGenerator random_number_generator;

	// JEB SIÊ VISUAL STUDIO
	// PIERDOLONE KURWA ZERO WSPARCIA DLA KONSTRUKTORÓW PRZENOSZ¥CYCH
	//MrowkaBase(const MrowkaBase&) = delete;
	//MrowkaBase& operator=(const MrowkaBase&) = delete;
	//MrowkaBase(MrowkaBase&&) = default;
	//MrowkaBase& operator=(MrowkaBase&&) = default;
};

class MrowkaKlikaV1 : public MrowkaBase<MrowkaKlikaV1, boost::random::minstd_rand>
{
private:
	std::unordered_set<GrafZFeromonami::vertex_descriptor> odwiedzone_;
public:
	MrowkaKlikaV1(GrafZFeromonami& graf, GrafZFeromonami::vertex_descriptor pozycja, random_number_generator ran) :
		MrowkaBase(graf, pozycja, ran)
	{
		
	}


	double ocen_wierzcholek(GrafZFeromonami::vertex_descriptor ten_wierzcholek, const std::vector<GrafZFeromonami::vertex_descriptor>& sasiedzi, GrafZFeromonami& graf)
	{
		assert(!sasiedzi.empty());

		double ile_juz_odwiedzonych = std::count_if(sasiedzi.begin(), sasiedzi.end(), [&](GrafZFeromonami::vertex_descriptor v)
		{
			return odwiedzone_.find(v) != odwiedzone_.end();
		});
		double ilosc_sasiadow = sasiedzi.size();
		odwiedzone_.insert(ten_wierzcholek);
		return ile_juz_odwiedzonych / ilosc_sasiadow;
	}
};

class MrowkaKlikaV2 : public MrowkaBase<MrowkaKlikaV2, boost::random::minstd_rand>
{
private:
	std::deque<GrafZFeromonami::vertex_descriptor> odwiedzone_;
public:
	MrowkaKlikaV2(GrafZFeromonami& graf, GrafZFeromonami::vertex_descriptor pozycja, random_number_generator ran) :
		MrowkaBase(graf, pozycja, ran)
	{

	}


	double ocen_wierzcholek(GrafZFeromonami::vertex_descriptor ten_wierzcholek, const std::vector<GrafZFeromonami::vertex_descriptor>& sasiedzi, GrafZFeromonami& graf)
	{
		assert(!sasiedzi.empty());

		double ile_juz_odwiedzonych = std::count_if(sasiedzi.begin(), sasiedzi.end(), [&](GrafZFeromonami::vertex_descriptor v)
		{
			return std::find(odwiedzone_.begin(), odwiedzone_.end(), v) != odwiedzone_.end();
		});
		double ilosc_sasiadow = sasiedzi.size();
		odwiedzone_.push_back(ten_wierzcholek);
		if(odwiedzone_.size() > 35)
			odwiedzone_.pop_front();
		return ile_juz_odwiedzonych / ilosc_sasiadow;
	}
};

void mrowki(GrafZFeromonami& graf, const int ilosc_watkow = std::thread::hardware_concurrency())
{
	std::random_device rd;
	boost::random::mt19937 mt(rd());
	typedef MrowkaKlikaV2 Mrowka;
	static const int ilosc_ruchow = 10000;

	std::vector<std::vector<Mrowka>> mrowiska(ilosc_watkow);
	for(int i = 0; i < ilosc_watkow; ++i)
		for(int j = 0; j < 1000; ++j)
			mrowiska[i].push_back(Mrowka(graf, boost::random_vertex(graf, mt), Mrowka::random_number_generator(rd())));
	std::vector<std::future<void>> wyniki(ilosc_watkow);
	for(int i = 0; i < ilosc_watkow; ++i)
	{
		wyniki[i] = std::async(std::launch::async, [](std::vector<Mrowka>& mrowisko)
		{
			for(int i = 0; i < ilosc_ruchow; ++i)
				for(auto& mrowka : mrowisko)
					mrowka.nastepny_ruch();
		}, std::ref(mrowiska[i]));
	}

	for(auto& wynik : wyniki)
		wynik.wait();

	auto vertices = boost::vertices(graf);
	std::for_each(vertices.first, vertices.second, [&](GrafZFeromonami::vertex_descriptor v)
	{
		double x = graf[v].feromony.load();
		x = obecny_poziom_feromonu(x, ilosc_ruchow);
		graf[v].feromony.store(x);
	});
}

void kliki_klasycznie(GrafZFeromonami& graf);

template<typename PodzbiorGrafuIterator>
void serializuj_do_dot(std::ostream& os, const GrafZFeromonami& graf, PodzbiorGrafuIterator b, PodzbiorGrafuIterator e)
{
	std::vector<typename PodzbiorGrafuIterator::value_type> vert_set(b, e);
	std::sort(vert_set.begin(), vert_set.end());
	os << "strict graph {\n";
	std::for_each(b, e, [&](GrafZFeromonami::vertex_descriptor v)
	{
		os << v << " [ fer = " << std::fixed << graf[v].feromony.load() << ";]" << "\n";
	});

	os << "\n\n";

	auto edge_set = boost::edges(graf);
	std::for_each(edge_set.first, edge_set.second, [&](GrafZFeromonami::edge_descriptor e)
	{
		if(std::binary_search(vert_set.begin(), vert_set.end(), boost::source(e, graf)) &&
		   std::binary_search(vert_set.begin(), vert_set.end(), boost::target(e, graf)))
			os << boost::source(e, graf) << " -- " << boost::target(e, graf) << "\n";
	});
	os << "}";
}

void serializuj_do_dot(std::ostream& os, const GrafZFeromonami& graf)
{
	auto vert_set = boost::vertices(graf);
	serializuj_do_dot(os, graf, vert_set.first, vert_set.second);
}

template<typename MutableGraph, typename RandomNumberGenerator>
void wygeneruj_graf_z_klika(MutableGraph& graf, int ilosc_wierzcholkow, int ilosc_krawedzi_hint, int ilosc_wierzcholkow_w_klice, RandomNumberGenerator& rng)
{
	boost::generate_random_graph(graf, ilosc_wierzcholkow, ilosc_krawedzi_hint, rng);
	std::unordered_set<typename boost::graph_traits<MutableGraph>::vertex_descriptor> klika;
	while(klika.size() < ilosc_wierzcholkow_w_klice)
	{
		klika.insert(boost::random_vertex(graf, rng));
	}
	for(auto& left : klika)
	{
		boost::remove_out_edge_if(left, [&](typename boost::graph_traits<MutableGraph>::edge_descriptor edge)
		{
			return klika.find(boost::target(edge, graf)) != klika.end();
		}, graf);
		for(auto& right : klika)
		{
			if(left != right)
			{
				boost::add_edge(left, right, graf);
			}
		}
	}
}

void test()
{
	GrafZFeromonami graf;
	std::random_device rd;
	boost::random::mt19937 mt(rd());

	wygeneruj_graf_z_klika(graf, 100, 150, 30, mt);
	//boost::generate_random_graph(graf, 50, 150, mt);
	mrowki(graf);

	auto vertices = boost::vertices(graf);
	std::vector<GrafZFeromonami::vertex_descriptor> sorted_vertices(vertices.first, vertices.second);
	std::sort(sorted_vertices.begin(), sorted_vertices.end(), [&](GrafZFeromonami::vertex_descriptor lhs, GrafZFeromonami::vertex_descriptor rhs)
	{
		return graf[lhs].feromony.load() > graf[rhs].feromony.load();
	});
	serializuj_do_dot(std::cout, graf, sorted_vertices.begin(), sorted_vertices.end());
	std::cout << "\n\n";

	kliki_klasycznie(graf);
	std::cout << "\n\n";
}

#include <boost/graph/bron_kerbosch_all_cliques.hpp>

struct kliki_o_rozmiarze
{
	kliki_o_rozmiarze(std::size_t rozmiar)
		: rozmiar(rozmiar)
	{ }

	template <typename Clique, typename Graph>
	inline void clique(const Clique& p, const Graph& g)
	{
		if(p.size() == rozmiar)
		{
			std::vector<GrafZFeromonami::vertex_descriptor> sorted_vertices(p.begin(), p.end());
			std::sort(sorted_vertices.begin(), sorted_vertices.end(), [&](GrafZFeromonami::vertex_descriptor lhs, GrafZFeromonami::vertex_descriptor rhs)
			{
				return g[lhs].feromony.load() > g[rhs].feromony.load();
			});
			serializuj_do_dot(std::cout, g, sorted_vertices.begin(), sorted_vertices.end());
		}
	}
	std::size_t rozmiar;
};

void kliki_klasycznie(GrafZFeromonami& graf)
{
	std::size_t size = 0;
	boost::bron_kerbosch_all_cliques(graf, boost::find_max_clique(size));
	boost::bron_kerbosch_all_cliques(graf, kliki_o_rozmiarze(size));
}