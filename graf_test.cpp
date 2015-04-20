#include <iostream>
#include <iomanip>
#include <atomic>
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

const double predkosc_wyparowania_feromonu = 0.1;

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
	double stopien_wyparowania_feromonu_;

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
		ran_(ran)
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
			wartosci_feromonow.push_back(graf[docelowy].feromony.load() - stopien_wyparowania_feromonu_);
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
		stopien_wyparowania_feromonu_ += predkosc_wyparowania_feromonu;
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

class MrowkaKlika : public MrowkaBase<MrowkaKlika, boost::random::minstd_rand>
{
private:
	std::unordered_set<GrafZFeromonami::vertex_descriptor> odwiedzone_;
public:
	MrowkaKlika(GrafZFeromonami& graf, GrafZFeromonami::vertex_descriptor pozycja, random_number_generator ran) :
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

void mrowki(GrafZFeromonami& graf, const int ilosc_watkow = std::thread::hardware_concurrency())
{
	std::random_device rd;
	boost::random::mt19937 mt(rd());

	std::vector<std::vector<MrowkaKlika>> mrowiska(ilosc_watkow);
	for(int i = 0; i < ilosc_watkow; ++i)
		for(int j = 0; j < 1000; ++j)
			mrowiska[i].push_back(MrowkaKlika(graf, boost::random_vertex(graf, mt), MrowkaKlika::random_number_generator(rd())));
	std::vector<std::future<void>> wyniki(ilosc_watkow);
	for(int i = 0; i < ilosc_watkow; ++i)
	{
		wyniki[i] = std::async(std::launch::async, [](std::vector<MrowkaKlika>& mrowisko)
		{
			for(int i = 0; i < 10000; ++i)
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
		x -= 10000 * predkosc_wyparowania_feromonu;
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

void test()
{
	GrafZFeromonami graf;
	std::random_device rd;
	boost::random::mt19937 mt(rd());

	boost::generate_random_graph(graf, 50, 150, mt);
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
			serializuj_do_dot(std::cout, g, p.begin(), p.end());
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