#pragma once

#include "configure.h"

string lower_case(string s) {
	string ret = "";
	for (int i = 0; i < s.length(); i++)
		ret += tolower(s[i]);
	return ret;
}

vector <string> split_tokens(string s) {
	vector <string> ret;
	string cur = "";

	for (int i = 0; i < s.length(); i++)
		if (!isalpha(s[i])) {
			if (!cur.empty()) {
				ret.push_back(cur);
				cur = "";
			}
		} else 
			cur = cur + s[i];
	if (!cur.empty())
		 ret.push_back(cur);
	return ret;
}
