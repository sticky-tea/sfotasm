#include "opcodes.hpp"

#include <sstream>
#include <iterator>
#include <fstream>
#include <bitset>
#include <string>

enum NUM_TYPE
{
	NAN,
	BIN_NUM,
	HEX_NUM,
	DEC_NUM
};

enum OPERAND_TYPE
{
	LABEL,
	ADDRESS,
	NUMBER,
	ADR_INDX,
	ADR_INDY,
};

const std::string LABEL_SIGN = "l"; 
const std::string COMMENT_SIGN = ";"; 
const std::string LABEL_CALL_SIGN = "lc";
const std::string JMP_SIGN = "j";
const std::string RELATIVE_SIGN = "rl";
const std::string PREPROC_SIGN = ".";
const std::string ERROR_ILLEGAL_INSTRUCTION_SIGN = "ei"; 
const std::string ERROR_ILLEGAL_OPERAND_SIGN = "eo"; 

class instructions
{
public:
	// vector: {OPCODE ADDR ADDR} ... or {*_SIGN}
	std::vector<std::string> parseInstruction(std::string instruction);

	NUM_TYPE isNumber(std::string op);
	OPERAND_TYPE getOperandType(std::string op);

	std::string convertToHex(std::string num, NUM_TYPE tp, bool is_addr);

	void addIllegalOpcodes();
private:
	opcodes codes;

	std::vector<std::string> parseKeyword(std::vector<std::string> instruction);
};