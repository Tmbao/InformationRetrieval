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
vector < pair <double, int> > query(vector<string> terms) {

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
		if (freq > 0) tf = 1 + log(index.freq);

		nume[doc_id[index.doc_id]] += tf * idf[index.term_id] * q_tfidf[index.term_id];
		deno[doc_id[index.doc_id]] += sqr(tf * idf[index.term_id]);
	}

	fclose(file);

	for (int i = 0; i < number_of_documents; i++)
		deno[i] = sqrt(deno[i]);

	vector < pair <double, int> > ret;
	for (int i = 0; i < number_of_documents; i++) 
		ret.push_back({nume[i] / deno[i], doc_uid[i]});
	sort(ret.rbegin(), ret.rend());

	return ret;
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
		cerr << "	Processing #" << i << " query" << endl;

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

		mAP += AP / number_of_queries;

	}

	fclose(file_query);
	fclose(file_groundtruth);

	FILE *file = fopen("Data\\mAP.txt", "w");
	fprintf(file, "%.4lf\n", mAP);
	fclose(file);
}

int main() {

	cerr << "Initializing Stop words" << endl;
	//init_stop_words();

	cerr << "Parsing RAW file" << endl;
	parse_to_RAW_file({"Data\\t9.filtering\\ohsu-trec\\trec9-train\\ohsumed.87"}, "Data\\RAW.bin");

	cerr << "Saving dictionary file" << endl;
	init_dictionary_file();

	cerr << "Loading dictionary file" << endl;
	load_dictionary_file();

	cerr << "Building inverted index" << endl;
	bsbi.index_construction();

	cerr << "Loading terms' offset" << endl;
	load_index_offset();

	cerr << "Pre-calculating term idf" << endl;
	calc_term_idf();

	cerr << "Answer oshu queries" << endl;
	//Oshu
	proc_all_queries("Data\\t9.filtering\\ohsu-trec\\trec9-train\\query.ohsu.1-63", 
					 "Data\\t9.filtering\\ohsu-trec\\trec9-train\\qrels.ohsu.batch.87");

	cerr << "Answer mesh queries" << endl;
	//MeSH
	proc_all_queries("Data\\t9.filtering\\ohsu-trec\\trec9-train\\query.mesh.1-4904",
					 "Data\\t9.filtering\\ohsu-trec\\trec9-train\\qrels.mesh.batch.87");

	cerr << "Done\nPlease use matlab/octave to run plot_precision_recall and plot_f_measure" << endl;

	return 0;

}