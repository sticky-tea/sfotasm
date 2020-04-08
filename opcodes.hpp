#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>

enum OPCODE_TYPE
{
	IMPLIED = 0,
	INDX,
	INDY,
	ABS,
	ABSX,
	IMM,
	ABSY,
	ZP,
	ZPX,
};

const std::string ERROR_ILLEGAL_OPERAND_SIGN_OP = "eo"; 

class opcodes
{
public:
	opcodes();
	std::string getOpcode(std::string name, OPCODE_TYPE addr_type);

	bool isKeyword(std::string name);
	bool isRelativeKeyword(std::string name);

	void initIllegalOpcodes();
private:
	void initOpcodes();
	std::map<const std::pair<std::string, OPCODE_TYPE>, std::string> opcode;
	std::vector<std::string> opcode_names;

	std::vector<std::string> rel_opcodes = {"BCC", "BCS", "BEQ", "BMI", "BNE", "BPL", "BVC", "BVS"};

	std::string getOpcodesList();
	std::string getIllegalOpcodesList();
};