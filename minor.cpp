#include <stdio.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <vector>

using namespace std;

vector<tuple<string, string, string, string>> instructions;
vector<string> tokens;
int instructions_end_index;
int delay = 0;
int updates = 0;
string cur_reg = "";
string dep1 = "";
string cur_reg2 = "";
string dep2 = "";
string cur_reg3 = "";
string cur_reg4 = "";
int jump_to = -1;
unordered_map<string, int> registers;
bool previous = false;
int part = 0;
int error;
string load_error;
int PC = 0;
int int_regs[32] = {0};
int memory[1024][256];
int row_buffer[256] = {0};
int cur_row = -1;
int clock_cycles = 0;
int row_access_delay = 0;
int col_access_delay = 0;


//auxiliary function which trims leading and trailing whitespace from a string
string trim(string input){ 
	int start = input.find_first_not_of(", \t");
	if(start == -1){
		return "";
	}
	else{
		return input.substr(start);
	}
}


//auxiliary function which takes a single line of the input file as a string
//and tokenizes it while recognizing any errors
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

	if(token == "$r0" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
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


void load_lw(vector<string> tokens){

	tuple<string, string, string, string> instruction;

	if(tokens.size() < 3){
		load_error = "lw operation takes 2 operands";
		return;
	}

	string token;

	token = tokens.at(1);

	if(token.substr(0,1) == "#"){
		load_error = "mul operation takes 3 operands";
		return;
	}

	if(token == "$r0" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
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

		char* q;
		long converted = strtol(token.substr(0,token.size()-5).c_str(), &q, 10);
		if(*q){
			load_error = "lw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		if(registers.find(token.substr(token.size()-4, 3)) == registers.end()){
			load_error = "lw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		string reg = token.substr(token.size()-4, 3);

		if(reg == "$at" || reg == "$gp" || reg == "$k0" || reg == "$k1" || reg == "$ra" || reg == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}

		instruction = make_tuple(tokens.at(0), tokens.at(1), token.substr(0,token.size()-5), token.substr(token.size()-4, 3));
	}else{
		instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), "");
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

		char* q;
		long converted = strtol(token.substr(0,token.size()-5).c_str(), &q, 10);
		if(*q){
			load_error = "sw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}
		
		if(registers.find(token.substr(token.size()-4, 3)) == registers.end()){
			load_error = "sw operation takes 2 operands : register, address\nthe given address is invalid";
			return;
		}

		string reg = token.substr(token.size()-4, 3);

		if(reg == "$at" || reg == "$gp" || reg == "$k0" || reg == "$k1" || reg == "$ra" || reg == "$fp"){
			load_error ="Trying to access registers not accessible to user";
			return;
		}

		instruction = make_tuple(tokens.at(0), tokens.at(1), token.substr(0,token.size()-5), token.substr(token.size()-4, 3));

	}else{

		instruction = make_tuple(tokens.at(0), tokens.at(1), tokens.at(2), "");
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

	if(token == "$r0" || token == "$at" || token == "$gp" || token == "$k0" || token == "$k1" || token == "$ra" || token == "$fp"){
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


//auxiliary function used to load the instructions into the memory
void load_file(string infilename) {

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

		line = line.substr(0, line.size()-1);
		
		tokenize(line);

		load_error = "";
		
		if(tokens.size()<1) {
			continue;
		}

		string token1;
		
		token1 = tokens.at(0);
		
		//if the line starts with a "#", the whole line is a comment
		if(token1.substr(0,1) == "#") {
			continue;
		}

		if (token1.substr(token1.size()-1) == ":") {
			continue;
		}

		//if the first token is an operation, we load the instruction as the correct operation

		if(token1 == "add") {
			load_add(tokens);
			if(load_error != "") {
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		
		if(token1 == "addi") {
			load_addi(tokens);
			if(load_error != "") {
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "lw") {
			load_lw(tokens);
			if(load_error != "") {
				cerr << "load error : " << linenum << ": " << load_error << endl;
				error = 1;
				return;
			}
			memory_num = memory_num + 4;
			continue;
		}

		if(token1 == "sw") {
			load_sw(tokens);
			if(load_error != "") {
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


void print_instructions(tuple<string, string, string, string> instruction) {
	cout << "Instruction executed: " << get<0>(instruction) << "\t" << get<1>(instruction) << "\t" << get<2>(instruction) << "\t" << get<3>(instruction) << endl;
	return;
}


void execute_add(tuple<string, string, string, string> instruction) {
	print_instructions(instruction);
	int a = int_regs[registers[get<2>(instruction)]];
	int b = int_regs[registers[get<3>(instruction)]];
	int c = a + b;
	int_regs[registers[get<1>(instruction)]] = c;
	cout << "Register value updated: " << get<1>(instruction) << " = " << c << endl;
	return;
}


void execute_addi(tuple<string, string, string, string> instruction) {
	print_instructions(instruction);
	int a = int_regs[registers[get<2>(instruction)]];
	int b = stoi(get<3>(instruction));
	int c = a + b;
	int_regs[registers[get<1>(instruction)]] = c;
	cout << "Register value updated: " << get<1>(instruction) << " = " << c << endl;
	return;
}


void execute_lw(tuple<string, string, string, string> instruction) {
	print_instructions(instruction);
	int address;
	if (get<3>(instruction) == "") {
		address = stoi(get<2>(instruction));
	}
	else {
		int a = int_regs[registers[get<3>(instruction)]];
		int b;
		if(get<2>(instruction) == "") {
			b = 0;
		}else{
			b = stoi(get<2>(instruction));
		}
		address = a + b;
	}
	if (address < instructions_end_index) {
		cout << "Trying to load register value from instruction memory" << endl;
		error = 1;
		return;
	}
	if (address%4 != 0 || address >= 262144*4) {
		cout << "Trying to load register value from invalid memory address" << endl;
		error = 1;
		return;
	}

	int row = address/1024;
	int col = (address%1024)/4;

	if (cur_row == -1) {
		for (int i = 0; i < 256; i++) {
			row_buffer[i] = memory[row][i];
		}
		cur_row = row;
		updates += 1;
		int_regs[registers[get<1>(instruction)]] = row_buffer[col];
		delay = clock_cycles + row_access_delay + col_access_delay;
		cout << "Clock Cycle " << clock_cycles + 1 << " to " << delay - col_access_delay << " : DRAM Activate row " << row << endl;
		cout << "Clock Cycle " << delay - col_access_delay + 1 << " to " << delay << " : DRAM column access" << endl;
		cout << "Register value updated: " << get<1>(instruction) << " = " << row_buffer[col] << endl;
	}

	else if (cur_row == row) {
		int_regs[registers[get<1>(instruction)]] = row_buffer[col];
		delay = clock_cycles + col_access_delay;
		cout << "Clock Cycle " << clock_cycles + 1 << " to " << delay << " : DRAM column access" << endl;
		cout << "Register value updated: " << get<1>(instruction) << " = " << row_buffer[col] << endl;
	}

	else {
		for (int i = 0; i < 256; i++) {
			memory[cur_row][i] = row_buffer[i];
		}
		for (int i = 0; i < 256; i++) {
			row_buffer[i] = memory[row][i];
		}
		updates += 1;
		int_regs[registers[get<1>(instruction)]] = row_buffer[col];
		delay = clock_cycles + 2 * row_access_delay + col_access_delay;
		cout << "Clock Cycle " << clock_cycles + 1 << " to " << clock_cycles + row_access_delay << " : DRAM Writeback row " << cur_row << endl;
		cur_row = row;
		cout << "Clock Cycle " << clock_cycles + 1 + row_access_delay << " to " << delay - col_access_delay << " : DRAM Activate row " << row << endl;
		cout << "Clock Cycle " << delay - col_access_delay + 1<< " to " << delay << " : DRAM column access" << endl;
		cout << "Register value updated: " << get<1>(instruction) << " = " << row_buffer[col] << endl;
	}

	return;
}


void execute_sw(tuple<string, string, string, string> instruction) {
	print_instructions(instruction);
	int address;
	if (get<3>(instruction) == "") {
		address = stoi(get<2>(instruction));
	}
	else {
		int a = int_regs[registers[get<3>(instruction)]];
		int b;
		if(get<2>(instruction) == "") {
			b = 0;
		}
		else {
			b = stoi(get<2>(instruction));
		}
		address = a + b;
	}
	if (address < instructions_end_index) {
		cout << "Trying to store register value to instruction memory" << endl;
		error = 1;
		return;
	}
	if(address%4 != 0 || address >= 262144*4) {
		cout << "Trying to store register value to invalid memory address" << endl;
		error = 1;
		return;
	}

	int row = address/1024;
	int col = (address%1024)/4;

	if (cur_row == -1) {
		for (int i = 0; i < 256; i++) {
			row_buffer[i] = memory[row][i];
		}
		cur_row = row;
		row_buffer[col] = int_regs[registers[get<1>(instruction)]];
		updates += 2;
		delay = clock_cycles + row_access_delay + col_access_delay;
		cout << "Clock Cycle " << clock_cycles + 1 << " to " << delay - col_access_delay << " : DRAM Activate row " << row << endl;
		cout << "Clock Cycle " << delay - col_access_delay + 1 << " to " << delay << " : DRAM column access and update" << endl;
		cout << "Memory Address updated: " << address << " = " << row_buffer[col] << endl;

	}

	else if (cur_row == row) {
		row_buffer[col] = int_regs[registers[get<1>(instruction)]];
		updates += 1;
		delay = clock_cycles + col_access_delay;
		cout << "Clock Cycle " << clock_cycles + 1 << " to " << delay << " : DRAM column access and update" << endl;
		cout << "Memory Address updated: " << address << " = " << row_buffer[col] << endl;
	}

	else {
		for (int i = 0; i < 256; i++) {
			memory[cur_row][i] = row_buffer[i];
		}
		for (int i = 0; i < 256; i++) {
			row_buffer[i] = memory[row][i];
		}
		row_buffer[col] = int_regs[registers[get<1>(instruction)]];
		updates += 2;
		delay = clock_cycles + 2 * row_access_delay + col_access_delay;
		cout << "Clock Cycle " << clock_cycles + 1 << " to " << clock_cycles + row_access_delay << " : DRAM Writeback row " << cur_row << endl;
		cur_row = row;
		cout << "Clock Cycle " << clock_cycles + row_access_delay + 1 << " to " << delay - col_access_delay << " : DRAM Activate row " << row << endl;
		cout << "Clock Cycle " << delay - col_access_delay + 1 << " to " << delay << " : DRAM column access and update" << endl;
		cout << "Memory Address updated: " << address << " = " << row_buffer[col] << endl;
	}
	
	return;
}


bool is_valid(tuple<string, string, string, string> instruction) {
	if (get<0>(instruction) == "lw" || get<0>(instruction) == "sw") {
		if (previous) {
			return false;
		}
		else {
			previous = true;
			cur_reg3 = get<1>(instruction);
			cur_reg4 = get<3>(instruction);
			dep2 = get<0>(instruction);
			jump_to = PC;
			cout << "Clock Cycle " << clock_cycles << ":\nInstruction " << get<0>(instruction) << "\t" << get<1>(instruction) << "\t" << get<2>(instruction) << "\t" << get<3>(instruction) << " issues DRAM request" << endl;
			return true;
		}
	}

	//for lw, check dependency for both read and write

	if (dep1 == "lw") {
		if (get<1>(instruction) == cur_reg || get<2>(instruction) == cur_reg || get<3>(instruction) == cur_reg || get<1>(instruction) == cur_reg2) {
			return false;
		}
	}

	if (dep2 == "lw") {
		if (get<1>(instruction) == cur_reg3 || get<2>(instruction) == cur_reg3 || get<3>(instruction) == cur_reg3 || get<1>(instruction) == cur_reg4) {
			return false;
		}
	}

	//for sw, check dependency for only write, as value can be read without problem

	if (dep1 == "sw") {
		if (get<1>(instruction) == cur_reg || get<1>(instruction) == cur_reg2) {
			return false;
		}
	}

	if (dep2 == "sw") {
		if (get<1>(instruction) == cur_reg3 || get<1>(instruction) == cur_reg4) {
			return false;
		}
	}

	return true;

}


void execute(vector<tuple<string, string, string, string>> instructions) {

	int next = -1;

	while(PC < instructions_end_index) {

		tuple<string, string, string, string> instruction;

		instruction = instructions.at(PC/4);


		if(get<0>(instruction) == "add") {
			//cout << "PC = " << PC << endl;
			clock_cycles += 1;
			cout << "Clock Cycle " << clock_cycles << ":" << endl;
			execute_add(instruction);
			if(error == 1){
				return;
			}
			PC += 4;
			next = PC;
		}

		else if(get<0>(instruction) == "addi") {
			//cout << "PC = " << PC << endl;
			clock_cycles += 1;
			cout << "Clock Cycle " << clock_cycles << ":" << endl;
			execute_addi(instruction);
			if (error == 1) {
				return;
			}
			PC += 4;
			next = PC;
		}

		else if(get<0>(instruction) == "lw") {
			//cout << "PC = " << PC << endl;
			if (!previous) {
				clock_cycles += 1;
				cout << "Clock Cycle " << clock_cycles << ": Instruction issues DRAM request" << endl;
			}
			execute_lw(instruction);
			if (error == 1) {
				return;
			}
			else {
				cur_reg = get<1>(instruction);
				cur_reg2 = get<3>(instruction);
				cur_reg3 = "";
				cur_reg4 = "";
				dep1 = "lw";
				dep2 = "";
				jump_to = -1;
				previous = false;
				if (next == -1) {
					PC = PC + 4;
				}
				else {
					PC = next + 4;
				}
				int lim = delay - clock_cycles;
				for (int i = 0; i < lim; i++) {
					//cout << "i = " << i << endl;
					if (PC >= instructions_end_index) {
						clock_cycles = delay;
						break;
					}
					//cout << "PC = " << PC << endl;
					instruction = instructions.at(PC/4);
					clock_cycles += 1;
					if (is_valid(instruction)) {
						if(get<0>(instruction) == "add") {
							cout << "Clock Cycle " << clock_cycles << ":" << endl;
							execute_add(instruction);
							if(error == 1){
								return;
							}
						}

						else if(get<0>(instruction) == "addi") {
							cout << "Clock Cycle " << clock_cycles << ":" << endl;
							execute_addi(instruction);
							if(error == 1){
								return;
							}
						}

						//else {
						//	cout << "Clock Cycle " << clock_cycles << " : " << "Found first lw/sw instruction and skipping it" << endl;
						//}
						PC += 4;
						//cout << "previous = " << previous << endl;
					}

					else {
						break;
					}
				}
				if (previous) {
						next = PC - 4;
						//cout << "next = " << next << endl;
						PC = jump_to;
						//cout << "PC jump = " << PC << endl;
						clock_cycles = delay;
				}
				else {
					next = PC - 4;
					clock_cycles = delay;
				}
			}
		}

		else if(get<0>(instruction) == "sw") {
			//cout << "PC = " << PC << endl;
			if (!previous) {
				clock_cycles += 1;
				cout << "Clock Cycle " << clock_cycles << ": Instruction issues DRAM request" << endl;
			}
			//clock_cycles += 1;
			//cout << "Clock Cycle " << clock_cycles << ": Instruction issues DRAM request" << endl;
			execute_sw(instruction);
			if(error == 1) {
				return;
			}
			else {
				cur_reg = get<1>(instruction);
				cur_reg2 = get<3>(instruction);
				cur_reg3 = "";
				cur_reg4 = "";
				dep1 = "sw";
				dep2 = "";
				jump_to = -1;
				previous = false;
				if (next == -1) {
					PC = PC + 4;
				}
				else {
					PC = next + 4;
				}
				int lim = delay - clock_cycles;
				for (int i = 0; i < lim; i++) {
					//cout << "i = " << i << endl;
					if (PC >= instructions_end_index) {
						clock_cycles = delay;
						break;
					}
					//cout << "PC = " << PC << endl;
					instruction = instructions.at(PC/4);
					clock_cycles += 1;
					if (is_valid(instruction)) {
						
						if(get<0>(instruction) == "add"){
							cout << "Clock Cycle " << clock_cycles << ":" << endl;
							execute_add(instruction);
							if(error == 1){
								return;
							}
						}

						else if(get<0>(instruction) == "addi") {
							cout << "Clock Cycle " << clock_cycles << ":" << endl;
							execute_addi(instruction);
							if(error == 1){
								return;
							}
						}

						//else {
						//	cout << "Clock Cycle " << clock_cycles << " : " << "Found first lw/sw instruction and skipping it" << endl;
						//}

						//cout << "previous = " << previous << endl;
						PC += 4;
					}
					else {
						break;
					}
				}
				if (previous) {
						next = PC - 4;
						//cout << "next = " << next << endl;
						PC = jump_to;
						//cout << "PC jump = " << PC << endl;
						clock_cycles = delay;
				}
				else {
					next = PC - 4;
					clock_cycles = delay;
				}
			}
		}
	}

	return;
}


void execute_part1(vector<tuple<string, string, string, string>> instructions) {
	while(PC < instructions_end_index) {

		tuple<string, string, string, string> instruction;

		instruction = instructions.at(PC/4);


		if(get<0>(instruction) == "add") {
			//cout << "PC = " << PC << endl;
			clock_cycles += 1;
			cout << "Clock Cycle " << clock_cycles << ":" << endl;
			execute_add(instruction);
			if(error == 1){
				return;
			}
			PC += 4;
		}

		else if(get<0>(instruction) == "addi") {
			//cout << "PC = " << PC << endl;
			clock_cycles += 1;
			cout << "Clock Cycle " << clock_cycles << ":" << endl;
			execute_addi(instruction);
			if (error == 1) {
				return;
			}
			PC += 4;
		}

		else if(get<0>(instruction) == "lw") {
			clock_cycles += 1;
			cout << "Clock Cycle " << clock_cycles << " : Instruction issues DRAM request" << endl; 
			execute_lw(instruction);
			if (error == 1) {
				return;
			}
			clock_cycles = delay;
			PC += 4;
		}

		else if(get<0>(instruction) == "sw") {
			clock_cycles += 1;
			cout << "Clock Cycle " << clock_cycles << " : Instruction issues DRAM request" << endl;
			execute_sw(instruction);
			if (error == 1) {
				return;
			}
			clock_cycles = delay;
			PC += 4;
		}
	}

	return;
}


int main(int argc, char* argv[]) {

	if (argc < 5 || argc > 5) {
		cout << "Please enter appropriate arguments: filename, row access delay, column access delay, and part for checking" << endl;
		return -1;
	}

	registers["$r0"] = 0;
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

	load_file(argv[1]);

	row_access_delay = stoi(argv[2]);
	col_access_delay = stoi(argv[3]);
	part = stoi(argv[4]);

	if(error == 1) {
		cout << load_error << endl;
		return -1;
	}

	//int executions[instructions_end_index/4] = {0};
	if (part == 2) {
		execute(instructions);
	}

	else if (part == 1) {
		execute_part1(instructions);
	}

	else {
		cout << "Invalid part value entered" << endl;
		return -1;
	}
	
	cout << "Total clock cycles : " << clock_cycles << endl;
	cout << "Number of row buffer updates: " << updates << endl;
	return 0; 

}
