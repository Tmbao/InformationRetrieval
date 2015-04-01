#pragma comment(linker, "/STACK:1073741824")

#include "configure.h"
#include "index.h"
#include "stop_words.h"
#include "document_initialize.h"
#include "query_building.h"
#include "BSBI_index_construction.h"
#include "SPIMI_index_construction.h"

using namespace ohsu_trec;

BSBI bsbi;
SPIMI spimi;

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

vector < pair <double, int> > query(vector <string> terms) {

	memset(nume, 0, sizeof nume);
	memset(deno, 0, sizeof deno);

	// Load Query
	int freq[maximum_of_terms];
	memset(freq, 0, sizeof freq);

	for (string term : terms) {
		auto s_terms = split_tokens(lower_case(term));
		for (auto s_term: s_terms) {
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
	for (Index index; fread(&index, sizeof(Index), 1, file); ) {
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
		ret.push_back({nume[i], doc_uid[i]});
	sort(ret.rbegin(), ret.rend());

	//return feedback_rerank(ret, q_tfidf, freq);
	return local_analysis_rerank(ret, q_tfidf, freq);
	//return global_analysis_rerank(ret, q_tfidf, freq);
	//return ret;
} 

void proc_all_queries(string query_path, string groundtruth_path) {
	int number_of_queries = 0;
	map <string, int> query_id;
	vector < set <int> > relevants;

	FILE *file_query, *file_groundtruth;
	file_query = fopen(query_path.c_str(), "r");
	file_groundtruth = fopen(groundtruth_path.c_str(), "r");

	double mAP = 0;
	char token[8192];

	while (fscanf(file_groundtruth, "%s", token) != EOF) {
		if (query_id.count(token) == 0) {
			query_id[token] = number_of_queries++;
			relevants.push_back(set <int>());
		}
		int id = query_id[token];
		int u_id;
		fscanf(file_groundtruth, "%d", &u_id);
		relevants[id].insert(u_id);
		fgets(token, 8192, file_groundtruth);
	}

	for (int i = 0; fscanf(file_query, "%s", token) != EOF; i++) {
		cerr << "    Processing #" << i << " query" << endl;

		vector <string> terms;
		string query_name;

		// Read number
		fscanf(file_query, "%s", token);
		fscanf(file_query, "%s", token);
		fscanf(file_query, "%s", token);
		query_name = token;

		// Read title
		fscanf(file_query, "%s", token);
		fgets(token, 8192, file_query);
		auto tokens = split_tokens(token);
		for (int j = 0; j < tokens.size(); j++)
			terms.push_back(tokens[j]);

		// Read desc
		fscanf(file_query, "%s", token);
		fgets(token, 8192, file_query);
		fgets(token, 8192, file_query);
		tokens = split_tokens(token);
		for (int j = 0; j < tokens.size(); j++)
			terms.push_back(tokens[j]);

		// Read close tag
		fscanf(file_query, "%s", token);

		// Evaluate
		auto rank_list = query(terms);

		double AP = 0;

		string name_PR, name_F;
		name_PR = "Data\\result_PR\\" + query_name;
		name_F = "Data\\result_F\\" + query_name;

		FILE *file_PR, *file_F;
		file_PR = fopen(name_PR.c_str(), "w");
		file_F = fopen(name_F.c_str(), "w");

		int n_correct = 0, total_correct = relevants[i].size(), n_predict = 0;
		for (int j = 0; j < rank_list.size() && n_correct < total_correct; j++) {
			n_predict++;
			if (relevants[i].find(rank_list[j].second) != relevants[i].end()) {
				n_correct++;

				double recall = (double)n_correct / total_correct;
				double precision = (double)n_correct / n_predict;
				double f_measure = 2 * precision * recall / (precision + recall);

				AP = (AP * (n_correct - 1) + precision) / n_correct;

				fprintf(file_PR, "%.4lf %.4lf\n", recall, precision);
				fprintf(file_F, "%d %.4lf\n", n_predict, f_measure);
			}
		}

		fclose(file_PR);
		fclose(file_F);

		cerr << fixed << "    Average Precision: " << AP << endl;

		mAP += AP / number_of_queries;

	}

	fclose(file_query);
	fclose(file_groundtruth);

	FILE *file = fopen("Data\\mAP.txt", "w");
	fprintf(file, "%.4lf\n", mAP);
	fclose(file);
}

int main() {

	long long startTime = clock();

	cerr << "Initializing Stop words";
	init_stop_words();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Parsing RAW file";
	//parse_to_RAW_file({"Data\\t9.filtering\\ohsu-trec\\trec9-train\\ohsumed.87"}, "Data\\RAW.bin");
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Saving dictionary file";
	//init_dictionary_file();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Loading dictionary file";
	load_dictionary_file();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Building inverted index";
	//bsbi.index_construction();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Loading terms' offset";
	load_index_offset();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Pre-calculating term idf";
	calc_term_idf();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Answer oshu queries\n";
	//Oshu
	proc_all_queries("Data\\t9.filtering\\ohsu-trec\\trec9-train\\query.ohsu.1-63", 
					 "Data\\t9.filtering\\ohsu-trec\\trec9-train\\qrels.ohsu.batch.87");
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	return 0;

	cerr << "Answer mesh queries\n";
	//MeSH
	/*proc_all_queries("Data\\t9.filtering\\ohsu-trec\\trec9-train\\query.mesh.1-4904",
					 "Data\\t9.filtering\\ohsu-trec\\trec9-train\\qrels.mesh.batch.87");*/
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Done\nPlease use matlab/octave to run plot_precision_recall and plot_f_measure" << endl;

	return 0;

}
