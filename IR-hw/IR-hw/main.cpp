#pragma comment(linker, "/STACK:268435456")

#include "configure.h"
#include "index.h"
#include "stop_words.h"
#include "document_initialize.h"
#include "query_building.h"
#include "BSBI_index_construction.h"
#include "SPIMI_index_construction.h"

BSBI bsbi;
SPIMI spimi;

vector <int> class_sizes;
vector <string> class_names;
vector <string> file_names;
map <int, int> class_of_file;
map <string, int> class_id;

int offset[maximum_of_terms];
double idf[maximum_of_terms];

vector <string> get_all_file_names(string directory) { 
	vector <string> res;
	DIR *dir = opendir(directory.c_str()); 
	while (dirent *pdir = readdir(dir)) { 
		string name = pdir->d_name; 
		if (name != "." && name != ".." && name != "desktop.ini") 
			res.push_back(pdir->d_name); 
	} 
	(void)closedir(dir);
	return res; 
}

void fetching_file_names() {
	file_names.clear();
	class_of_file.clear();

	class_names = get_all_file_names("Data\\20_newsgroups\\");
	for (int i = 0; i < class_names.size(); i++) {
		class_sizes.push_back(0);
		auto files = get_all_file_names("Data\\20_newsgroups\\" + class_names[i]); 
		class_id[class_names[i]] = i;
		for (auto file: files) {
			file_names.push_back("Data\\20_newsgroups\\" + class_names[i] + "/" + file);
			class_of_file[file_names.size() - 1] = i;

			class_sizes.back()++;
		}
	}	
}

void build_all_queries() {
	for (auto class_name: class_names) 
		build_query(class_name, "Data\\20_newsgroups\\" + class_name);
}

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
	int number_of_terms = dictionary.size();
	int number_of_documents = file_names.size();
	for (int i = 0; i < number_of_terms; i++)
		idf[i] = 1 + log(number_of_documents / double((offset[i + 1] - offset[i]) / sizeof(Index)));
}

vector < pair <double, int> > query(string file_query) {
	int number_of_documents = file_names.size();
	int number_of_terms = dictionary.size();

	double nume[maximum_of_terms], deno[maximum_of_terms], distance[maximum_of_terms];
	FILE *file;

	memset(nume, 0, sizeof nume);
	memset(deno, 0, sizeof deno);

	// Load Query

	int freq[maximum_of_terms];
	memset(freq, 0, sizeof freq);

	file = fopen(file_query.c_str(), "r");

	char term[1024];
	while (fscanf(file, "%s", term) != EOF) {
		auto s_terms = split_tokens(lower_case(term));
		for (auto s_term: s_terms) {
			if (s_term.length() == 0) continue;
			if (stop_words.find(s_term) == stop_words.end()) 
				if (dictionary.count(s_term)) 
					freq[dictionary[s_term]]++;
		}
	}	

	fclose(file);

	vector<double> q_tfidf(number_of_terms, 0);

	for (int i = 0; i < number_of_terms; i++) {
		double tf = 0;
		if (freq[i] > 0) tf = 1 + log(freq[i]);

		q_tfidf[i] = tf * idf[i];
	}

	// Load Docs

	file = fopen(INDEX_path, "rb");

	for (Index index; fread(&index, sizeof(Index), 1, file); ) {
		double tf = 0;
		if (freq > 0) tf = 1 + log(index.freq);

		nume[index.doc_id] += tf * idf[index.term_id] * q_tfidf[index.term_id];
		deno[index.doc_id] += sqr(tf * idf[index.term_id]);
	}

	fclose(file);

	for (int i = 0; i < number_of_documents; i++)
		deno[i] = sqrt(deno[i]);

	vector < pair <double, int> > ret;

	for (int i = 0; i < number_of_documents; i++) 
		ret.push_back({nume[i] / deno[i], i});

	sort(ret.rbegin(), ret.rend());

	return ret;
} 

void proc_all_queries() {
	DIR *dir = opendir("Data\\query"); 

	double mAP = 0;
	int n_query = 0;

	while (dirent *pdir = readdir(dir)) { 
		string name = pdir->d_name; 

		if (name != "." && name != ".." && name != "desktop.ini") {
			int id = class_id[name];
			auto ret = query("Data\\query\\" + name);

			double AP = 0;

			string name_PR, name_F;
			name_PR = "Data\\result_PR\\" + name;
			name_F = "Data\\result_F\\" + name;

			FILE *file_PR, *file_F;
			file_PR = fopen(name_PR.c_str(), "w");
			file_F = fopen(name_F.c_str(), "w");

			int n_correct = 0, total_correct = class_sizes[id], n_predict = 0;
			for (int i = 0; i < ret.size(); i++) {
				n_predict++;
				if (class_of_file[ret[i].second] == id) {
					n_correct++;

					double recall = double(n_correct) / total_correct;
					double precision = double(n_correct) / n_predict;
					double f_measure = 2 * precision * recall / (precision + recall);

					AP = (AP * (n_correct - 1) + precision) / n_correct;

					fprintf(file_PR, "%.4lf %.4lf\n", recall, precision);
					fprintf(file_F, "%d %.4lf\n", n_predict, f_measure);
				}
			}

			fclose(file_PR);
			fclose(file_F);

			mAP += AP;
			n_query++;
		}
	} 

	mAP /= n_query;

	freopen("Data\mAP.txt", "w", stdout);
	printf("%.4lf\n", mAP);
	fclose(stdout);
}

int main() {

	cerr << "Fetching file names" << endl;
	fetching_file_names();

	cerr << "Initializing Stop words" << endl;
	init_stop_words();

	cerr << "Parsing RAW file" << endl;
	parse_to_RAW_file(file_names);

	cerr << "Saving dictionary file" << endl;
	init_dictionary_file();

	cerr << "Loading dictionary file" << endl;
	load_dictionary_file();

	cerr << "Building inverted index" << endl;
	bsbi.index_construction();

	cerr << "Building queries" << endl;
	build_all_queries();

	cerr << "Loading terms' offset" << endl;
	load_index_offset();

	cerr << "Pre-calculating term idf" << endl;
	calc_term_idf();

	cerr << "Answer all queries" << endl;
	proc_all_queries();

	cerr << "Done\nPlease use matlab/octave to run plot_precision_recall and plot_f_measure" << endl;

	return 0;

}
