	#include <iostream>
	#include <istream>
	#include <vector>
	#include <string>
	#include <cctype>
	#include <algorithm>
	#include "variable.h"
	#include "lexer.h" 
	#include "inputbuf.h"

	//Error Code Definition
	//1 - Syntax Error
	//2 - 1.1 Error
	//3 - 1.2 Error
	//4 - 1.3 Error

	string decl_error;
	vector<scope> scopes;
	int scopeCount = -1;

	//All my 16 functions
	void parse_program();
	void parse_scope();
	void parse_scope_list();
	void parse_var_decl();
	void parse_id_list();
	TokenType parse_type_name();
	void parse_stmt_list();
	void parse_stmt();
	void parse_assign_stmt();
	void parse_while_stmt();
	TokenType parse_expr(int lineNo);
	TokenType parse_arithmatic_operator();
	TokenType parse_binary_boolean_operator();
	TokenType parse_relational_operator();
	TokenType parse_primary();
	void parse_bool_const();
	void parse_condition();

	LexicalAnalyzer lexer;
	Token t;
	vector<string> typeErrorMessage;
	bool isThereError = false;

	void type_mismatch(int lineNo, string errorType)
	{
		if (!isThereError)
		{
			typeErrorMessage.push_back("TYPE MISMATCH " + to_string(lineNo) + " " + errorType);
			isThereError = true;
		}
	}

	TokenType typeCheck(TokenType operator_token, TokenType type1, TokenType type2, int lineNo) { //Checks  the expression inside.
		if (operator_token == PLUS || operator_token == MINUS || operator_token == MULT || operator_token == DIV) //Arithmatic Operator
		{
			if ((type1 == INT || type1 == REAL) && (type2 == INT || type2 == REAL))
			{
				if (type1 == REAL || type2 == REAL)
				{
					return REAL; //I1
				}
				if (operator_token == PLUS || operator_token == MINUS || operator_token == MULT)
				{
					if (type1 == INT && type2 == INT)
					{
						return INT; //I2 
					}
				}
				if (operator_token == DIV)
				{
					if (type1 == INT && type2 == INT)
					{
						return REAL; //I3
					}
				}
			}
			else
			{
				type_mismatch(lineNo, "C3"); //C3 Error 
			}
		}
		if (operator_token == XOR || operator_token == OR || operator_token == AND) { //OP token is boolean
			if (type1 == BOOLEAN && type2 == BOOLEAN)
			{
				return BOOLEAN; //I4
			}
			else
			{
				type_mismatch(lineNo, "C4");
				//Throw C4 Error
			}
		}
		if (operator_token == GREATER || operator_token == GTEQ || operator_token == LESS || operator_token == NOTEQUAL || operator_token == LTEQ) //OP Token is Relational Operator
		{
			if (type1 == INT || type1 == REAL) {
				if ((type2 == INT || type2 == REAL))
				{
					return BOOLEAN; //I5
				}
				else
				{
					type_mismatch(lineNo, "C6");
					//Throw C6 Error
				}
			}
			else if(type1 == type2){
				return BOOLEAN; //I5	
			}
			else
			{
				type_mismatch(lineNo, "C5");
				//Throw C5 Error
			}
		}
		return ERROR;
	}

	variable getVar(string varName) { //Checks all scopes for the varType pertaining to the string passed
		variable var;
		for (int j = scopeCount; j > -1; j--)
		{
			scope currentScope = scopes[j];
			for (size_t i = 0; i < currentScope.v.size(); i++)
			{
				string myString = scopes[j].v[i].name;
				if (varName.compare(myString) == 0)
				{
					var = scopes[j].v[i];
					return var;
				}
			}
		}
		return var; //useless, won't be needed
	} 

	void syntax_error() {
		cout << "Syntax Error" << endl;
		exit(1);
	}
	//For 1.2 Error Only
	bool isVarInAnyScope(string varName) {
		for (int j = scopeCount; j > -1; j--)
		{
			scope currentScope = scopes[j];
			for (size_t i = 0; i < scopes[j].v.size(); i++)
			{
				string currentString = currentScope.v[i].name;
				if (currentString.compare(varName) == 0)
				{
					return true;
				}
			}
		}
		return false;
	}

	//For 1.1 Error Only
	bool isVarInLocalScope(string varName) { //Name is pretty fucking self explanatory
		int numVariables = scopes[scopeCount].v.size();
			scope currentScope = scopes[scopeCount];
			for (int i = 0; i < numVariables; i++)
			{
				string currentString = currentScope.v[i].name;
				if (currentString.compare(varName) == 0) //Checks if the variable is already in the scope
				{
					return true;
				}
			}
			return false;
		}

	void variableGetsUsed(string varName) { //Another helper function, returns the variable once I pass in a string	
		variable test = getVar(varName);
		for (int j = scopeCount; j > -1; j--)
		{
			scope currentScope = scopes[j];
			for (size_t i = 0; i < currentScope.v.size(); i++)
			{
				string myString = scopes[j].v[i].name;
				if (varName.compare(myString) == 0)
				{
					scopes[j].v[i].used = true;
					return;
				}
			}
		}
	}

	void makeVariables(string name, TokenType type, int lineNo) {
			variable var;
			var.lineNo = lineNo;
			var.type = type;
			var.name = name;
			scopes[scopeCount].v.push_back(var);
		}

		string anyUnused() //Small little side note: This only gets the first unused variable, if we want to return ALL the variables, we can use a vector and return that, either way it should work pretty well :)
		{
			for (int j = scopeCount; j > -1; j--)
			{
				scope currentScope = scopes[j]; //Get the current scope
			for (size_t i = 0; i < currentScope.v.size(); i++)
			{
				if (currentScope.v[i].used == false)
				{
					return currentScope.v[i].name;
				}
			}
		}
		return "";
	}

	void expect(TokenType expected, Token token) {
		if (token.token_type != expected)
		{
			syntax_error();
		}
		else
		{
			t = lexer.GetToken(); //Consume the token
		}
	}

	int main() {
		t = lexer.GetToken();
		parse_program();
		if (!decl_error.empty())
		{
			cout << decl_error << endl;
			exit(2); //Used by the holy debugger to tell if I got the right output
		}
		if (!typeErrorMessage.empty())
		{
			for (size_t i = 0; i < typeErrorMessage.size(); i++)
			{
				cout << typeErrorMessage.at(i) << endl;
			}
			exit(3);
		}
	}

	void parse_program() {
		parse_scope();
	}

	void parse_scope() {
		expect(LBRACE, t);

		scope temp; //Create a new scope
		scopes.push_back(temp); //Add it to the scope vector
		scopeCount++; //Increment the scope count

		parse_scope_list();
		string check = anyUnused();
		if(!check.empty())
		{
			decl_error = "ERROR CODE 1.3 " + check;
		}		

		scopes.pop_back();					//Pop the bottom scope off
		scopeCount--;//Decremement scope count

		expect(RBRACE, t); 
	} 

	void parse_scope_list() {
		Token peek = lexer.GetToken();
		lexer.UngetToken(peek);
		if (t.token_type == LBRACE)
		{
			parse_scope();
		}
		else if (t.token_type == ID && peek.token_type != EQUAL)
		{
			parse_var_decl(); //Have to parse both for the condition
		}
		else if ((t.token_type == WHILE) || (t.token_type == ID && peek.token_type == EQUAL))
		{
			parse_stmt();	//have to parse both for both conditions
		}
		else
		{
			syntax_error();
		}
		Token check = lexer.GetToken();
		lexer.UngetToken(check);
		//After we are done with the above bullshittery
		if (t.token_type == WHILE || (t.token_type == ID && check.token_type != EQUAL) || t.token_type == LBRACE || (t.token_type == ID && check.token_type == EQUAL))
		{
			parse_scope_list();
		}
		//Don't put an else here!
	}

	vector<string> variableNames; //This is literally a hack... don't follow my advice

	void parse_var_decl() {
		parse_id_list(); //Needs to return ID
		int lineNo = t.line_no;
		expect(COLON, t);//COLON
		TokenType type = parse_type_name();
		expect(SEMICOLON, t); //SEMICOLON
		for (size_t i = 0; i < variableNames.size(); i++)
		{
			if (isVarInLocalScope(variableNames[i]))
			{
				decl_error = "ERROR CODE 1.1 " + variableNames[i];  //MAY NEED WORK, MAY BLOW OUT PARSER, DOUBLE CHECk
			}
			else
			{
				makeVariables(variableNames[i], type, lineNo); //Function to do stuff
			}
		}
		variableNames.clear(); //I told it was a hack...
	}


	void parse_id_list() {
		variableNames.push_back(t.lexeme); //ITS A HACK !!!!!!!!!
		expect(ID, t); //ID
		if (t.token_type == COMMA)
		{
			expect(COMMA, t); //ID
			parse_id_list();
		}
	}

	TokenType parse_type_name() { //This will probably need work later to setup the statement logic
		if (t.token_type == REAL)
		{
			expect(REAL, t);
			return REAL;
		}
		else if (t.token_type == INT)
		{
			expect(INT, t);
			return INT;
		}
		else if (t.token_type == BOOLEAN)
		{
			expect(BOOLEAN, t);
			return BOOLEAN;
		}
		else if (t.token_type == STRING)
		{
			expect(STRING, t);
			return STRING;
		}
		else
		{
			syntax_error();
			return END_OF_FILE; //Never going to be reached
		}
	}

	void parse_stmt_list() {
		parse_stmt();
		Token peek = lexer.GetToken();
		lexer.UngetToken(peek);
		if ((t.token_type == WHILE) || (t.token_type == ID && peek.token_type == EQUAL))
		{
			parse_stmt_list();
		}
	}

	void parse_stmt() {
		Token peek = lexer.GetToken();
		lexer.UngetToken(peek);
		if (t.token_type == ID && peek.token_type == EQUAL)
		{
			parse_assign_stmt();
		}
		else if (t.token_type == WHILE)
		{
			parse_while_stmt();
		}
		else
		{
			syntax_error();
		}
	}

	void parse_assign_stmt() {
		string nameboy = t.lexeme;
		if (!isVarInAnyScope(nameboy)) {
			decl_error = "ERROR CODE 1.2 " + nameboy;
		}
		else
		{
			variableGetsUsed(nameboy);
		}
		variable temp = getVar(nameboy);
		TokenType LHS = temp.type;
		int lineNo = t.line_no;
		expect(ID, t); // ID
		expect(EQUAL, t); //Equal
		TokenType RHS = parse_expr(lineNo);
		if ((LHS == INT && RHS != INT) || (LHS == BOOLEAN && RHS != BOOLEAN) || (LHS == STRING && RHS != STRING))
		{
			type_mismatch(lineNo, "C1");
			//Throw C1 Errors
		}
		else if (LHS == REAL && (RHS != INT || RHS != REAL))
		{
			type_mismatch(lineNo, "C2");
			//Throw C2 Errors
		}
		expect(SEMICOLON, t); //SEMICOLON
	}

	void parse_while_stmt() {
		expect(WHILE, t); //While	
		parse_condition();

		Token peek = lexer.GetToken();
		lexer.UngetToken(peek);
		if (t.token_type == LBRACE) 
		{
			expect(LBRACE, t); //LBRACE TODO: determine if this is included as a scope?
			/*
			scope temp; //Create a new scope
			scopes.push_back(temp); //Add it to the scope vector
			scopeCount++; //Increment the scope count
			*/
			parse_stmt_list();

			expect(RBRACE, t); //RBRACE

			//string var = anyUnused(); //I use a backwards method in order to pull the first unused call, see function for more documentation
			/*if (!var.empty())
			{
				decl_error = "ERROR CODE 1.3 " + var;
			}
			*/
			//scopes.pop_back();
			//scopeCount--;
		}
		else if ((t.token_type == ID && peek.token_type == EQUAL) || t.token_type == WHILE) {
			parse_stmt();
		}
		else
		{
			syntax_error();
		}
	}

	TokenType parse_expr(int lineNo) {
		if (t.token_type == PLUS || t.token_type == MINUS || t.token_type == MULT || t.token_type == DIV)
		{
			TokenType t = parse_arithmatic_operator();
			TokenType l = parse_expr(lineNo);
			TokenType g = parse_expr(lineNo);
			return typeCheck(t, l, g, lineNo);
		}
		else if (t.token_type == AND || t.token_type == OR || t.token_type == XOR)
		{
			TokenType t = parse_binary_boolean_operator();
			TokenType l = parse_expr(lineNo);
			TokenType g = parse_expr(lineNo);
			return typeCheck(t, l, g, lineNo);
		}
		else if (t.token_type == GREATER || t.token_type == GTEQ || t.token_type == LESS || t.token_type == NOTEQUAL || t.token_type == LTEQ)
		{
			TokenType t = parse_relational_operator();
			TokenType l = parse_expr(lineNo);
			TokenType g = parse_expr(lineNo);
			return typeCheck(t, l, g, lineNo);
		}
		else if (t.token_type == NOT)
		{
			expect(NOT, t); //NOT
			TokenType t = parse_expr(lineNo);
			if (t != BOOLEAN)
			{
				type_mismatch(lineNo, "C8");
				//Throw C8 Error
			}
			return BOOLEAN;
		}
		else if (t.token_type == ID || t.token_type == NUM || t.token_type == REALNUM || t.token_type == STRING_CONSTANT || t.token_type == TRUE || t.token_type == FALSE)
		{
			TokenType type = parse_primary();
			return type;
		}
		else
		{
			syntax_error();
			return END_OF_FILE; //To shut up the compiler
		}
	}

	TokenType parse_arithmatic_operator() {
		if (t.token_type == PLUS)
		{
			expect(PLUS, t);
			return PLUS;
		}
		else if (t.token_type == MINUS)
		{
			expect(MINUS, t);
			return MINUS;
		}
		else if (t.token_type == MULT)
		{
			expect(MULT, t);
			return MULT;
		}
		else if (t.token_type == DIV)
		{
			expect(DIV, t);
			return DIV;
		}
		else
		{
			syntax_error();
			return END_OF_FILE; //Shuts up the compiler
		}
	}

	TokenType parse_binary_boolean_operator() {
		if (t.token_type == AND)
		{
			expect(AND, t);
			return AND;
		}
		else if (t.token_type == OR)
		{
			expect(OR, t);
			return OR;
		}
		else if (t.token_type == XOR)
		{
			expect(XOR, t);
			return XOR;
		}
		else
		{
			syntax_error();
			return END_OF_FILE; //To shut up the compiler
		}
	}

	TokenType parse_relational_operator() {
		if (t.token_type == GREATER)
		{
			expect(GREATER, t);
			return GREATER;
		}
		else if (t.token_type == GTEQ)
		{
			expect(GTEQ, t);
			return GTEQ;
		}
		else if (t.token_type == LESS)
		{
			expect(LESS, t);
			return LESS;
		}
		else if (t.token_type == NOTEQUAL)
		{
			expect(NOTEQUAL, t);
			return NOTEQUAL;
		}
		else if (t.token_type == LTEQ)
		{
			expect(LTEQ, t);
			return LTEQ;
		}
		else
		{
			syntax_error();
			return END_OF_FILE; //To shut up the compiler
		}
	}

	TokenType parse_primary() {
		if (t.token_type == TRUE || t.token_type == FALSE)
		{
			parse_bool_const();
			return BOOLEAN;
		}
		else if (t.token_type == ID)
		{
			string nameBoy = t.lexeme; //Referencing a variable in the following way : A  = B; etc; I mean, we obviously can't do that lol
			if (!isVarInAnyScope(nameBoy))
			{
				decl_error =  "ERROR CODE 1.2 " + nameBoy;
			}
			else
			{
				variableGetsUsed(nameBoy); // :)
			}
			TokenType type = getVar(nameBoy).type;
			expect(ID, t);
			return type;
		}
		else if (t.token_type == NUM)
		{
			expect(NUM, t);
			return INT;
		}
		else if (t.token_type == REALNUM)
		{
			expect(REALNUM, t);
			return REAL;
		}
		else if (t.token_type == STRING_CONSTANT)
		{
			expect(STRING_CONSTANT, t);
			return STRING;
		}
		else
		{
			syntax_error();
			return END_OF_FILE; //To shut up the compiler
		}
	}
 
	void parse_bool_const() {
		if (t.token_type == TRUE)
		{
			expect(TRUE, t);
		}
		else if (t.token_type == FALSE)
		{
			expect(FALSE, t);
		}
		else
		{
			syntax_error();
		}
	}

	void parse_condition() {
		expect(LPAREN, t);
		int lineNo = t.line_no;
		TokenType g = parse_expr(lineNo);
		if (g != BOOLEAN)
		{
			//Throw C7 Error
			type_mismatch(lineNo, "C7");
		}
		expect(RPAREN, t);
	}