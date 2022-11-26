#include <stdio.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <set>

using namespace std;

vector<vector<tuple<string, string, string, string>>> instructions;

vector<string> tokens;

int num_cores;

int RAD;

int CAD;

int MRM_occupied = 0;

int MRM_delay = 8;

tuple<int, int> location;

vector<tuple<string, string, int, int, int, int>> DRAM_queue;

tuple<string, string, int, int, int, int> MRM_request;

vector<set<string>> unsafe_registers;

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

vector<int> PC;

vector<vector<int>> int_regs;

vector<vector<int>> executions;

int memory[1024][256];

vector<unordered_map<string, int>> labels;

int clock_cycles = 0;

int mem_limit;

int DRAM_execution = 0;

int MRM_execution = 0;

int M;



string trim(string input){ 

	int start = input.find_first_not_of(", \t");

	if(start == -1){
		return "";
	}else{
		return input.substr(start);
	}
}

void tokenize(string line){

	line = trim(line);

	if(line == ""){
		return;
	}
	else
	{
		string token;
		
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

		if(first == line.size()){
			token = line;
			line = "";
			tokens.push_back(token);
			tokenize(line);
			return;
		}else{
			token = line.substr(0, first);
			line = line.substr(first+1);
			line = trim(line);
			tokens.push_back(token);
			tokenize(line);
			return;
		}
	}
}


void load_label(vector<string> tokens, int jump_location, int core_num){

	tuple<string, string, string, string> instruction;

	string token;
	token = tokens.at(0);

	if(tokens.size() == 1){
		if(labels.at(core_num-1).find(token.substr(0,tokens.at(0).size()-1)) != labels.at(core_num-1).end()){
			cout << "cannot define same label twice" << endl;
			error = 1;
			return;
		}
		instruction = make_tuple(token.substr(0,tokens.at(0).size()-1), "", "", "");
		instructions.at(core_num-1).push_back(instruction);
		labels.at(core_num-1)[token.substr(0,tokens.at(0).size()-1)] = jump_location;
		return;
	}
	if(tokens.at(1).substr(0,1) == "#"){
		instruction = make_tuple(token.substr(0,tokens.at(0).size()-1), "", "", "");
		instructions.at(core_num-1).push_back(instruction);
		labels.at(core_num-1)[token.substr(0,tokens.at(0).size()-1)] = jump_location;
		return;	
	}
	load_error = "label should not be followed by any operand in the same line, it can only be followed by a comment in the same line";
	return;
}

void load_add(vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_sub(vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_mul(vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_beq(std::vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_bne(std::vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_slt(vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_j(vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_lw(vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_sw(vector<string> tokens, int core_num){

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

	instructions.at(core_num-1).push_back(instruction);
	return;
}

void load_addi(vector<string> tokens, int core_num){

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
	instructions.at(core_num-1).push_back(instruction);
	return;	
}


void load_file(string infilename, int core_num){

	ifstream infile(infilename);

	string line;
	int linenum; linenum = 0;

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

		if(token1.substr(0,1) == "#"){
			continue;
		}

		if(token1.substr(token1.size()-1) == ":"){
			load_label(tokens, instructions.at(core_num-1).size(), core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "add"){
			load_add(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "sub"){
			load_sub(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "mul"){
			load_mul(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "beq"){
			load_beq(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "bne"){
			load_bne(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "slt"){
			load_slt(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "addi"){
			load_addi(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "lw"){
			load_lw(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "sw"){
			load_sw(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		if(token1 == "j"){
			load_j(tokens, core_num);
			if(load_error != ""){
				cerr << "CORE " << core_num << " load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			continue;
		}

		cerr << "load error: " << linenum << ": the first token is neither a label, a comment nor a valid operation" << endl;
		error = 1;
		return;
	}
	return;
}

string int_to_hex(int reg_val){

	char hex_string[20];

	sprintf(hex_string, "%X", reg_val);

	return hex_string;
}

void print_registers(int core_num){

	char hex_string[20];

	for(int i = 0; i<31; i++){

		sprintf(hex_string, "%X", int_regs.at(core_num - 1).at(i));
		cout << hex_string << " ";
	}

	sprintf(hex_string, "%X", int_regs.at(core_num - 1).at(31));
	cout << hex_string << endl;

	return;

}

void print_instructions(){

	for(int c = 1; c <= num_cores; c++){
		cout << "Execution of instructions for CORE " << c << endl;
		for(int i = 0; i<instructions.at(c-1).size() ; i++){
			tuple<string, string, string, string> instruction = instructions.at(c-1).at(i);
			cout << get<0>(instruction) << "\t" << get<1>(instruction) << "\t" << get<2>(instruction) << "\t" << get<3>(instruction) << "\t" << executions.at(c-1).at(i) << endl;
		}
	}
	return;

}

void DRAM_enqueue(tuple<string, string, int, int, int, int> DRAM_queue_element){
	MRM_occupied = 1;
	MRM_request = DRAM_queue_element;
	return;
}

int execute_MRM(){

	if (MRM_request == make_tuple("", "", -1, -1, -1, -1)){
		return 0;
	}else{
		if(clock_cycles - get<4>(MRM_request) == MRM_delay){
			if(DRAM_queue.size() == 0){
				tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(MRM_request), get<1>(MRM_request), get<2>(MRM_request), get<3>(MRM_request), clock_cycles, get<5>(MRM_request));
				DRAM_queue.push_back(new_DRAM_queue_element);
				MRM_occupied = 0;
				return 0;
			}

			if(get<0>(MRM_request) == "lw"){
				
				tuple<string, string, int, int, int, int> curr_DRAM_queue_element;
				int l;

				for(l = DRAM_queue.size(); l>0; l--){

					curr_DRAM_queue_element = DRAM_queue.at(l-1);
					
					if(get<5>(curr_DRAM_queue_element) != get<5>(MRM_request)){
						continue;
					}

					if(get<0>(curr_DRAM_queue_element) == "lw"){
						if(get<1>(curr_DRAM_queue_element) == get<1>(MRM_request)){
							DRAM_queue.erase(DRAM_queue.begin()+l-1);
							if(DRAM_queue.size() != 0){
								if(l ==1){
									curr_DRAM_queue_element = DRAM_queue.at(0);
									tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(curr_DRAM_queue_element), get<1>(curr_DRAM_queue_element), get<2>(curr_DRAM_queue_element), get<3>(curr_DRAM_queue_element), clock_cycles, get<5>(curr_DRAM_queue_element));
									DRAM_queue.at(0) = new_DRAM_queue_element;
								}
							}
						}
					}
				}

				for(l = DRAM_queue.size(); l>0; l--){

					curr_DRAM_queue_element = DRAM_queue.at(l-1);

					if(get<5>(curr_DRAM_queue_element) != get<5>(MRM_request)){
						continue;
					}

					if(get<0>(curr_DRAM_queue_element) == "sw"){
						if(get<2>(curr_DRAM_queue_element) == get<2>(MRM_request) && get<3>(curr_DRAM_queue_element) == get<3>(MRM_request)){
							if(DRAM_execution == get<5>(MRM_request)){
								tuple<string, string, int, int, int, int> new_MRM_request = make_tuple(get<0>(MRM_request), get<1>(MRM_request), get<2>(MRM_request), get<3>(MRM_request), get<4>(MRM_request) + 1, get<5>(MRM_request));
								MRM_request = new_MRM_request;
								return 0;
							}else{
								int CN = get<5>(MRM_request);
								cout << "CORE " << CN << endl;
								int_regs.at(CN - 1).at(registers[get<1>(MRM_request)]) = stoi(get<1>(curr_DRAM_queue_element));
								cout << "modified register " << get<1>(MRM_request) << " = " << int_regs.at(CN - 1).at(registers[get<1>(MRM_request)]) << endl;
								unsafe_registers.at(CN - 1).erase(get<1>(MRM_request));
								MRM_request = make_tuple("", "", -1, -1, -1, -1);
								MRM_occupied = 0;
								return CN;
							}
						}
					}
				}

				for(l = 1; l<=DRAM_queue.size(); l++){
					
					if(get<5>(curr_DRAM_queue_element) != get<5>(MRM_request)){
						continue;
					}

					if(get<2>(curr_DRAM_queue_element) == get<2>(MRM_request)){
						tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(MRM_request), get<1>(MRM_request), get<2>(MRM_request), get<3>(MRM_request), clock_cycles, get<5>(MRM_request));
						DRAM_queue.insert(DRAM_queue.begin()+l, new_DRAM_queue_element);
						MRM_occupied = 0;
						return 0;
					}
				}

				DRAM_queue.push_back(MRM_request);
				MRM_occupied = 0;
				return 0;
			}else{

				tuple<string, string, int, int, int, int> curr_DRAM_queue_element;
				int l;

				for(l = DRAM_queue.size(); l>0; l--){

					curr_DRAM_queue_element = DRAM_queue.at(l-1);

					if(get<5>(curr_DRAM_queue_element) != get<5>(MRM_request)){
						continue;
					}

					if(get<2>(curr_DRAM_queue_element) == get<2>(MRM_request) && get<3>(curr_DRAM_queue_element) == get<3>(MRM_request)){
						if(get<0>(curr_DRAM_queue_element) == "lw"){
							break;
						}else{
							DRAM_queue.erase(DRAM_queue.begin()+l-1);
							if(DRAM_queue.size() != 0){
								if(l == 1){
									curr_DRAM_queue_element = DRAM_queue.at(0);
									tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(curr_DRAM_queue_element), get<1>(curr_DRAM_queue_element), get<2>(curr_DRAM_queue_element), get<3>(curr_DRAM_queue_element), clock_cycles, get<5>(curr_DRAM_queue_element));
									DRAM_queue.at(0) = new_DRAM_queue_element;
								}
							}
						}
					}
				}

				if(l == 0){
					l = 1;
				}

				for(l; l<=DRAM_queue.size(); l++){
					
					curr_DRAM_queue_element = DRAM_queue.at(l-1);

					if(get<5>(curr_DRAM_queue_element) != get<5>(MRM_request)){
						continue;
					}

					if(get<2>(curr_DRAM_queue_element) == get<2>(MRM_request)){
						tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(MRM_request), get<1>(MRM_request), get<2>(MRM_request), get<3>(MRM_request), clock_cycles, get<5>(MRM_request));
						DRAM_queue.insert(DRAM_queue.begin()+l, new_DRAM_queue_element);
						MRM_occupied = 0;
						return 0;
					}
				}

				DRAM_queue.push_back(MRM_request);
				MRM_occupied = 0;
				return 0;
			}
		}else{
			return 0;
		}
	}
}

int execute_DRAM(){
	if(DRAM_queue.size() == 0){
		return 0;
	}else{
		tuple<string, string, int, int, int, int> DRAM_queue_element = DRAM_queue.at(0);
		cout << "start clock_cycle : " << get<4>(DRAM_queue_element) << endl;
		if(row_in_buffer == -1){
			if(clock_cycles - get<4>(DRAM_queue_element) == RAD){
				cout << "row buffer updated to row " << get<2>(DRAM_queue_element)+1 << " of the memory" << endl;
				row_buffer_updates += 1;
				row_in_buffer = get<2>(DRAM_queue_element);
				tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(DRAM_queue_element), get<1>(DRAM_queue_element), get<2>(DRAM_queue_element), get<3>(DRAM_queue_element), clock_cycles, get<5>(DRAM_queue_element));
				DRAM_queue.at(0) = new_DRAM_queue_element;
				return 0;
			}else{
				return 0;
			}
		}else if(row_in_buffer != get<2>(DRAM_queue_element)){
			if(clock_cycles - get<4>(DRAM_queue_element) == RAD){
				cout << "row buffer has been copied back to row " << row_in_buffer+1 << " of the memory" << endl;
				row_in_buffer = -1;
				tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(DRAM_queue_element), get<1>(DRAM_queue_element), get<2>(DRAM_queue_element), get<3>(DRAM_queue_element), clock_cycles, get<5>(DRAM_queue_element));
				DRAM_queue.at(0) = new_DRAM_queue_element;
				return 0;
			}else{
				return 0;
			}
		}else{
			if(clock_cycles - get<4>(DRAM_queue_element) == CAD){
				int CN = get<5>(DRAM_queue_element);
				if(get<0>(DRAM_queue_element) == "lw"){
					int_regs.at(CN - 1).at(registers[get<1>(DRAM_queue_element)]) = memory[get<2>(DRAM_queue_element)][get<3>(DRAM_queue_element)/4];
					cout << "CORE " << CN << endl;
					cout << "modified register " << get<1>(DRAM_queue_element) << " = " << int_regs.at(CN - 1).at(registers[get<1>(DRAM_queue_element)]) << endl;
					print_registers(CN);
					unsafe_registers.at(CN - 1).erase(get<1>(DRAM_queue_element));
					DRAM_queue.erase(DRAM_queue.begin());
					if(DRAM_queue.size() != 0){
						DRAM_queue_element = DRAM_queue.at(0);
						tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(DRAM_queue_element), get<1>(DRAM_queue_element), get<2>(DRAM_queue_element), get<3>(DRAM_queue_element), clock_cycles, get<5>(DRAM_queue_element));
						DRAM_queue.at(0) = new_DRAM_queue_element;
					}
				}else{
					memory[get<2>(DRAM_queue_element)][get<3>(DRAM_queue_element)/4] = stoi(get<1>(DRAM_queue_element));
					int address = get<2>(DRAM_queue_element)*1024 + get<3>(DRAM_queue_element);
					cout << "modified memory address " << address << " - " << address + 3 << " = " << stoi(get<1>(DRAM_queue_element)) << endl;
					print_registers(CN);
					DRAM_queue.erase(DRAM_queue.begin());
					if(DRAM_queue.size() != 0){
						DRAM_queue_element = DRAM_queue.at(0);
						tuple<string, string, int, int, int, int> new_DRAM_queue_element = make_tuple(get<0>(DRAM_queue_element), get<1>(DRAM_queue_element), get<2>(DRAM_queue_element), get<3>(DRAM_queue_element), clock_cycles, get<5>(DRAM_queue_element));
						DRAM_queue.at(0) = new_DRAM_queue_element;
					}
				}
				return CN;
			}else{
				return 0;
			}
		}
	}
}

int execute_add(tuple<string, string, string, string> instruction, int core_num) {

	if(unsafe_registers.at(core_num - 1).find(get<1>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "add\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<1>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<2>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "add\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<2>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<3>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "add\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<3>(instruction) <<" to become safe" << endl;
		return 0;
	}

	cout << "CORE " << core_num << endl;

	cout << "add\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	
	int a = int_regs.at(core_num - 1).at(registers[get<2>(instruction)]);
	int b = int_regs.at(core_num - 1).at(registers[get<3>(instruction)]);
	int c = a + b;
	
	int_regs.at(core_num - 1).at(registers[get<1>(instruction)]) = c;
	PC.at(core_num - 1) += 1;
	cout << "modified registers : " << get<1>(instruction) << " = " << c << endl;
	print_registers(core_num);
	return 1;
}

int execute_sub(tuple<string, string, string, string> instruction, int core_num) {

	if(unsafe_registers.at(core_num - 1).find(get<1>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "sub\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<1>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<2>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "sub\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<2>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<3>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "sub\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<3>(instruction) <<" to become safe" << endl;
		return 0;
	}
	
	cout << "CORE " << core_num << endl;

	cout << "sub\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	
	int a = int_regs.at(core_num - 1).at(registers[get<2>(instruction)]);
	int b = int_regs.at(core_num - 1).at(registers[get<3>(instruction)]);
	int c = a - b;
	
	int_regs.at(core_num - 1).at(registers[get<1>(instruction)]) = c;
	PC.at(core_num - 1) += 1;
	cout << "modified registers : " << get<1>(instruction) << " = " << c << endl;
	print_registers(core_num);
	return 1;
}

int execute_mul(tuple<string, string, string, string> instruction, int core_num) {

	if(unsafe_registers.at(core_num - 1).find(get<1>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "mul\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<1>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<2>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "mul\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<2>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<3>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "mul\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<3>(instruction) <<" to become safe" << endl;
		return 0;
	}

	cout << "CORE " << core_num << endl;

	cout << "mul\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	
	int a = int_regs.at(core_num - 1).at(registers[get<2>(instruction)]);
	int b;
	if(get<3>(instruction).substr(0,1) == "$"){
		b = int_regs.at(core_num - 1).at(registers[get<3>(instruction)]);
	}else{
		b = stoi(get<3>(instruction));
	}

	int c = a * b;
	int_regs.at(core_num - 1).at(registers[get<1>(instruction)]) = c;
	cout << "modified registers : " << get<1>(instruction) << " = " << c << endl;
	PC.at(core_num - 1) += 1;
	print_registers(core_num);
	return 1;
}

int execute_beq(tuple<string, string, string, string> instruction, int core_num) {

	if(unsafe_registers.at(core_num - 1).find(get<1>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "beq\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<1>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<2>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "beq\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<2>(instruction) <<" to become safe" << endl;
		return 0;
	}

	cout << "CORE " << core_num << endl;

	cout << "beq\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs.at(core_num - 1).at(registers[get<1>(instruction)]);
	int b = int_regs.at(core_num - 1).at(registers[get<2>(instruction)]);
	if (a == b) {
		if(labels.at(core_num-1).find(get<3>(instruction)) == labels.at(core_num - 1).end()){
			cout << "Referenced label does not exist" << endl;
			error = 1;
			return 0;
		}
		int jump_location = labels.at(core_num-1)[get<3>(instruction)];
		PC.at(core_num - 1) = jump_location;
	}
	else {
		PC.at(core_num - 1) += 1;
	}
	print_registers(core_num);
	return 1;
}

int execute_bne(tuple<string, string, string, string> instruction, int core_num) {

	if(unsafe_registers.at(core_num - 1).find(get<1>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "bne\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<1>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<2>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "bne\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<2>(instruction) <<" to become safe" << endl;
		return 0;
	}

	cout << "CORE " << core_num << endl;

	cout << "bne\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs.at(core_num - 1).at(registers[get<1>(instruction)]);
	int b = int_regs.at(core_num - 1).at(registers[get<2>(instruction)]);
	if (a != b) {
		if(labels.at(core_num-1).find(get<3>(instruction)) == labels.at(core_num - 1).end()){
			cout << "Referenced label does not exist" << endl;
			error = 1;
			return 0;
		}
		int jump_location = labels.at(core_num-1)[get<3>(instruction)];
		PC.at(core_num - 1) = jump_location;
	}
	else {
		PC.at(core_num - 1) += 1;
	}
	print_registers(core_num);
	return 1;
}

int execute_slt(tuple<string, string, string, string> instruction, int core_num) {

	if(unsafe_registers.at(core_num - 1).find(get<1>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "slt\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<1>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<2>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "slt\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<2>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<3>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "slt\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<3>(instruction) <<" to become safe" << endl;
		return 0;
	}

	cout << "CORE " << core_num << endl;

	cout << "slt\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	
	int a = int_regs.at(core_num - 1).at(registers[get<2>(instruction)]);
	int b = int_regs.at(core_num - 1).at(registers[get<3>(instruction)]);
	if (a < b) {
		int_regs.at(core_num - 1).at(registers[get<1>(instruction)]) = 1;
	}
	else {
		int_regs.at(core_num - 1).at(registers[get<1>(instruction)]) = 0;
	}
	PC.at(core_num - 1) += 1;
	cout << "modified registers : " << get<1>(instruction) << " = " << int_regs.at(core_num - 1).at(registers[get<2>(instruction)]) << endl;
	print_registers(core_num);
	return 1;
}

int execute_addi(tuple<string, string, string, string> instruction, int core_num) {

	if(unsafe_registers.at(core_num -1).find(get<1>(instruction)) != unsafe_registers.at(core_num -1).end()){
		cout << "CORE " << core_num << endl;
		cout << "addi\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<1>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if(unsafe_registers.at(core_num -1).find(get<2>(instruction)) != unsafe_registers.at(core_num -1).end()){
		cout << "CORE " << core_num << endl;
		cout << "addi\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
		cout << "waiting for register "<< get<2>(instruction) <<" to become safe" << endl;
		return 0;
	}

	cout << "CORE " << core_num << endl;

	cout << "addi\t" << get<1>(instruction) << ", " << get<2>(instruction) << ", " << get<3>(instruction) << endl;
	int a = int_regs.at(core_num - 1).at(registers[get<2>(instruction)]);
	int b = stoi(get<3>(instruction));
	int c = a + b;
	int_regs.at(core_num - 1).at(registers[get<1>(instruction)]) = c;
	PC.at(core_num - 1) += 1;
	cout << "modified registers : " << get<1>(instruction) << " = " << c << endl;
	print_registers(core_num);
	return 1;
}

int execute_lw(tuple<string, string, string, string> instruction, int core_num) {

	int address;

	if(unsafe_registers.at(core_num -1).find(get<3>(instruction)) != unsafe_registers.at(core_num -1).end()){
		cout << "CORE " << core_num << endl;
		cout << "lw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "waiting for register "<< get<3>(instruction) <<" to become safe" << endl;
		return 0;
	}

	int a = int_regs.at(core_num - 1).at(registers[get<3>(instruction)]);
	
	int b;

	if(get<2>(instruction) == ""){
		b = 0;
	}else{
		b = stoi(get<2>(instruction));
	}

	address = a + b + mem_limit*(core_num - 1);

	if ((a + b) > mem_limit - 4){
		cout << "CORE " << core_num << endl;
		cout << "lw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "Trying to access memory address outside memory size limit for a core" << endl;
		error = 1;
		PC.at(core_num - 1) += 1;
		return 0;
	}

	if(address%4 != 0){
		cout << "CORE " << core_num << endl;
		cout << "lw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "Trying to load register value from invalid memory address" << endl;
		error = 1;
		PC.at(core_num - 1) += 1;
		return 0;
	}

	if (MRM_occupied == 1){
		cout << "CORE " << core_num << endl;
		cout << "lw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "Memory request manager is currently handling another request, hence a request cannot be raised" << endl;
		return 0;
	}

	if (DRAM_queue.size() == 32){
		cout << "CORE " << core_num << endl;
		cout << "lw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "The DRAM_queue is currently full so a request cannot be raised" << endl;
		return 0;
	}

	cout << "CORE " << core_num << endl;
	cout << "lw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
	cout << "DRAM request issued" << endl;
	
	tuple<string, string, int, int, int, int> DRAM_queue_element = make_tuple("lw", get<1>(instruction), address/1024, address%1024, clock_cycles, core_num);
	DRAM_enqueue(DRAM_queue_element);
	unsafe_registers.at(core_num - 1).insert(get<1>(instruction));
	PC.at(core_num - 1) += 1;
	print_registers(core_num);
	return 1;
}

int execute_sw(tuple<string, string, string, string> instruction, int core_num) {
	
	int address;
	
	if(unsafe_registers.at(core_num - 1).find(get<3>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "sw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "waiting for register "<< get<3>(instruction) <<" to become safe" << endl;
		return 0;
	}

	int a = int_regs.at(core_num - 1).at(registers[get<3>(instruction)]);

	int b;
	if(get<2>(instruction) == ""){
		b = 0;
	}else{
		b = stoi(get<2>(instruction));
	}
	address = a + b + mem_limit*(core_num - 1);

	cout << address << endl;

	if(address%4 != 0){
		cout << "CORE " << core_num << endl;
		cout << "sw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "Trying to load register value from invalid memory address" << endl;
		error = 1;
		PC.at(core_num - 1) += 1;
		return 0;
	}

	if(unsafe_registers.at(core_num - 1).find(get<1>(instruction)) != unsafe_registers.at(core_num - 1).end()){
		cout << "CORE " << core_num << endl;
		cout << "sw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "waiting for register "<< get<1>(instruction) <<" to become safe" << endl;
		return 0;
	}

	if (MRM_occupied == 1){
		cout << "CORE " << core_num << endl;
		cout << "sw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "Memory request manager is currently handling another request, hence a request cannot be raised" << endl;
		return 0;
	}

	if (DRAM_queue.size() == 32){
		cout << "CORE " << core_num << endl;
		cout << "sw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
		cout << "The DRAM_queue is currently full so a request cannot be raised" << endl;
		return 0;
	}

	cout << "CORE " << core_num << endl;
	cout << "sw\t" << get<1>(instruction) << ", " << get<2>(instruction) << "(" << get<3>(instruction) << ")"  << endl;
	cout << "DRAM request issued" << endl;

	tuple<string, string, int, int, int, int> DRAM_queue_element = make_tuple("sw", to_string(int_regs.at(core_num - 1).at(registers[get<1>(instruction)])), address/1024, address%1024, clock_cycles, core_num);
	DRAM_enqueue(DRAM_queue_element);
	PC.at(core_num - 1) += 1;
	print_registers(core_num);
	return 1;
}

int execute_j(tuple<string, string, string, string> instruction, int core_num) {
	cout << "CORE " << core_num << endl;
	cout << "j\t" << get<1>(instruction) << endl;
	if(labels.at(core_num - 1).find(get<1>(instruction)) == labels.at(core_num - 1).end()){
		cout << "Referenced label does not exist" << endl;
		error = 1;
		return 0;
	}
	int jump_location = labels.at(core_num - 1)[get<1>(instruction)];
	PC.at(core_num - 1) = jump_location;
	print_registers(core_num);
	return 1;
}


int main(int argc, char* argv[]){

	/*
		the input arguments must be:
		<exec_name> <number of programs to run simultaneously(num_cores)> <program1> <program2> ..... <program 'num_cores'><RAD> <CAD> <M>
	*/
	num_cores = stoi(argv[1]);

	if(num_cores == 0){
		cout << "no program files were provided" << endl;
		return -1;
	}

	if(argc != num_cores+5){
		cout << "the input arguments must be: <exec_name> <number of programs to run simultaneously(num_cores)> <program1> <program2> ..... <program 'num_cores'><RAD> <CAD>"<< endl;
		return -1;
	}

	RAD = stoi(argv[num_cores+2]);
	CAD = stoi(argv[num_cores+3]);

	M = stoi(argv[num_cores+4]);

	mem_limit = 1048576/num_cores;

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

	vector<int> empty_int_regs;

	for(int i = 0; i<32; i++){
		empty_int_regs.push_back(0);
	}
	empty_int_regs.at(29) = mem_limit-4;

	vector<tuple<string, string, string, string>> emptyInstructionVector;

	vector<int> empty_execution_vec;

	unordered_map<string, int> empty_label_map;

	set<string> empty_unsafe_registers;

	for(int i = 1; i<=num_cores; i++){
		instructions.push_back(emptyInstructionVector);
		int_regs.push_back(empty_int_regs);
		PC.push_back(0);
		labels.push_back(empty_label_map);
		unsafe_registers.push_back(empty_unsafe_registers);
		load_file(argv[i+1], i);
		executions.push_back(empty_execution_vec);


		for(int j = 0; j < instructions.at(i-1).size(); j++){
			executions.at(i-1).push_back(0);
		}

		if(instructions.at(i-1).size() == 0){
			cout << "Warning : file number "<< i << " is an empty file" << endl;
		}
	}

	if(error == 1){
		cout << load_error << endl;
		return -1;
	}
	
	MRM_request = make_tuple("", "", -1, -1, -1, -1);

	int incomplete;

	incomplete = 1;

	tuple<string, string, string, string> instruction;

	int pc;

	while(DRAM_queue.size() != 0 || incomplete == 1 || MRM_occupied != 0){

		incomplete = 0;

		DRAM_execution = 0;

		MRM_execution = 0;

		cout << "clock_cycle : " << clock_cycles << "-" << clock_cycles + 1 << endl;
		cout << "DRAM_queue size : " << DRAM_queue.size() << endl;

		DRAM_execution = execute_DRAM();

		MRM_execution = execute_MRM();


		for(int i=1; i<=num_cores; i++){


			if(PC.at(i-1) < instructions.at(i-1).size()){

				instruction = instructions.at(i-1).at(PC.at(i-1));
				incomplete = 1;
				
				if(DRAM_execution == i || MRM_execution == i){
					continue;
				}

				if(DRAM_execution == 0){

					if(get<0>(instruction) == "add"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_add(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else if(get<0>(instruction) == "sub"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_sub(instruction, i);
						if(error == 1){
							return -1;
						}
					}
					
					else if(get<0>(instruction) == "mul"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_mul(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else if(get<0>(instruction) == "beq"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_beq(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else if(get<0>(instruction) == "bne"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_bne(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else if(get<0>(instruction) == "j"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_j(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else if(get<0>(instruction) == "lw"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_lw(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else if(get<0>(instruction) == "sw"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_sw(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else if(get<0>(instruction) == "slt"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_slt(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else if(get<0>(instruction) == "addi"){
						pc = PC.at(i-1);
						executions.at(i-1).at(pc) += execute_addi(instruction, i);
						if(error == 1){
							return -1;
						}
					}

					else{
						PC.at(i-1) += 1;
						executions.at(i-1).at(PC.at(i-1)-1) += 1;
						i = i-1;
					}
				}
			
			}else{

				cout << "CORE " << i << " : program counter has reached end of program" << endl;
				continue;
			}
		}

		clock_cycles += 1;
		
		if(clock_cycles >= M){
			break;
		}
	}

	cout << "Total clock_cycles : " << clock_cycles << endl;
	cout << "The row buffer was updated : " << row_buffer_updates << " times" << endl;
	print_instructions();

	int thpt = 0;

	for(int i = 1; i < num_cores; i++){
		for (int j = 0; j < instructions.at(i-1).size(); j++){
			thpt += executions.at(i-1).at(j);
		}
	}

	thpt -= DRAM_queue.size();

	if (MRM_occupied == 1){
		thpt -= 1;
	}

	float f;
	f = thpt;

	cout << "The throughput for the given program in " << clock_cycles << " cycles is " << f/(clock_cycles) << endl;

}