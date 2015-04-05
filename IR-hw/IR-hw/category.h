#pragma once
#include "configure.h"

class category: public map <int, string> {
public:

	category() {}
	category(string file_name) {
		ifstream in_file(file_name);

		int n;
		in_file >> n;
		for (int i = 0; i < n; i++) {
			int k;
			string v;
			in_file >> k >> v;
			this->operator[](k) = v;
		}

		in_file.close();
	}

	void save(string file_name) {
		ofstream out_file(file_name);

		out_file << this->size() << "\n";
		for (auto it = this->begin(); it != this->end(); it++)
			out_file << it->first << " " << it->second << "\n";

		out_file.close();
	}
	
};

