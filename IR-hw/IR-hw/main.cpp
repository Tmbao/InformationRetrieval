//#pragma comment(linker, "/STACK:1073741824")

#include "configure.h"
#include "index.h"
#include "stop_words.h"
#include "document.h"
#include "query_building.h"
#include "BSBI_index_construction.h"
#include "SPIMI_index_construction.h"
#include "query.h"
#include "category.h"
#include "classifier.h"
#include "fetching_files.h"
#include "clustering.h"

using namespace reuters21578;

BSBI bsbi;
SPIMI spimi;

void proc_all_queries(string query_path, string groundtruth_path) {
	int number_of_queries = 0;
	map <string, int> query_id;
	vector< set <int>> relevants;

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

		vector<string> terms;
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

void verify_list_files(vector<string> files, vector<string> queries) {
	category cat(CATEGORY_path);

	for (int i = 0; i < files.size(); i++) {
		auto cate = cat[i];
		if (files[i].find(string(TWENTYGRPS_directory) + "\\" + cate) == -1) {
			cout << "FAIL" << endl;
			return;
		}
	}
	for (int i = 0; i < queries.size(); i++) {
		auto cate = cat[-1 - i];
		if (queries[i].find(string(TWENTYGRPS_directory) + "\\" + cate) == -1) {
			cout << "FAIL" << endl;
			return;
		}
	}
}

int main() {

	long long startTime = clock();

	cerr << "Initializing Stop words";
	init_stop_words();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Parsing list files";
	//init_list_files();
	auto files = load_list_files();
	//auto queries = load_queries();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Parsing RAW file";
	parse_to_RAW_file(files, RAW_path);
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Saving dictionary file";
	init_dictionary_file();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Loading dictionary file";
	load_dictionary_file();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Building inverted index";
	bsbi.index_construction();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Loading terms' offset";
	load_index_offset();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Pre-calculating term idf";
	calc_term_idf();
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	cerr << "Verifying list files";
	//verify_list_files(files, queries);
	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	auto ret = HAC::cluster(100);

	freopen("Data\\Log.txt", "r", stdout);
	
	for (int i = 0; i < ret.size(); i++)
		cout << i << " " << ret[i] << endl;

	// category cat(CATEGORY_path);
	// cerr << "Classifying" << endl;
	// double AP = 0;
	// for (int i = 0; i < queries.size(); i++) {
	// 	string predict = naive_bayes::classify(read_file(queries[i]));
	// 	bool result = predict == cat[-1 - i];
	// 	cout << "	" << queries[i] << " " << predict << " " << (result ? "TRUE" : "FALSE");
	// 	cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	// 	if (result)
	// 		AP += 1.0 / queries.size();
	// }
	// cout << fixed << AP << endl;

	//cerr << "Answer oshu queries\n";
	////Oshu
	//proc_all_queries("Data\\t9.filtering\\ohsu-trec\\trec9-train\\query.ohsu.1-63", 
	//				 "Data\\t9.filtering\\ohsu-trec\\trec9-train\\qrels.ohsu.batch.87");
	//cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	return 0;

	//cerr << "Answer mesh queries\n";
	////MeSH
	///*proc_all_queries("Data\\t9.filtering\\ohsu-trec\\trec9-train\\query.mesh.1-4904",
	//				 "Data\\t9.filtering\\ohsu-trec\\trec9-train\\qrels.mesh.batch.87");*/
	//cerr << " - Done: " << double(clock() - startTime) / CLOCKS_PER_SEC << "s\n";

	//cerr << "Done\nPlease use matlab/octave to run plot_precision_recall and plot_f_measure" << endl;

	return 0;
}
