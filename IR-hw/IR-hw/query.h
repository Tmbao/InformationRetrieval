#pragma once

#include "configure.h"
#include "index.h"
#include "document_initialize.h"
using namespace twenty_newsgroups;

int offset[maximum_of_terms];
double idf[maximum_of_terms];

void load_index_offset() {
	int number_of_terms = dictionary.size();
	memset(offset, -1, sizeof offset);

	FILE *index_file = fopen(INDEX_path, "rb");
	while (true) {
		int fposition = ftell(index_file);

		Index index;
		if (!fread(&index, sizeof(Index), 1, index_file))
			break;
		if (offset[index.term_id] == -1)
			offset[index.term_id] = fposition;
		number_of_terms = index.term_id + 1;
	}
	offset[number_of_terms] = ftell(index_file);

	fclose(index_file);
}

void calc_term_idf() {
	for (int i = 0; i < number_of_terms; i++)
		idf[i] = 1 + log(number_of_documents / double((offset[i + 1] - offset[i]) / sizeof(Index)));
}

double nume[maximum_of_terms], deno[maximum_of_terms], distance[maximum_of_terms];

vector < pair <double, int> > feedback_rerank(vector < pair <double, int> > rank_list, vector <double> q_tfidf, int q_freq[maximum_of_terms], int n_docs = 5, double alpha = 0.6, double beta = 0.1, double gamma = 0.1) {
	vector <int> rank(rank_list.size());

	for (int i = 0; i < rank_list.size(); i++)
		rank[doc_id[rank_list[i].second]] = i;

	memset(nume, 0, sizeof nume);
	memset(deno, 0, sizeof deno);

	vector <double> delta_tfidf(q_tfidf.size());

	// Re-calculate query tfidf
	FILE *file = fopen(INDEX_path, "rb");
	for (Index index; fread(&index, sizeof(Index), 1, file);) {
		double tf = 0;
		if (index.freq > 0) tf = 1 + log(index.freq);

		if (rank[doc_id[index.doc_id]] < n_docs)
			delta_tfidf[index.term_id] += beta * tf * idf[index.term_id];
		else if (rank[doc_id[index.doc_id]] + 1 == rank_list.size())
			delta_tfidf[index.term_id] -= gamma * tf * idf[index.term_id];
	}
	for (int i = 0; i < q_tfidf.size(); i++)
		q_tfidf[i] = alpha * q_tfidf[i] + delta_tfidf[i];

	fseek(file, 0, SEEK_SET);
	for (Index index; fread(&index, sizeof(Index), 1, file);) {
		double tf = 0;
		if (index.freq > 0) tf = 1 + log(index.freq);

		nume[doc_id[index.doc_id]] += tf * idf[index.term_id] * q_tfidf[index.term_id];
		deno[doc_id[index.doc_id]] += sqr(tf * idf[index.term_id]);
	}

	fclose(file);

	for (int i = 0; i < number_of_documents; i++)
		deno[i] = sqrt(deno[i]);

	vector < pair <double, int> > ret;
	for (int i = 0; i < number_of_documents; i++)
		ret.push_back({ nume[i], doc_uid[i] });
	sort(ret.rbegin(), ret.rend());

	return ret;
}

vector < pair <double, int> > local_analysis_rerank(vector < pair <double, int> > rank_list, vector <double> q_tfidf, int q_freq[maximum_of_terms], int n_docs = 5, int terms_p_doc = 100) {
	map <int, int> local_id;
	vector <int> original_id;
	vector <int> rank(rank_list.size());

	for (int i = 0; i < rank_list.size(); i++)
		rank[doc_id[rank_list[i].second]] = i;

	vector < vector < pair <double, int> > > freq(rank_list.size());

	// Calculate frequency
	FILE *file = fopen(INDEX_path, "rb");
	for (Index index; fread(&index, sizeof(Index), 1, file);) {
		if (rank[doc_id[index.doc_id]] < n_docs) {
			freq[doc_id[index.doc_id]].push_back({ index.freq, index.term_id });
		}
	}

	// Get the top frequency
	for (int i = 0; i < freq.size(); i++) {
		sort(freq[i].rbegin(), freq[i].rend());
		while (freq[i].size() > terms_p_doc)
			freq[i].pop_back();

		for (int j = 0; j < freq[i].size(); j++) {
			if (local_id.count(freq[i][j].second) == 0) {
				local_id[freq[i][j].second] = local_id.size();
				original_id.push_back(freq[i][j].second);
			}
		}
	}

	// Calculate association matrix
	int n_terms = original_id.size();
	vector < vector <double> > c(n_terms), s(n_terms);
	for (int i = 0; i < n_terms; i++) {
		c[i] = vector <double>(n_terms, 0);
		s[i] = vector <double>(n_terms, 0);
	}
	for (int i = 0; i < freq.size(); i++) {
		for (int j = 0; j < freq[i].size(); j++) {
			for (int k = j; k < freq[i].size(); k++)
				c[local_id[freq[i][j].second]][local_id[freq[i][k].second]] += freq[i][j].first * freq[i][k].first;
		}
	}
	for (int i = 0; i < n_terms; i++) {
		for (int j = 0; j < n_terms; j++) {
			if (abs(c[i][i] + c[j][j] - c[j][i]) < eps)
				s[i][j] = 0;
			else
				s[i][j] = c[i][j] / (c[i][i] + c[j][j] - c[j][i]);
		}

		if (q_freq[original_id[i]] > 0) {
			int chosen = i;
			for (int j = 0; j < n_terms; j++) {
				if ((s[i][j] > 0) && (q_freq[original_id[chosen]] != 0 || (q_freq[original_id[j]] == 0 && s[i][j] > s[i][chosen])))
					chosen = j;
			}

			if (chosen != i && q_freq[original_id[chosen]] == 0)
				q_tfidf[original_id[chosen]] += idf[original_id[chosen]];
		}
	}

	// Re-ranking
	fseek(file, 0, SEEK_SET);
	for (Index index; fread(&index, sizeof(Index), 1, file);) {
		double tf = 0;
		if (index.freq > 0) tf = 1 + log(index.freq);

		nume[doc_id[index.doc_id]] += tf * idf[index.term_id] * q_tfidf[index.term_id];
		deno[doc_id[index.doc_id]] += sqr(tf * idf[index.term_id]);
	}

	fclose(file);

	for (int i = 0; i < number_of_documents; i++)
		deno[i] = sqrt(deno[i]);

	vector < pair <double, int> > ret;
	for (int i = 0; i < number_of_documents; i++)
		ret.push_back({ nume[i], doc_uid[i] });
	sort(ret.rbegin(), ret.rend());

	return ret;
}

bool is_intialize_global_analysis = false;
map <int, int> gl_local_id;
vector <int> gl_original_id;
vector < vector <double> > gl_s;
void global_analysis_initialize(int n_docs = 100, int terms_p_doc = 10) {

	srand(time(NULL));

	vector <bool> chosen_docs(number_of_documents, false);
	for (int i = 0; i <= n_docs; i++)
		chosen_docs[rand() % chosen_docs.size()] = 1;

	vector < vector < pair <double, int> > > freq(number_of_documents);

	// Calculate frequency
	FILE *file = fopen(INDEX_path, "rb");
	for (Index index; fread(&index, sizeof(Index), 1, file);) {
		if (chosen_docs[doc_id[index.doc_id]])
			freq[doc_id[index.doc_id]].push_back({ index.freq, index.term_id });
	}
	fclose(file);

	// Get the top frequency
	for (int i = 0; i < freq.size(); i++) {
		sort(freq[i].rbegin(), freq[i].rend());
		while (freq[i].size() > terms_p_doc)
			freq[i].pop_back();

		for (int j = 0; j < freq[i].size(); j++) {
			if (gl_local_id.count(freq[i][j].second) == 0) {
				gl_local_id[freq[i][j].second] = gl_local_id.size();
				gl_original_id.push_back(freq[i][j].second);
			}
		}
	}

	// Calculate association matrix
	int n_terms = gl_original_id.size();
	vector < vector <double> > c(n_terms);
	gl_s = vector < vector<double> >(n_terms);
	for (int i = 0; i < n_terms; i++) {
		c[i] = vector <double>(n_terms, 0);
		gl_s[i] = vector <double>(n_terms, 0);
	}
	for (int i = 0; i < freq.size(); i++) {
		for (int j = 0; j < freq[i].size(); j++) {
			for (int k = j; k < freq[i].size(); k++)
				c[gl_local_id[freq[i][j].second]][gl_local_id[freq[i][k].second]] += freq[i][j].first * freq[i][k].first;
		}
	}
	for (int i = 0; i < n_terms; i++) {
		for (int j = 0; j < n_terms; j++) {
			if (abs(c[i][i] + c[j][j] - c[j][i]) < eps)
				gl_s[i][j] = 0;
			else
				gl_s[i][j] = c[i][j] / (c[i][i] + c[j][j] - c[j][i]);
		}
	}
}

vector < pair <double, int> > global_analysis_rerank(vector < pair <double, int> > rank_list, vector <double> q_tfidf, int q_freq[maximum_of_terms], int n_docs = 5, int terms_p_doc = 100) {

	if (!is_intialize_global_analysis) {
		global_analysis_initialize();
		is_intialize_global_analysis = true;
	}

	for (int i = 0; i < gl_s.size(); i++) {
		if (q_freq[gl_original_id[i]] > 0) {
			int chosen = i;
			for (int j = 0; j < gl_s[i].size(); j++) {
				if ((gl_s[i][j] > 0) && (q_freq[gl_original_id[chosen]] != 0 || (q_freq[gl_original_id[j]] == 0 && gl_s[i][j] > gl_s[i][chosen])))
					chosen = j;
			}

			if (chosen != i && q_freq[gl_original_id[chosen]] == 0)
				q_tfidf[gl_original_id[chosen]] += idf[gl_original_id[chosen]];
		}
	}

	// Re-ranking
	FILE *file = fopen(INDEX_path, "rb");
	for (Index index; fread(&index, sizeof(Index), 1, file);) {
		double tf = 0;
		if (index.freq > 0) tf = 1 + log(index.freq);

		nume[doc_id[index.doc_id]] += tf * idf[index.term_id] * q_tfidf[index.term_id];
		deno[doc_id[index.doc_id]] += sqr(tf * idf[index.term_id]);
	}

	fclose(file);

	for (int i = 0; i < number_of_documents; i++)
		deno[i] = sqrt(deno[i]);

	vector < pair <double, int> > ret;
	for (int i = 0; i < number_of_documents; i++)
		ret.push_back({ nume[i], doc_uid[i] });
	sort(ret.rbegin(), ret.rend());

	return ret;
}

vector < pair <double, int> > query(vector <string> terms,
									int rerank = 0 /* 0: No reranking, 1: Pseudo Feedback, 2: Local Analysis, 3: Global Analysis */) {

	memset(nume, 0, sizeof nume);
	memset(deno, 0, sizeof deno);

	// Load Query
	int freq[maximum_of_terms];
	memset(freq, 0, sizeof freq);

	for (string term : terms) {
		auto s_terms = split_tokens(lower_case(term));
		for (auto s_term : s_terms) {
			if (s_term.length() == 0) continue;
			if (stop_words.find(s_term) == stop_words.end())
			if (dictionary.count(s_term))
				freq[dictionary[s_term]]++;
		}
	}

	vector<double> q_tfidf(number_of_terms, 0);
	for (int i = 0; i < number_of_terms; i++) {
		double tf = 0;
		if (freq[i] > 0) tf = 1 + log(freq[i]);
		q_tfidf[i] = tf * idf[i];
	}

	// Load Docs
	FILE *file = fopen(INDEX_path, "rb");
	for (Index index; fread(&index, sizeof(Index), 1, file);) {
		double tf = 0;
		if (index.freq > 0) tf = 1 + log(index.freq);

		nume[doc_id[index.doc_id]] += tf * idf[index.term_id] * q_tfidf[index.term_id];
		deno[doc_id[index.doc_id]] += sqr(tf * idf[index.term_id]);
	}

	fclose(file);

	for (int i = 0; i < number_of_documents; i++)
		deno[i] = sqrt(deno[i]);

	vector < pair <double, int> > ret;
	for (int i = 0; i < number_of_documents; i++)
		ret.push_back({ nume[i], doc_uid[i] });
	sort(ret.rbegin(), ret.rend());

	switch (rerank) {
	case 0:
		return ret;
	case 1:
		return feedback_rerank(ret, q_tfidf, freq);
	case 2:
		return local_analysis_rerank(ret, q_tfidf, freq);
	case 3:
		return global_analysis_rerank(ret, q_tfidf, freq);
	}
}
