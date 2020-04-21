#include "instructions.hpp"

std::vector<std::string> instructions::parseInstruction(std::string instruction)
{
	if(instruction[0] == ';')
	return {COMMENT_SIGN};

	std::istringstream iss(instruction);
	std::vector<std::string> parsed_inst((std::istream_iterator<std::string>(iss)),
                                 std::istream_iterator<std::string>());

	std::string inst_name = parsed_inst[0];
	std::string kw = std::string(inst_name);

	for(auto& c : kw) 
		c = ::toupper(c);

	if(codes.isKeyword(kw))
	{
		parsed_inst[0] = kw;
		return parseKeyword(parsed_inst);
	}

	else if(inst_name[inst_name.length()-1] == ':')
	{
		inst_name.pop_back();
		return {LABEL_SIGN, inst_name};
	}
	else if(inst_name[0] == '.')
		return {PREPROC_SIGN};
	else if(parsed_inst.size() > 1 && parsed_inst[1][0] == '.')
		return {PREPROC_SIGN};
	return {ERROR_ILLEGAL_INSTRUCTION_SIGN};
}

std::vector<std::string> instructions::parseKeyword(std::vector<std::string> instruction)
{
	std::string name = instruction[0];
	std::vector<std::string> code;

	if(instruction.size() == 1)
	{
		code.push_back(codes.getOpcode(name, OPCODE_TYPE::IMPLIED));
	}

	if(instruction.size() == 2)
	{
		std::string op = instruction[1];
		OPERAND_TYPE tp = getOperandType(op);

		switch(tp)
		{
			case NUMBER:
			{
				op = convertToHex(op, isNumber(op), false);

				while(op.length() < 2)
					op = '0' + op;

				code.push_back(codes.getOpcode(name, OPCODE_TYPE::IMM));
				code.push_back(op);
				break;
			}

			case ADDRESS:
			{
				op = convertToHex(op, isNumber('#'+op), true);

				if(codes.isRelativeKeyword(name))
					return {RELATIVE_ADDR_SIGN, op, codes.getOpcode(name, OPCODE_TYPE::IMPLIED)};

				std::string low = std::string(1, op[0]) + std::string(1, op[1]);
				std::string high = std::string(1, op[2]) + std::string(1, op[3]);

				code.push_back(codes.getOpcode(name, OPCODE_TYPE::ABS));
				code.push_back(high);
				code.push_back(low);
				break;
			}

			case LABEL:
			{
				if(codes.isRelativeKeyword(name))
					return {RELATIVE_SIGN, op, codes.getOpcode(name, OPCODE_TYPE::IMPLIED)};

				code.push_back(LABEL_CALL_SIGN);
				code.push_back(op);
				code.push_back(codes.getOpcode(name, OPCODE_TYPE::ABS));
				break;
			}
		}
	}

	if(instruction.size() == 3)
	{
		std::string op = instruction[1];
		std::string reg = instruction[2];

		if(op[op.length()-1] == ',')
		{
			op.pop_back();
			OPERAND_TYPE tp = getOperandType(op);

			if(tp == OPERAND_TYPE::ADR_INDY && reg == "Y")
			{
				op.pop_back(); // )
				op.erase(op.begin());
				op = convertToHex(op, isNumber('#'+op), true);

				std::string low = std::string(1, op[0]) + std::string(1, op[1]);
				std::string high = std::string(1, op[2]) + std::string(1, op[3]);

				code.push_back(codes.getOpcode(name, OPCODE_TYPE::INDY));
				code.push_back(high);
				code.push_back(low);

				return code;
			}

			else if(tp == OPERAND_TYPE::ADR_INDX && reg == "X)")
			{	
				op.erase(op.begin());
				op = convertToHex(op, isNumber('#'+op), true);

				std::string low = std::string(1, op[0]) + std::string(1, op[1]);
				std::string high = std::string(1, op[2]) + std::string(1, op[3]);

				code.push_back(codes.getOpcode(name, OPCODE_TYPE::INDX));
				code.push_back(high);
				code.push_back(low);

				return code;
			}

			else if(tp == OPERAND_TYPE::ADDRESS)
			{
				op = convertToHex(op, isNumber('#'+op), true);

				std::string low = std::string(1, op[0]) + std::string(1, op[1]);
				std::string high = std::string(1, op[2]) + std::string(1, op[3]);

				if(reg == "X" || reg == "x")
				{
					code.push_back(codes.getOpcode(name, OPCODE_TYPE::ABSX));
					code.push_back(high);
					code.push_back(low);
				}

				else if(reg == "Y" || reg == "y")
				{
					code.push_back(codes.getOpcode(name, OPCODE_TYPE::ABSY));
					code.push_back(high);
					code.push_back(low);
				}
			}

			else if(tp == OPERAND_TYPE::LABEL)
			{
				code.push_back(LABEL_CALL_SIGN);
				code.push_back(op);
				if(reg == "X" || reg == "x")
				{
					code.push_back(codes.getOpcode(name, OPCODE_TYPE::ABSX));
				}
				else
				{
					code.push_back(codes.getOpcode(name, OPCODE_TYPE::ABSY));
				}

				return code;
			}

			else
				return {ERROR_ILLEGAL_OPERAND_SIGN};
		}
	}

	return code;
}

NUM_TYPE instructions::isNumber(std::string op)
{
	if(op[0] != '#')
		return NUM_TYPE::NAN;

	if(op[1] == '$')
	{
		for(int i(2); i < op.size(); i++)
			if(!((op[i] >= '0' && op[i] <= '9') || (op[i] >= 'A' && op[i] <= 'F')))
				return NUM_TYPE::NAN;
		return NUM_TYPE::HEX_NUM;
	}

	else if(op[1] == '%')
	{
		for(int i(2); i < op.size(); i++)
			if(op[i] != '0' && op[i] != '1')
				return NUM_TYPE::NAN;
		return NUM_TYPE::BIN_NUM;
	}

	else
	{
		for(int i(2); i < op.size(); i++)
			if(!((op[i] >= '0' && op[i] <= '9')))
				return NUM_TYPE::NAN;
		return NUM_TYPE::DEC_NUM;
	}
}

OPERAND_TYPE instructions::getOperandType(std::string op)
{
	if(isNumber(op))
		return OPERAND_TYPE::NUMBER;
	if(isNumber('#'+op))
		return OPERAND_TYPE::ADDRESS;
	if(op[0] == '(' && op[op.length()-1] == ')')
	{
		op.pop_back();
		op.erase(op.begin());

		if(isNumber('#'+op))
			return OPERAND_TYPE::ADR_INDY;
	}

	if(op[0] == '(' && op[op.length()-1] != ')')
	{
		op.erase(op.begin());

		if(isNumber('#'+op))
			return OPERAND_TYPE::ADR_INDX;
	}

	return OPERAND_TYPE::LABEL;
}

std::string instructions::convertToHex(std::string num, NUM_TYPE tp, bool is_addr)
{
	std::stringstream s;
	std::string hx;

	switch(tp)
	{
		case BIN_NUM:
		{
			num.erase(num.begin());

			if(!is_addr)
				num.erase(num.begin());

			std::bitset<8> bt(num);

			s << std::hex << std::uppercase << bt.to_ulong();
			hx = s.str();
			break;
		}
		case HEX_NUM:
		{
			num.erase(num.begin());
			if(!is_addr)
				num.erase(num.begin());

			while(num[0] == '0')
				num.erase(num.begin());

			s << std::uppercase << num;
			hx = s.str();
			break;
		}
		case DEC_NUM:
		{
			if(!is_addr)
				num.erase(num.begin());

			s << std::hex << std::uppercase << std::stoi(num);
			hx = s.str();
		}
	}

	if(is_addr)
	{
		while(hx.length() < 4)
			hx = '0' + hx;
	}

	return hx;
}

void instructions::addIllegalOpcodes()
{
	codes.initIllegalOpcodes();
}