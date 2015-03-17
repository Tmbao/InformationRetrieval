#pragma once

#include "configure.h"

struct ID {
	int term_id, doc_id;
	ID (int _term_id = 0, int _doc_id = 0): term_id(_term_id), doc_id(_doc_id) {}
	friend bool operator != (const ID &a, const ID &b) {
		return make_pair(a.term_id, a.doc_id) != make_pair(b.term_id, b.doc_id);
	}
	friend bool operator < (const ID &a, const ID &b) {
		return make_pair(a.term_id, a.doc_id) < make_pair(b.term_id, b.doc_id);
	}
	friend bool operator <= (const ID &a, const ID &b) {
		return make_pair(a.term_id, a.doc_id) <= make_pair(b.term_id, b.doc_id);
	}
};

struct Index: ID {
	int freq;
};
