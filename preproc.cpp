#include "preproc.hpp"

preproc::preproc()
{
	keywords = {".ines", ".inesprg", ".ineschr", ".inesmap", ".inesmir", ".org",
				 ".db", "dw", "incbin", ".bank", ".rsset", ".rs",
				 ".byte", ".word", ".use", ".include", ".list", ".nolist",
				 ".define"};
}

std::vector<std::string> preproc::parsePreprocInstruction(std::string inst)
{
	std::istringstream iss(inst);
	std::vector<std::string> parsed_inst((std::istream_iterator<std::string>(iss)),
                                 std::istream_iterator<std::string>());

	if(parsed_inst[0] == ".inesprg")
		prg = parsed_inst[1];

	else if(parsed_inst[0] == ".ineschr")
		chr = parsed_inst[1];

	else if(parsed_inst[0] == ".inesmap")
		mapper = parsed_inst[1];

	else if(parsed_inst[0] == ".inesmir")
		mirroring = parsed_inst[1];

	else if(parsed_inst[0] == ".ines")
	{
		prg = parsed_inst[1];
		chr = parsed_inst[2];
		mapper = parsed_inst[3];
		mirroring = parsed_inst[4];
	}

	else if(parsed_inst[0] == ".incbin")
	{
		parsed_inst[1].pop_back();
		parsed_inst[1].erase(parsed_inst[1].begin());
		return {PREPROC_INCBIN_SIGN, parsed_inst[1]};
	}

	else if(parsed_inst[0] == ".org")
	{
		return {PREPROC_OFFSET_SIGN, std::to_string(makeDec(parsed_inst[1]))};
	}

	else if(parsed_inst[0] == ".bank")
	{
		return {PREPROC_BANK_SIGN, parsed_inst[1]};
	}

	else if(parsed_inst[0] == ".list")
	{
		return {PREPROC_LIST_SIGN};
	}

	else if(parsed_inst[0] == ".nolist")
	{
		return {PREPROC_NOLIST_SIGN};
	}

	else if(parsed_inst[0] == ".rsset")
	{
		return {PREPROC_RSSET_SIGN, std::to_string(makeDec(parsed_inst[1]))};
	}

	else if(parsed_inst[0] == ".define")
	{
		return {PREPROC_DEFINE_SIGN, parsed_inst[1], parsed_inst[2]};
	}

	else if(parsed_inst[1] == ".rs")
	{
		return {PREPROC_RS_SIGN, parsed_inst[0], parsed_inst[2]};
	}

	else if(parsed_inst[0] == ".include")
	{
		parsed_inst[1].pop_back();
		parsed_inst[1].erase(parsed_inst[1].begin());
		return {PREPROC_INCLUDE_SIGN, parsed_inst[1]};
	}

	else if(parsed_inst[0] == ".use")
	{
		if(parsed_inst[1] == "illegal_opcodes")
			return {PREPROC_USE_ILLOPCODES_SIGN};
		else if(parsed_inst[1] == "addresses_defines")
			return {PREPROC_USE_DEFS_SIGN};
		else
			return {PREPROC_ERROR};
	}

	else if(parsed_inst[0] == ".db" || parsed_inst[0] == ".byte")
	{
		std::vector<std::string> args;

		args.push_back(PREPROC_DB_SIGN);

		for(size_t i(1); i < parsed_inst.size(); i++)
		{
			if(i != parsed_inst.size()-1)
				parsed_inst[i].pop_back();
			OPERAND_TYPE t = ins.getOperandType(parsed_inst[i]);

			if(t == OPERAND_TYPE::ADDRESS)
			{
				std::string tmp = ins.convertToHex("#"+parsed_inst[i], ins.isNumber("#"+parsed_inst[i]), false);

				while(tmp.length() < 2)
					tmp = "0" + tmp;

				args.push_back(tmp);
			}
		}

		return args;
	}

	else if(parsed_inst[0] == ".dw" || parsed_inst[0] == ".word")
	{
		if(ins.getOperandType(parsed_inst[1]) == OPERAND_TYPE::LABEL)
			return {PREPROC_DW_LABEL_SIGN, parsed_inst[1]};

		std::vector<std::string> args;

		args.push_back(PREPROC_DW_SIGN);

		for(size_t i(1); i < parsed_inst.size(); i++)
		{
			if(i != parsed_inst.size()-1)
				parsed_inst[i].pop_back();
			OPERAND_TYPE t = ins.getOperandType(parsed_inst[i]);

			if(t == OPERAND_TYPE::ADDRESS)
			{
				std::string tmp = ins.convertToHex("#"+parsed_inst[i], ins.isNumber("#"+parsed_inst[i]), false);

				while(tmp.length() < 4)
					tmp = "0" + tmp;

				std::string adr = std::string(1,tmp[2]) + std::string(1,tmp[3]);
				adr += std::string(1,tmp[0]) + std::string(1,tmp[1]);

				args.push_back(adr);
			}
		}

		return args;
	}

	else
		return {PREPROC_ERROR};

	return {PREPROC_DONE};
}

std::string preproc::makeHeader()
{
	std::string header = "";
	std::string mp = makeHex(mapper);

	header += HEADER_START;
	header += makeHex(prg);
	header += makeHex(chr);
	header += mapper.length() == 1 ? std::string(1, mapper[0]) : std::string(1, mapper[1]);
	header += mirroring;
	header += mapper.length() == 1 ? std::string(1, '0') : std::string(1, mapper[0]);
	header += "0";

	header += "0000000000000000";

	return header;
}

int preproc::makeDec(std::string w)
{
	w.erase(w.begin());
	std::stringstream s;
	size_t n;
	s << std::hex << w;
	s >> n;

	return n;
}

int preproc::getChrSizeKb()
{
	return std::stoi(chr)*8;
}

bool preproc::isPreprocKeyword(std::string key)
{
	return std::find(keywords.begin(), keywords.end(), key) != keywords.end();
}

std::string preproc::makeHex(std::string w)
{
	std::stringstream s;
	s << std::hex << std::uppercase << std::stoi(w);

	auto n = s.str();

	if(n.length() < 2)
		n = "0" + n;

	return n;
}