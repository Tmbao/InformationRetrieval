#include "configure.h"
#include "category.h"

namespace reuters21578 {

	vector<string> load_list_files() {
		vector<string> ret;

		DIR *pdir = opendir(REUTERS_directory);
		dirent *entry;
		while (entry = readdir(pdir)) {
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "desktop.ini") != 0) {
				string name = entry->d_name;
				if (name.find("reut2") != -1)
					ret.push_back(name);
			}
		}

		return ret;
	}

	void init_list_files() {
	}
};

namespace twenty_newsgroups {

	vector<string> get_list_directory() {
		vector<string> ret;

		DIR *pdir = opendir(TWENTYGRPS_directory);
		dirent *entry;
		while (entry = readdir(pdir)) {
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "desktop.ini") != 0)
				ret.push_back(entry->d_name);
		}
		closedir(pdir);
		return ret;
	}

	vector<string> get_list_files(vector<string> directories) {
		category cat;
		vector<string> ret;

		for (auto directory : directories) {
			string dir = string(TWENTYGRPS_directory) + "\\" + directory;
			DIR *pdir = opendir(dir.c_str());

			dirent *entry;
			while (entry = readdir(pdir)) {
				if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "desktop.ini") != 0) {
					cat[ret.size()] = directory;
					ret.push_back(dir + "\\" + string(entry->d_name));
				}
			}

			closedir(pdir);
		}

		cat.save(CATEGORY_path);

		return ret;
	}

	void shuffle_list_files(vector<string> &files, int n_shuffle = 10000) {
		category cat(CATEGORY_path);

		srand(time(NULL));
		for (int step = 0; step < n_shuffle; step++) {
			int i = rand() % files.size();
			int j = rand() % files.size();

			swap(cat[i], cat[j]);
			swap(files[i], files[j]);
		}

		cat.save(CATEGORY_path);
	}

	void init_queries(vector<string> &files, int n_queries = 5000) {
		category cat(CATEGORY_path);

		vector<string> queries;
		for (int i = 1; i <= n_queries; i++) {
			auto temp = cat[files.size() - 1];
			queries.push_back(files.back());
			cat.erase(cat.find(files.size() - 1));
			cat[-i] = temp;
			files.pop_back();
		}

		ofstream out_file(QUERIES_path);
		for (int i = 0; i < queries.size(); i++) {
			out_file << queries[i] << endl;
			//cerr << queries[i].find("Data\\20_newsgroups\\" + cat[-1 - i]) << endl;
		}
		out_file.close();

		cat.save(CATEGORY_path);
	}

	void init_list_files() {

		auto clusters = get_list_directory();
		auto files = get_list_files(clusters);
		shuffle_list_files(files);
		init_queries(files);
		// Save list files
		ofstream out_file(LISTFILES_path);
		for (int i = 0; i < files.size(); i++)
			out_file << files[i] << endl;
		out_file.close();
	}

	vector<string> load_list_files() {
		vector<string> ret;

		ifstream in_file(LISTFILES_path);
		for (string file; in_file >> file;)
			ret.push_back(file);
		in_file.close();

		return ret;
	}

	vector<string> load_queries() {
		vector<string> ret;

		ifstream in_file(QUERIES_path);
		for (string file; in_file >> file;)
			ret.push_back(file);
		in_file.close();

		return ret;
	}

};