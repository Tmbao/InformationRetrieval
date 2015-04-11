#include "configure.h"
#include "document.h"
#include "query.h"

using namespace reuters21578;

namespace HAC {

	struct Tedge {
		double similarity;
		int i, j;
		Tedge(double _similarity = 0, int _i = 0, int _j = 0) : similarity(_similarity), i(_i), j(_j) {}

		friend bool operator < (const Tedge &a, const Tedge &b) {
			if (abs(a.similarity - b.similarity) < eps)
				return make_pair(a.i, a.j) < make_pair(b.i, b.j);
			else
				return a.similarity < b.similarity;
		}
	};
	
	set<int> list;
	vector<int> count;
	vector<int> dsu;

	int DSU(int u) {
		return dsu[u] == u ? u : dsu[u] = DSU(dsu[u]);
	}

	vector<vector<double>> tfidf;

	void normalize(vector<double> &vec) {
		double sum_sqr = 0;
		for (int i = 0; i < vec.size(); i++)
			sum_sqr += vec[i] * vec[i];
		for (int i = 0; i < vec.size(); i++)
			vec[i] /= sum_sqr;
	}

	void initialize() {
		count.resize(number_of_documents);
		dsu.resize(number_of_documents);
		for (int i = 0; i < number_of_documents; i++) {
			count[i] = 1;
			dsu[i] = i;
			list.insert(i);
		}

		tfidf.resize(number_of_documents, vector<double>(number_of_terms, 0));

		FILE *file = fopen(INDEX_path, "rb");

		for (Index index; fread(&index, sizeof(Index), 1, file);) {
			double tf = 0;
			if (index.freq > 0) tf = 1 + log(index.freq);

			int d_id = doc_id[index.doc_id];
			tfidf[d_id][index.term_id] = tf * idf[index.term_id];
		}

		fclose(file);

		for (int i = 0; i < number_of_documents; i++)
			normalize(tfidf[i]);
	}

	set<Tedge> edge;

	double similarity(int i, int j, bool debug = false) {
		double nume = 0;
		double deno = 0;
		for (int k = 0; k < number_of_terms; k++)
			nume += (tfidf[i][k] + tfidf[j][k]) * (tfidf[i][k] + tfidf[j][k]);
		nume -= count[i] + count[j];
		deno = (count[i] + count[j]) * (count[i] + count[j] - 1.0);
		return nume / deno;
	}

	void remove(int i, int j) {
		if (i > j) swap(i, j);
		auto it = edge.lower_bound(Tedge(similarity(i, j) - eps, i, j));
		if (it != edge.end() && it->i == i && it->j == j)
			edge.erase(it);
	}

	void remove(int i) {
		for (int j : list) 
			remove(i, j);
	}

	void add(int i) {
		for (int j : list) {
			if (i >= j) continue;
			edge.insert(Tedge(similarity(i, j), i, j));
		}
	}

	void join(int i, int j) {
		i = DSU(i);
		j = DSU(j);
		dsu[j] = i;

		count[i] += count[j];
		for (int k = 0; k < number_of_terms; k++)
			tfidf[i][k] += tfidf[j][k];
		normalize(tfidf[i]);

		remove(j);
		remove(i);

		tfidf[j].clear();
		list.erase(j);

		add(i);
	}

	vector<int> cluster(int n_cluster = 20) {
		cerr << "	Initializing terms ..." << endl;
		initialize();

		edge.clear();
		cerr << "	Initializing similarity ..." << endl;

		vector <int> _list;
		for (auto it = list.begin(); it != list.end(); it++)
			_list.push_back(*it);
		for (int i : _list) {
			cerr << "		Intializing #" << i << endl;
			add(i);
		}

		while (list.size() > n_cluster) {
			cerr << "	Processing ... " << list.size() << " cluster(s) remaining" << endl;
			Tedge top = *edge.rbegin();
			join(top.i, top.j);
		}

		vector<int> ret;
		map<int, int> id;
		for (int i = 0; i < number_of_documents; i++) {
			int x = DSU(i);
			if (id.count(x) == 0)
				id[x] = id.size();
			ret.push_back(id[x]);
		}
		return ret;
	}
};
