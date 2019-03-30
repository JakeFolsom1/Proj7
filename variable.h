#pragma once
#include <vector>
#include <string.h>
#include "lexer.h"

using namespace std;

struct variable {
public:
	string name;
	TokenType type;
	int lineNo;
	bool used = false;
};

struct scope {
public: 
	vector<variable> v;
};
