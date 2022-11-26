#include <stdio.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <set>

using namespace std;

vector<tuple<string, string, string, string>> instructions;

vector<string> tokens;

int RAD;

int CAD;

tuple<int, int> location;

int instructions_end_index;

vector<tuple<string, string, int, int, int>> DRAM_queue;

set<string> unsafe_registers;

int row_buffer_updates = 0;

int row_in_buffer = -1;

unordered_map<string, int> registers;

string register_names[32] = {
	"zero",
	"at",
	"v0",
	"v1",
	"a0",
	"a1",
	"a2",
	"a3",
	"t0",
	"t1",
	"t2",
	"t3",
	"t4",
	"t5",
	"t6",
	"t7",
	"s0",
	"s1",
	"s2",
	"s3",
	"s4",
	"s5",
	"s6",
	"s7",
	"t8",
	"t9",
	"k0",
	"k1",
	"gp",
	"sp",
	"fp",
	"ra"
};

int error;

string load_error;

int PC = 0;

int int_regs[32] = {0};

int memory[1024][256];

string hex_regs[32];

unordered_map<string, int> labels;


int clock_cycles = 0;


string trim(string input){ 

	int start = input.find_first_not_of(", \t");
	//int last = input.find_last_not_of(", \t");

	if(start == -1){
		return "";
	}else{
		return input.substr(start);
		/*if(input.substr(input.size()-1) == "\n"){
			return input.substr(start, input.size()-1);
		}else{
			return input.substr(start);
		}*/
	}
}

void tokenize(string line){

	//remove leading and trailing white spaces
	line = trim(line);

	//check if the line is empty
	if(line == ""){
		return;
	}
	else //if the string is not empty
	{
		//the token is separated from the rest of the instruction using a space, tab or comma
		string token;
		
		//finding the position of the first whitespace or comma in the now trimmed line
		int first_space;
		int first_tab;
		int first_comma;
		int first;
		
		first_space = line.find(" ");
		if(first_space == -1){
			first_space = line.size();
		}

		first_tab = line.find("\t");
		if(first_tab == -1){
			first_tab = line.size();
		}

		first_comma = line.find(",");
		if(first_comma == -1){
			first_comma = line.size();
		}

		first = min(first_space, min(first_tab, first_comma));

		//if there is no sapce, tab or comma in the entire line, then the entire line is one token
		if(first == line.size()){
			token = line;
			line = "";
			tokens.push_back(token);
			tokenize(line);
			return;
		}else{
		//the part of the string before the first space, tab or comma is stored as a token
			token = line.substr(0, first);
			line = line.substr(first+1);
			line = trim(line);
			tokens.push_back(token);
			tokenize(line);
			return;
		}
	}
}

void load_label(vector<string> tokens, int memory_num){

	tuple<string, string, string, string> instruction;

	string token;
	token = tokens.at(0);

	if(tokens.size() == 1){
		if(labels.find(token.substr(0,tokens.at(0).size()-1)) != labels.end()){
			cout << "cannot define same label twice" << endl;
			error = 1;
			return;
		}
		instruction = make_tuple(token.substr(0,tokens.at(0).size()-1), "", "", "");
		instructions.push_back(instruction);
		labels[token.substr(0,tokens.at(0).size()-1)] = memory_num;
		return;
	}
	if(tokens.at(1).substr(0,1) == "#"){
		instruction = make_tuple(token.substr(0,tokens.at(0).size()-1), "", "", "");
		instructions.push_back(instruction);
		labels[token.substr(0,tokens.at(0).size()-1)] = memory_num;
		return;	
	}
	load_error = "label should not be followed by any operand in the same line, it can only be followed by a comment in the same line";
	return;
}


void load_add(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 4){
		load_error = "add operation takes 3 operands";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "add operation takes 3 operands";
		return;
	}

	if(token == "$zero" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}
	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	for(int i = 2; i<4; i++){

		token = tokens.at(i);

		if(token.substr(0,1) == "#"){
			load_error = "add operation takes 3 operands";
			return;
		}

		if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}
		if(registers.find(token) == registers.end()){
			load_error ="operand is not a valid register";
			return;
		}
	}

	if(tokens.size() > 4){
		if(tokens.at(4).substr(0,1) != "#"){
			load_error = "add operation takes 3 operands, excess operands provided";
			return;
		}
	}

	instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), tokens.at(3));
	instructions.push_back(instruction);
	return;
}


void load_sub(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 4){
		load_error = "sub operation takes 3 operands";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "sub operation takes 3 operands";
		return;
	}

	if(token == "$zero" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}
	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	for(int i = 2; i<4; i++){

		token = tokens.at(i);

		if(token.substr(0,1) == "#"){
			load_error = "sub operation takes 3 operands";
			return;
		}

		if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}
		if(registers.find(token) == registers.end()){
			load_error ="operand is not a valid register";
			return;
		}
	}

	if(tokens.size() > 4){
		if(tokens.at(4).substr(0,1) != "#"){
			load_error = "sub operation takes 3 operands, excess operands provided";
			return;
		}
	}

	instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), tokens.at(3));
	instructions.push_back(instruction);
	return;
}


void load_mul(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 4){
		load_error = "mul operation takes 3 operands";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "mul operation takes 3 operands";
		return;
	}

	if(token == "$zero" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}

	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	token = tokens.at(2);

	if(token.substr(0,1) == "#"){
		load_error = "mul operation takes 3 operands";
		return;
	}

	if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}

	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	token = tokens.at(3);

	if(token.substr(0,1) == "#"){
		load_error = "mul operation takes 3 operands";
		return;
	}

	if(registers.find(token) == registers.end()){
		
		char* p;
		long converted = strtol(token.c_str(), &p, 10);
		if(*p){
			load_error = "operand is not a valid register";
			return;
		}
	}

	if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}

	if(tokens.size() > 4){
		if(tokens.at(4).substr(0,1) != "#"){
			load_error = "mul operation takes 3 operands, excess operands provided";
			return;
		}
	}

	instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), tokens.at(3));
	instructions.push_back(instruction);
	return;
}


void load_beq(std::vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 4){
		load_error = "beq operation takes 3 operands";
		return;
	}

	string token;

	for(int i = 1; i<3; i++){

		token = tokens.at(i);

		if(token.substr(0,1) == "#"){
			load_error = "beq operation takes 3 operands";
			return;
		}

		if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}
		if(registers.find(token) == registers.end()){
			load_error ="operand is not a valid register";
			return;
		}
	}

	token = tokens.at(3);

	if(token.substr(0,1) == "#"){
		load_error = "beq operation takes 3 operands";
		return;
	}

	if(tokens.size() > 4){
		if(tokens.at(4).substr(0,1) != "#"){
			load_error = "beq operation takes 3 operands, excess operands provided";
			return;
		}
	}

	instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), tokens.at(3));
	instructions.push_back(instruction);
	return;

}


void load_bne(std::vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 4){
		load_error = "bne operation takes 3 operands";
		return;
	}

	string token;

	for(int i = 1; i<3; i++){

		token = tokens.at(i);

		if(token.substr(0,1) == "#"){
			load_error = "bne operation takes 3 operands";
			return;
		}

		if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}
		if(registers.find(token) == registers.end()){
			load_error ="operand is not a valid register";
			return;
		}
	}

	token = tokens.at(3);

	if(token.substr(0,1) == "#"){
		load_error = "bne operation takes 3 operands";
		return;
	}

	if(tokens.size() > 4){
		if(tokens.at(4).substr(0,1) != "#"){
			load_error = "bne operation takes 3 operands, excess operands provided";
			return;
		}
	}

	instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), tokens.at(3));
	instructions.push_back(instruction);
	return;
}


void load_slt(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 4){
		load_error = "slt operation takes 3 operands";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "slt operation takes 3 operands";
		return;
	}

	if(token == "$zero" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}

	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	for(int i = 2; i<4; i++){

		token = tokens.at(i);

		if(token.substr(0,1) == "#"){
			load_error = "slt operation takes 3 operands";
			return;
		}

		if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}
		if(registers.find(token) == registers.end()){
			load_error ="operand is not a valid register";
			return;
			}
	}

	if(tokens.size() > 4){
		if(tokens.at(4).substr(0,1) != "#"){
			load_error = "slt operation takes 3 operands, excess operands provided";
			return;
		}
	}

	instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), tokens.at(3));
	instructions.push_back(instruction);
	return;
}


void load_j(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 2){
		load_error = "j operation takes 1 operand";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "j operation takes 1 operand";
		return;
	}


	if(tokens.size() > 2){
		if(tokens.at(2).substr(0,1) != "#"){
			load_error = "j operation takes 1 operand, excess operands provided";
			return;
		}
	}

	instruction = make_tuple(tokens.at(0), tokens.at(1), "", "");
	instructions.push_back(instruction);
	return;
}


void load_lw(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 3){
		load_error = "lw operation takes 2 operands";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "lw operation takes 3 operands";
		return;
	}

	if(token == "$zero" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}

	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	token = tokens.at(2);

	if(token.substr(0,1) == "#"){
		load_error = "lw operation takes 2 operands : register, address";
		return;
	}

	char* p;
	long converted = strtol(token.c_str(), &p, 10);
	if(*p){
		if(token.size()<5){
			load_error = "lw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		int lparen_pos = token.find_first_of("(");
		int rparen_pos = token.find_first_of(")");
		int register_name_length = rparen_pos - lparen_pos - 1;

		char* q;
		long converted = strtol(token.substr(0,lparen_pos).c_str(), &q, 10);
		if(*q){
			load_error = "lw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		if(registers.find(token.substr(lparen_pos + 1, register_name_length)) == registers.end()){
			load_error = "lw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		string reg = token.substr(lparen_pos + 1, register_name_length);

		if(reg == "$at" || reg == "$gp" || reg == "$k0" || reg == "$k1" || reg == "$ra" || reg == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}

		instruction = make_tuple(tokens.at(0), tokens.at(1), token.substr(0,lparen_pos), reg);
	}else{
		load_error = "invalid instruction format, use offset and register for memory address";
		return;
	}
	instructions.push_back(instruction);
	return;

}


void load_sw(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 3){
		load_error = "sw operation takes 2 operands";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "sw operation takes 2 operands : register, address";
		return;
	}

	if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}
	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	token = tokens.at(2);

	if(token.substr(0,1) == "#"){
		load_error = "sw operation takes 2 operands : register, address";
		return;
	}

	char* p;
	long converted = strtol(token.c_str(), &p, 10);
	if(*p){
		if(token.size()<5){
			load_error = "sw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		int lparen_pos = token.find_first_of("(");
		int rparen_pos = token.find_first_of(")");
		int register_name_length = rparen_pos - lparen_pos - 1;

		char* q;
		long converted = strtol(token.substr(0,lparen_pos).c_str(), &q, 10);
		if(*q){
			load_error = "sw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		if(registers.find(token.substr(lparen_pos + 1, register_name_length)) == registers.end()){
			load_error = "sw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		string reg = token.substr(lparen_pos + 1, register_name_length);

		if(reg == "$at" || reg == "$gp" || reg == "$k0" || reg == "$k1" || reg == "$ra" || reg == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}

		instruction = make_tuple(tokens.at(0), tokens.at(1), token.substr(0,lparen_pos), token.substr(lparen_pos + 1, register_name_length));

	}else{
		load_error = "invalid instruction format, use offset and register for memory address";
		return;
	}

	instructions.push_back(instruction);
	return;

}

void load_addi(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 4){
		load_error = "addi operation takes 3 operands";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "addi operation takes 3 operands";
		return;
	}

	if(token == "$zero" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}

	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	token = tokens.at(2);

	if(token.substr(0,1) == "#"){
		load_error = "addi operation takes 3 operands";
		return;
	}

	if(token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
		load_error ="Trying to access registers not accessible to user";
		return;
	}

	if(registers.find(token) == registers.end()){
		load_error ="operand is not a valid register";
		return;
	}

	token = tokens.at(3);

	if(token.substr(0,1) == "#"){
		load_error = "addi operation takes 3 operands";
		return;
	}

	char* p;
	long converted = strtol(token.c_str(), &p, 10);
	if(*p){
		load_error ="operand is not a valid immediate value";
		return;
	}

	if(tokens.size() > 4){
		if(tokens.at(4).substr(0,1) != "#"){
			load_error = "addi operation takes 3 operands, excess operands provided";
			return;
		}
	}

	instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), tokens.at(3));
	instructions.push_back(instruction);
	return;
	
}

void load_file(string infilename){

	//opening the input file and storing the fstream as infile
	ifstream infile(infilename);

	//vector called instructions to store all the instructions as a 4-tuple of strings
	vector<tuple<string, string, string, string>> instructions;
	
	//reading the input file line by line
	string line;
	int linenum; linenum = 0;
	int memory_num; memory_num = 0;
	while(getline(infile, line)){
		
		linenum++;
		tokens.clear();

		line = line.substr(0, line.size());
		
		tokenize(line);



		load_error = "";
		
		if(tokens.size()<1){
			continue;
		}

		string token1;
		
		token1 = tokens.at(0);
		
		//if the line starts with a "#", the whole line is a comment
		if(token1.substr(0,1) == "#"){
			continue;
		}

		//if the first token ends with a semicolon, we have a label declaration
		//we load the instruction as a label declaration
		if(token1.substr(token1.size()-1) == ":"){
			load_label(tokens, memory_num);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		//if the first token is an operation, we load the instruction as the correct operation

		if(token1 == "add"){
			load_add(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "sub"){
			load_sub(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "mul"){
			load_mul(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "beq"){
			load_beq(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "bne"){
			load_bne(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "slt"){
			load_slt(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "addi"){
			load_addi(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "lw"){
			load_lw(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "sw"){
			load_sw(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "j"){
			load_j(tokens);
			if(load_error != ""){
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		cerr << "load error: " << linenum << ": the first token is neither a label, a comment nor a valid operation" << endl;
		error = 1;
		return;
	}

	instructions_end_index = memory_num;
	return;
}

void print_registers(){

	char hex_string[20];

	for(int i = 0; i<31; i++){

		sprintf(hex_string, "%X", int_regs[i]);
		cout << hex_string << " ";
	}

	sprintf(hex_string, "%X", int_regs[31]);
	cout << hex_string << endl;

	return;

}

void print_instructions(int* executions){

	for(int i = 0; i<instructions_end_index/4; i++){

		tuple<string, string, string, string> instruction = instructions.at(i);
		cout << get<0>(instruction) << "\t" << get<1>(instruction) << "\t" << get<2>(instruction) << "\t" << get<3>(instruction) << "\t" << executions[i] << endl;
	}
	return;

}

void DRAM_enqueue(tuple<string, string, int, int, int> DRAM_queue_element){
	
	if(DRAM_queue.size() == 0){
		DRAM_queue.push_back(DRAM_queue_element);
		return;
	}

	if(get<0>(DRAM_queue_element) == "lw"){
		
		tuple<string, string, int, int, int> curr_DRAM_queue_element;
		int l;

		for(l = DRAM_queue.size(); l>0; l--){
			curr_DRAM_queue_element = DRAM_queue.at(l-1);
			if(get<0>(curr_DRAM_queue_element) == "lw"){
				if(get<1>(curr_DRAM_queue_element) == get<1>(DRAM_queue_element)){
					DRAM_queue.erase(DRAM_queue.begin()+l-1);
					if(DRAM_queue.size() != 0){
						if(l == 1){
							curr_DRAM_queue_element = DRAM_queue.at(0);
							tuple<string, string, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(curr_DRAM_queue_element), get<1>(curr_DRAM_queue_element), get<2>(curr_DRAM_queue_element), get<3>(curr_DRAM_queue_element), clock_cycles);
							DRAM_queue.at(0) = new_DRAM_queue_element;
						}
					}
				}
			}
		}
		for(l = DRAM_queue.size(); l>0; l--){
			curr_DRAM_queue_element = DRAM_queue.at(l-1);
			if(get<0>(curr_DRAM_queue_element) == "lw"){
				continue;
			}else{
				if(get<2>(curr_DRAM_queue_element) == get<2>(DRAM_queue_element) && get<3>(curr_DRAM_queue_element) == get<3>(DRAM_queue_element)){
					break;
				}
			}
		}

		if(l == 0){
			l = 1;
		}

		for(l; l<DRAM_queue.size(); l++){
			curr_DRAM_queue_element = DRAM_queue.at(l-1);
			if(get<2>(curr_DRAM_queue_element) == get<2>(DRAM_queue_element)){
				DRAM_queue.insert(DRAM_queue.begin()+l, DRAM_queue_element);
				return;
			}
		}

		DRAM_queue.push_back(DRAM_queue_element);
		return;

	}else{

		tuple<string, string, int, int, int> curr_DRAM_queue_element;
		int l;
		for(l = DRAM_queue.size(); l>0; l--){
			curr_DRAM_queue_element = DRAM_queue.at(l-1);
			if(get<2>(curr_DRAM_queue_element) == get<2>(DRAM_queue_element) && get<3>(curr_DRAM_queue_element) == get<3>(DRAM_queue_element)){
				if(get<0>(curr_DRAM_queue_element) == "lw"){
					break;
				}else{
					DRAM_queue.erase(DRAM_queue.begin()+l-1);
					if(DRAM_queue.size() != 0){
						if(l == 1){
							curr_DRAM_queue_element = DRAM_queue.at(0);
							tuple<string, string, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(curr_DRAM_queue_element), get<1>(curr_DRAM_queue_element), get<2>(curr_DRAM_queue_element), get<3>(curr_DRAM_queue_element), clock_cycles);
							DRAM_queue.at(0) = new_DRAM_queue_element;
						}
					}
				}
			}
		}

		if(l == 0){
			l = 1;
		}

		for(l; l<DRAM_queue.size(); l++){
			curr_DRAM_queue_element = DRAM_queue.at(l-1);
			if(get<2>(curr_DRAM_queue_element) == get<2>(DRAM_queue_element)){
				DRAM_queue.insert(DRAM_queue.begin()+l, DRAM_queue_element);
				return;
			}
		}

		DRAM_queue.push_back(DRAM_queue_element);
		return;
	}
}

int execute_add(tuple<string, string, string, string> instruction) {

	if(unsafe_registers.find(get<1>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<2>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<3>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;

	cout << "add\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs[registers[get<2>(instruction)]];
	int b = int_regs[registers[get<3>(instruction)]];
	int c = a + b;
	int_regs[registers[get<1>(instruction)]] = c;
	PC += 4;
	cout << "modified registers : " << get<1>(instruction) << " = " << c << endl;
	print_registers();
	return 1;
}


int execute_sub(tuple<string, string, string, string> instruction) {

	if(unsafe_registers.find(get<1>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<2>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<3>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}
	
	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;

	cout << "sub\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs[registers[get<2>(instruction)]];
	int b = int_regs[registers[get<3>(instruction)]];
	int c = a - b;
	int_regs[registers[get<1>(instruction)]] = c;
	PC += 4;
	cout << "modified registers : " << get<1>(instruction) << " = " << c << endl;
	print_registers();
	return 1;
}


int execute_mul(tuple<string, string, string, string> instruction) {

	if(unsafe_registers.find(get<1>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<2>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<3>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;

	cout << "mul\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs[registers[get<2>(instruction)]];
	int b;
	if(get<3>(instruction).substr(0,1) == "$"){
		b = int_regs[registers[get<3>(instruction)]];
	}else{
		b = stoi(get<3>(instruction));
	}

	int c = a * b;
	int_regs[registers[get<1>(instruction)]] = c;
	cout << "modified registers : " << get<1>(instruction) << " = " << c << endl;
	PC += 4;
	print_registers();
	return 1;
}


int execute_beq(tuple<string, string, string, string> instruction) {

	if(unsafe_registers.find(get<1>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<2>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;

	cout << "beq\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs[registers[get<1>(instruction)]];
	int b = int_regs[registers[get<2>(instruction)]];
	if (a == b) {
		if(labels.find(get<3>(instruction)) == labels.end()){
			cout << "Referenced label does not exist" << endl;
			error = 1;
			return 0;
		}
		int mem = labels[get<3>(instruction)];
		PC = mem;
	}
	else {
		PC += 4;
	}
	print_registers();
	return 1;
}


int execute_bne(tuple<string, string, string, string> instruction) {

	if(unsafe_registers.find(get<1>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<2>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;

	cout << "bne\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs[registers[get<1>(instruction)]];
	int b = int_regs[registers[get<2>(instruction)]];
	if (a != b) {
		if(labels.find(get<3>(instruction)) == labels.end()){
			cout << "Referenced label does not exist" << endl;
			error = 1;
			return 0;
		}
		int mem = labels[get<3>(instruction)];
		PC = mem;
	}
	else {
		PC += 4;
	}
	print_registers();
	return 1;
}


int execute_slt(tuple<string, string, string, string> instruction) {

	if(unsafe_registers.find(get<1>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<2>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<3>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;

	cout << "slt\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs[registers[get<2>(instruction)]];
	int b = int_regs[registers[get<3>(instruction)]];
	if (a < b) {
		int_regs[registers[get<1>(instruction)]] = 1;
	}
	else {
		int_regs[registers[get<1>(instruction)]] = 0;
	}
	PC += 4;
	cout << "modified registers : " << get<1>(instruction) << " = " << int_regs[registers[get<1>(instruction)]] << endl;
	print_registers();
	return 1;
}


int execute_addi(tuple<string, string, string, string> instruction) {

	if(unsafe_registers.find(get<1>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	if(unsafe_registers.find(get<2>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;

	cout << "addi\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs[registers[get<2>(instruction)]];
	int b = stoi(get<3>(instruction));
	int c = a + b;
	int_regs[registers[get<1>(instruction)]] = c;
	PC += 4;
	cout << "modified registers : " << get<1>(instruction) << " = " << c << endl;
	print_registers();
	return 1;
}


int execute_lw(tuple<string, string, string, string> instruction) {

	int address;

	if (get<3>(instruction) == "") {
		address = stoi(get<2>(instruction));
	}
	else {
		int a = int_regs[registers[get<3>(instruction)]];
		if(unsafe_registers.find(get<3>(instruction)) != unsafe_registers.end()){
			cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
			clock_cycles += 1;
			return 0;
		}
		int b;
		if(get<2>(instruction) == ""){
			b = 0;
		}else{
			b = stoi(get<2>(instruction));
		}
		address = a + b;
	}

	if (address < instructions_end_index) {
		cout << "Trying to load register value from instruction memory" << endl;
		error = 1;
		PC += 4;
		return 0;
	}
	if(address%4 != 0){
		cout << "Trying to load register value from invalid memory address" << endl;
		error = 1;
		PC += 4;
		return 0;
	}

	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;
	cout << "lw\t" << get<1>(instruction) << ", " << address  << endl;
	cout << "DRAM request issued" << endl;
	tuple<string, string, int, int, int> DRAM_queue_element = make_tuple("lw", get<1>(instruction), address/1024, address%1024, clock_cycles);
	DRAM_enqueue(DRAM_queue_element);
	unsafe_registers.insert(get<1>(instruction));
	PC += 4;
	print_registers();
	return 1;
}



int execute_sw(tuple<string, string, string, string> instruction) {
	int address;
	if (get<3>(instruction) == "") {
		address = stoi(get<2>(instruction));
	}
	else {
		int a = int_regs[registers[get<3>(instruction)]];

		if(unsafe_registers.find(get<3>(instruction)) != unsafe_registers.end()){
			cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
			clock_cycles += 1;
			return 0;
		}

		int b;
		if(get<2>(instruction) == ""){
			b = 0;
		}else{
			b = stoi(get<2>(instruction));
		}
		address = a + b;
	}
	if (address < instructions_end_index) {
		cout << "Trying to load register value from instruction memory" << endl;
		error = 1;
		PC += 4;
		return 0;
	}
	if(address%4 != 0){
		cout << "Trying to load register value from invalid memory address" << endl;
		error = 1;
		PC += 4;
		return 0;
	}

	if(unsafe_registers.find(get<1>(instruction)) != unsafe_registers.end()){
		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		clock_cycles += 1;
		return 0;
	}

	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	clock_cycles += 1;
	cout << "sw\t" << get<1>(instruction) << ", " << address  << endl;
	cout << "DRAM request issued" << endl;
	tuple<string, string, int, int, int> DRAM_queue_element = make_tuple("sw", to_string(int_regs[registers[get<1>(instruction)]]), address/1024, address%1024, clock_cycles);
	DRAM_enqueue(DRAM_queue_element);
	PC += 4;
	print_registers();
	return 1;
}


int execute_j(tuple<string, string, string, string> instruction) {
	cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
	cout << "j\t" << get<1>(instruction) << endl;
	if(labels.find(get<1>(instruction)) == labels.end()){
		cout << "Referenced label does not exist" << endl;
		error = 1;
		return 0;
	}
	int mem = labels[get<1>(instruction)];
	PC = mem;
	clock_cycles += 1;
	print_registers();
	return 1;
}

int execute_DRAM(){
	if(DRAM_queue.size() == 0){
		return 0;
	}else{
		tuple<string, string, int, int, int> DRAM_queue_element = DRAM_queue.at(0);
		if(row_in_buffer == -1){
			if(clock_cycles - get<4>(DRAM_queue_element) == RAD-1){
				cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
				cout << "row buffer updated to row " << get<2>(DRAM_queue_element)+1 << " of the memory" << endl;
				row_buffer_updates += 1;
				print_registers();
				row_in_buffer = get<2>(DRAM_queue_element);
				clock_cycles += 1;
				tuple<string, string, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(DRAM_queue_element), get<1>(DRAM_queue_element), get<2>(DRAM_queue_element), get<3>(DRAM_queue_element), clock_cycles);
				DRAM_queue.at(0) = new_DRAM_queue_element;
				return 1;
			}else{
				return 0;
			}
		}else if(row_in_buffer != get<2>(DRAM_queue_element)){
			if(clock_cycles - get<4>(DRAM_queue_element) == RAD-1){
				cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
				cout << "row buffer has been copied back to row " << row_in_buffer+1 << " of the memory" << endl;
				print_registers();
				row_in_buffer = -1;
				clock_cycles += 1;
				tuple<string, string, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(DRAM_queue_element), get<1>(DRAM_queue_element), get<2>(DRAM_queue_element), get<3>(DRAM_queue_element), clock_cycles);
				DRAM_queue.at(0) = new_DRAM_queue_element;
				return 1;
			}else{
				return 0;
			}
		}else{
			if(clock_cycles - get<4>(DRAM_queue_element) == CAD-1){
				if(get<0>(DRAM_queue_element) == "lw"){
					int_regs[registers[get<1>(DRAM_queue_element)]] = memory[get<2>(DRAM_queue_element)][get<3>(DRAM_queue_element)/4];
					cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
					cout << "modified register " << get<1>(DRAM_queue_element) << " = " << int_regs[registers[get<1>(DRAM_queue_element)]] << endl;
					print_registers();
					clock_cycles += 1;
					unsafe_registers.erase(get<1>(DRAM_queue_element));
					DRAM_queue.erase(DRAM_queue.begin());
					if(DRAM_queue.size() != 0){
						DRAM_queue_element = DRAM_queue.at(0);
						tuple<string, string, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(DRAM_queue_element), get<1>(DRAM_queue_element), get<2>(DRAM_queue_element), get<3>(DRAM_queue_element), clock_cycles);
						DRAM_queue.at(0) = new_DRAM_queue_element;
					}
				}else{
					memory[get<2>(DRAM_queue_element)][get<3>(DRAM_queue_element)/4] = stoi(get<1>(DRAM_queue_element));
					int address = get<2>(DRAM_queue_element)*1024 + get<3>(DRAM_queue_element);
					cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
					cout << "modified memory address " << address << " - " << address + 3 << " = " << stoi(get<1>(DRAM_queue_element)) << endl;
					print_registers();
					clock_cycles += 1;
					tuple<string, string, int, int, int> new_DRAM_queue_element;
					DRAM_queue.erase(DRAM_queue.begin());
					if(DRAM_queue.size() != 0){
						DRAM_queue_element = DRAM_queue.at(0);
						tuple<string, string, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(DRAM_queue_element), get<1>(DRAM_queue_element), get<2>(DRAM_queue_element), get<3>(DRAM_queue_element), clock_cycles);
						DRAM_queue.at(0) = new_DRAM_queue_element;
					}
				}
				return 1;
			}else{
				return 0;
			}
		}
	}

}

void execute(vector<tuple<string, string, string, string>> instructions, int* executions){

	while(PC < instructions_end_index || DRAM_queue.size() != 0){

		/*cout << "unsafe registers : ";
		for (auto it = unsafe_registers.begin(); it != unsafe_registers.end(); it++)
	        cout << *it << " ";
	    cout << endl;
	*/
		tuple<string, string, string, string> instruction;

		if(PC < instructions_end_index){

			instruction = instructions.at(PC/4);
		
		}
		

		int DRAM_execution = execute_DRAM();

		if(DRAM_execution == 0){

			if(get<0>(instruction) == "add"){
				executions[PC/4] += execute_add(instruction);
				if(error == 1){
					return;
				}
			}

			else if(get<0>(instruction) == "sub"){
				executions[PC/4] += execute_sub(instruction);
				if(error == 1){
					return;
				}
			}
			
			else if(get<0>(instruction) == "mul"){
				executions[PC/4] += execute_mul(instruction);
				if(error == 1){
					return;
				}
			}

			else if(get<0>(instruction) == "beq"){
				executions[PC/4] += execute_beq(instruction);
				if(error == 1){
					return;
				}
			}

			else if(get<0>(instruction) == "bne"){
				executions[PC/4] += execute_bne(instruction);
				if(error == 1){
					return;
				}
			}

			else if(get<0>(instruction) == "j"){
				executions[PC/4] += execute_j(instruction);
				if(error == 1){
					return;
				}
			}

			else if(get<0>(instruction) == "lw"){
				executions[PC/4] += execute_lw(instruction);
				if(error == 1){
					return;
				}
			}

			else if(get<0>(instruction) == "sw"){
				executions[PC/4] += execute_sw(instruction);
				if(error == 1){
					return;
				}
			}

			else if(get<0>(instruction) == "slt"){
				executions[PC/4] += execute_slt(instruction);
				if(error == 1){
					return;
				}
			}

			else if(get<0>(instruction) == "addi"){
				executions[PC/4] += execute_addi(instruction);
				if(error == 1){
					return;
				}
			}

			else{
				if(PC < instructions_end_index){
					PC += 4;
					executions[PC/4] += 1;
				}else{
					cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
					clock_cycles += 1;
				}
			}
		}
	}

	return;
}

int main(int argc, char* argv[]){

	registers["$zero"] = 0;
	registers["$at"] = 1;
	registers["$v0"] = 2;
	registers["$v1"] = 3;
	registers["$a0"] = 4;
	registers["$a1"] = 5;
	registers["$a2"] = 6;
	registers["$a3"] = 7;
	registers["$t0"] = 8;
	registers["$t1"] = 9;
	registers["$t2"] = 10;
	registers["$t3"] = 11;
	registers["$t4"] = 12;
	registers["$t5"] = 13;
	registers["$t6"] = 14;
	registers["$t7"] = 15;
	registers["$s0"] = 16;
	registers["$s1"] = 17;
	registers["$s2"] = 18;
	registers["$s3"] = 19;
	registers["$s4"] = 20;
	registers["$s5"] = 21;
	registers["$s6"] = 22;
	registers["$s7"] = 23;
	registers["$t8"] = 24;
	registers["$t9"] = 25;
	registers["$k0"] = 26;
	registers["$k1"] = 27;
	registers["$gp"] = 28;
	registers["$sp"] = 29;
	registers["$fp"] = 30;
	registers["$ra"] = 31;

	int_regs[29] = 262140;

	for (int i = 0; i < 32; i++)
	{
		hex_regs[i] = "0000";
	}

	RAD = stoi(argv[2]);
	CAD = stoi(argv[3]);

	load_file(argv[1]);

	if(error == 1){
		cout << load_error << endl;
		return -1;
	}

	if(instructions_end_index == 0){
		cout << "empty file" << endl;
		return -1;
	}

	int executions[instructions_end_index/4] = {0};

	executions[PC/4] += 1;

	execute(instructions, executions);

	cout << "clock_cycles : " << clock_cycles << endl;
	cout << "row buffer was updated " << row_buffer_updates << " times" << endl;

	print_instructions(executions);

	return 0; 

}
