#pragma once

#include "configure.h"

struct topic {
	int id;					// .I
	int u_id;				// .U
	vector<string> terms;	// .M
	string title;			// .T
	string pub_type;		// .P
	string abstract;		// .W
	string author;			// .A
	string source;			// .S

	topic(int _id = -1) : id(_id) {}
};

