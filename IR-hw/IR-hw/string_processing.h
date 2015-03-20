#pragma once

#include "configure.h"

string lower_case(string s) {
	string ret = "";
	for (int i = 0; i < s.length(); i++)
		ret += tolower(s[i]);
	return ret;
}

string to_string(int n) {
	if (n == 0) 
		return "0";

	string ret = "";
	while (n > 0) {
		ret = char('0' + n % 10) + ret;
		n /= 10;
	}
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

vector <string> split_tokens(string s, string delimiters) {
	set <char> delims;
	for (int i = 0; i < delimiters.size(); i++)
		delims.insert(delimiters[i]);

	vector <string> ret;
	string cur = "";
	for (int i = 0; i < s.length(); i++)
		if (delims.find(s[i]) != delims.end()) {
			if (!cur.empty()) {
				ret.push_back(cur);
				cur = "";
			}
		}
		else
			cur = cur + s[i];
	if (!cur.empty())
		ret.push_back(cur);
	return ret;
}