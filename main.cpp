#include "preproc.hpp"

#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <set>

enum PASS_ERROR
{
	UNKNOWN_INSTRUCTION,
	ILLEGAL_ADR_TYPE,
	ORG_ADR_ERROR,
	TOO_FAR_JMP,
	UNKNOWN_PREPROC_INSTRUCTION,
	ILLEGAL_OPERAND,
	UNDEFINED_LABEL,
	ILLEGAL_DEFINE,
	BIN_FILE_NOT_FOUND,
	INPUT_FILE_NOT_FOUND
};

void show(std::vector<std::string> v)
{
	for(auto i : v)
		std::cout << i << std::endl;
}

std::string get_str(std::vector<std::string> op)
{
	std::string s = "";
	for(auto i : op)
		s += i;
	return s;
}

void err_show(PASS_ERROR err, int passnum, std::string instruction)
{
	std::string errs;

	switch(err)
	{
		case UNKNOWN_INSTRUCTION:
			errs = "Unknown instruction.";
			break;
		case ILLEGAL_ADR_TYPE:
			errs = "Illegal addressing type for instruction.";
			break;
		case ORG_ADR_ERROR:
			errs = "Illegal address for .org command.";
			break;
		case TOO_FAR_JMP:
			errs = "Too far jump.";
			break;
		case UNKNOWN_PREPROC_INSTRUCTION:
			errs = "Unknown preprocessing instruction.";
			break;
		case ILLEGAL_OPERAND:
			errs = "Illegal operand.";
			break;
		case UNDEFINED_LABEL:
			errs = "Undefined label.";
			break;
		case ILLEGAL_DEFINE:
			errs = "Defined names must start with @";
			break;
		case BIN_FILE_NOT_FOUND:
			errs = "Binary file not found.";
			break;
	}

	std::cout << "sfotasm: error on PASS " << std::to_string(passnum) << ": " << errs << std::endl;
	std::cout << "sfotasm: instruction: " << instruction << std::endl;
	exit(0);
}

std::vector<std::string> makeVectorFromFile(std::string filename)
{
	std::ifstream input(filename);
	std::vector<std::string> strs;
	std::string tmp_line;

	if(!input.good())
	{
		std::cout << "sfotasm: file not found." << std::endl;
		exit(0);
	}

	for(std::string line; std::getline(input, line);)
	{
		if(line == "" || line[0] == ';')
			continue;

		int i = 0;
		while(line[i] == ' ')
			i++;

		for(; i < line.size(); i++)
		{
			if(line[i] == ';')
				break;
			if(line[i] == '\t' || line[i] == '\n')
				continue;
			tmp_line += line[i];
		}

		if(tmp_line != "")
			strs.push_back(tmp_line);
		tmp_line = "";
	}

	return strs;
}

void writeBinary(std::string code, std::string filename)
{
	std::ofstream output(filename, std::ios::binary);
	unsigned char b;

	for(int i = 0; i < code.length()-1; i += 2)
	{
		std::string byte = std::string(1, code[i]) + std::string(1, code[i+1]);
		b = std::stoi(byte, 0, 16);

		output << b;
	}
}

void writeListring(std::string listing, std::string filename)
{
	std::ofstream output(filename);	

	output << listing;
}

std::map<std::string, std::string> def_addrs;
std::vector<std::string> def_names;

void initDefs()
{
	def_addrs["@START"] = "$C000";
	def_addrs["@INTS"] = "$FFFA";
	def_addrs["@JOY1"] = "$4016";
	def_addrs["@JOY2"] = "$4017";
	def_addrs["@APU_PULSE1_CTRL"] = "$4000";
	def_addrs["@APU_PULSE1_RCTRL"] = "$4001";
	def_addrs["@APU_PULSE1_FT"] = "$4002";
	def_addrs["@APU_PULSE1_CT"] = "$4003";

	def_names = {"@START", "@INTS", "@JOY1", "@JOY2", "@APU_PULSE1_CTRL",
				 "@APU_PULSE1_RCTRL", "@APU_PULSE1_FT", "@APU_PULSE1_CT"};
}

void show_help()
{
	std::cout << "sfotasm\n";
	std::cout << "6502 NES assembler\n\nUsage:\n";
	std::cout << "\tsfotasm inputfile.asm [outputfile.nes]";
	std::cout << std::endl;
}

int main(int argc, char** argv)
{
	std::string filename = "asm.asm";
	std::string resfilename = "result.nes";

	if(argc > 1)
	{
		filename = std::string(argv[1]);

		if(argc > 2)
			resfilename = std::string(argv[2]);
	}

	else
	{
		show_help();
		exit(0);
	}

	auto insts = makeVectorFromFile(filename);

	const int START_ADR = 0xC000;

	size_t bank = 0;
	size_t real_adr = START_ADR;
	size_t rsset = 0;
	size_t instr_num = 0;

	std::map<std::string, size_t> label_adrs;
	std::map<size_t, size_t> rel_adrs;
	std::vector<std::string> label_names;
	std::vector<std::string> rs_names;
	std::set<size_t> used_banks;

	instructions inst;
	preproc pr;

	// PASS 0: add includes

	size_t len = insts.size();

	for(size_t i(0); i < len; i++)
	{
		auto res = inst.parseInstruction(insts[i]);

		if(res[0] == PREPROC_SIGN)
		{
			auto prres = pr.parsePreprocInstruction(insts[i]);

			if(prres[0] == PREPROC_INCLUDE_SIGN)
			{
				auto f = makeVectorFromFile(prres[1]);
				std::reverse(std::begin(f), std::end(f));

				insts.erase(insts.begin()+i);

				for(auto j : f)
					insts.insert(insts.begin()+i, j);

				len += f.size()-1;
			}
		}

	}

	// PASS 1: some preprocessing, label setting, syntax checking

	bool defaddrs = false;
	initDefs();

	std::map<std::string, std::string> user_def_addrs;
	std::vector<std::string> user_def_names;

	for(auto& i : insts)
	{
		if(defaddrs)
		{
			for(auto j : def_names)
			{
					if(i.find(j) != -1)
					{
						i.replace(i.find(j), i.length(), def_addrs[j]);
					}
			}
		}

		for(auto j : user_def_names)
		{
			if(i.find(j) != -1)
			{
				i.replace(i.find(j), i.length(), user_def_addrs[j]);
			}
		}

		auto res = inst.parseInstruction(i);

		if(res[0] == COMMENT_SIGN)
			continue;
		else if(res[0] == ERROR_ILLEGAL_INSTRUCTION_SIGN)
			err_show(UNKNOWN_INSTRUCTION, 1, i);
		else if(res[0] == ERROR_ILLEGAL_OPERAND_SIGN)
			err_show(ILLEGAL_OPERAND, 1, i);

		if(res[0] == PREPROC_SIGN)
		{
			auto prres = pr.parsePreprocInstruction(i);

			if(prres[0] == PREPROC_ERROR)
				err_show(UNKNOWN_PREPROC_INSTRUCTION, 1, i);

			else if(prres[0] == PREPROC_USE_ILLOPCODES_SIGN)
			{
				inst.addIllegalOpcodes();
			}

			else if(prres[0] == PREPROC_USE_DEFS_SIGN)
			{
				defaddrs = true;
			}

			else if(prres[0] == PREPROC_DEFINE_SIGN)
			{
				if(prres[1][0] != '@')
					err_show(ILLEGAL_DEFINE, 1, i);

				user_def_names.push_back(prres[1]);
				user_def_addrs[prres[1]] = prres[2];
			}

			else if(prres[0] == PREPROC_OFFSET_SIGN)
			{
				int adr = std::stoi(prres[1]);

				if(adr >= (bank+1)*0x2000 + 0xC000)
					err_show(ORG_ADR_ERROR, 1, i);

				if(adr >= bank*0x2000 + 0xC000)
					real_adr = adr;
				else
					real_adr = adr + (bank*0x2000 + 0xC000);
			}

			else if(prres[0] == PREPROC_BANK_SIGN)
			{
				bank = std::stoi(prres[1]);
				used_banks.insert(bank);
				real_adr = bank*0x2000 + 0xC000;
			}

			else if(prres[0] == PREPROC_RSSET_SIGN)
			{
				rsset = std::stoi(prres[1]);
			}

			else if(prres[0] == PREPROC_RS_SIGN)
			{
				std::string rs_name = prres[1];
				size_t bytes = std::stoi(prres[2]);

				label_names.push_back(rs_name);
				label_adrs[rs_name] = rsset;
				rsset += bytes;
			}

			else if(prres[0] == PREPROC_DB_SIGN)
			{
				real_adr += prres.size()-1;
			}

			else if(prres[0] == PREPROC_DW_SIGN)
			{
				real_adr += 2*(prres.size()-1);
			}

			else if(prres[0] == PREPROC_DW_LABEL_SIGN)
			{
				real_adr += 2;
			}
		}

		else if(res[0] == LABEL_SIGN)
		{
			label_names.push_back(res[1]);
			label_adrs[res[1]] = real_adr;
		}

		else if(res[0] == RELATIVE_SIGN)
		{
			real_adr += 2;
			rel_adrs[instr_num] = real_adr;
		}

		else if(res[0] == RELATIVE_ADDR_SIGN)
		{
			real_adr += 2;
			rel_adrs[instr_num] = real_adr;
		}


		else
		{
			real_adr += res.size();
		}

		instr_num++;
	}


	// PASS 2: syntax checking, prog making

	std::string prog[127];
	bank = 0;
	size_t position = 0;
	size_t chr_size = 0;
	std::map<size_t, size_t> positions;

	bool listed = false;
	bool nowlisting = false;
	std::string listing = "";

	instr_num = 0;

	for(auto i : insts)
	{	
		auto res = inst.parseInstruction(i);

		if(res[0] == PREPROC_SIGN)
		{
			auto prres = pr.parsePreprocInstruction(i);

			if(prres[0] == PREPROC_BANK_SIGN)
			{
				positions[bank] = position;
				bank = std::stoi(prres[1]);
				position = positions[bank];
			}

			else if(prres[0] == PREPROC_OFFSET_SIGN)
			{
				int adr = std::stoi(prres[1]);

				if(adr >= bank*0x2000 + 0xC000)
					position = (adr - (0xC000 + bank*0x2000))*2;
				else
					position = adr*2;	

				while(position > prog[bank].size())
					prog[bank] += "FF";		
			}

			else if(prres[0] == PREPROC_DB_SIGN)
			{
				for(size_t i(1); i < prres.size(); i++)
				{
					prog[bank] += prres[i];
					position += 2;
				}
			}

			else if(prres[0] == PREPROC_DW_SIGN)
			{
				for(size_t i(1); i < prres.size(); i++)
				{
					prog[bank] += prres[i];
					position += 4;
				}
			}

			else if(prres[0] == PREPROC_LIST_SIGN)
			{
				listed = true;
				nowlisting = true;
			}

			else if(prres[0] == PREPROC_NOLIST_SIGN)
			{
				nowlisting = false;
			}				

			else if(prres[0] == PREPROC_DW_LABEL_SIGN)
			{
				if(std::find(label_names.begin(), label_names.end(), prres[1]) == label_names.end())
					err_show(UNDEFINED_LABEL, 2, i);

				size_t addr = label_adrs[prres[1]];
				std::stringstream s;
				std::string num;
				s << std::hex << std::uppercase << addr;
				num = s.str();

				while(num.length() < 4)
					num = "0" + num;

				prog[bank] += std::string(1,num[2]) + std::string(1,num[3]);
				prog[bank] += std::string(1,num[0]) + std::string(1,num[1]);
				position += 2 * 2;
			}

			else if(prres[0] == PREPROC_INCBIN_SIGN)
			{
				std::string hex;
				unsigned char* buffer;

				struct stat bf;

				if(stat(prres[1].c_str(), &bf) != 0)
					err_show(BIN_FILE_NOT_FOUND, 2, i);

				FILE* fptr = fopen(prres[1].c_str(), "rb");

				fseek(fptr, 0, SEEK_END);
				unsigned long fileLen = ftell(fptr);
				fseek(fptr, 0, SEEK_SET);

				buffer = (unsigned char*)malloc(fileLen+1);

				fread(buffer, fileLen, 1, fptr);
				fclose(fptr);

				for(size_t i(0); i < fileLen+1; i++)
				{
					std::stringstream ss;
					ss << std::hex << std::uppercase << (unsigned int)buffer[i];
					hex = ss.str();

					while(hex.length() < 2)
						hex = "0" + hex;

					prog[bank] += hex;

					chr_size++;

					position += 2;
					real_adr++;

					if(chr_size >= pr.getChrSizeKb()*1024)
						break;					
				}

				free(buffer);
			}
		}

		else if(res[0] == LABEL_CALL_SIGN)
		{
			if(std::find(label_names.begin(), label_names.end(), res[1]) == label_names.end())
				err_show(UNDEFINED_LABEL, 2, i);

			size_t addr = label_adrs[res[1]];
			std::stringstream s;
			std::string num;
			s << std::hex << std::uppercase << addr;
			num = s.str();

			while(num.length() < 4)
				num = "0" + num;

			prog[bank] += res[2];
			prog[bank] += std::string(1,num[2]) + std::string(1,num[3]);
			prog[bank] += std::string(1,num[0]) + std::string(1,num[1]);
			position += 3*2;

			if(nowlisting)
			{
				listing += res[2];
				listing += std::string(1,num[2]) + std::string(1,num[3]);
				listing += std::string(1,num[0]) + std::string(1,num[1]);				
				listing += "\t" + i + "\n";			
			}
		}

		else if(res[0] == RELATIVE_SIGN)
		{
			prog[bank] += res[2];

			int adr = label_adrs[res[1]] - rel_adrs[instr_num];
			if(adr < 0)
			{
				if(adr < -0xFF)
					err_show(TOO_FAR_JMP, 2, i);
				adr = 0xFF - (rel_adrs[instr_num] - label_adrs[res[1]] - 1);
			}

			if(adr > 0xFF)
				err_show(TOO_FAR_JMP, 2, i);

			std::stringstream s;
			std::string hx;
			s << std::hex << std::uppercase << adr;
			hx = s.str();

			if(hx.length() < 2)
				hx = "0" + hx;

			if(nowlisting)
				listing += "00"+res[2]+hx + "\t" + i + "\n";

			prog[bank] += hx;
			position += 2*2;
		}

		else if(res[0] == RELATIVE_ADDR_SIGN)
		{
			prog[bank] += res[2];

			int adr = std::stoi(res[1], 0, 16) - rel_adrs[instr_num];			

			if(adr < 0)
			{
				if(adr < -0xFF)
					err_show(TOO_FAR_JMP, 2, i);
				adr = 0xFF - (rel_adrs[instr_num] - std::stoi(res[1], 0, 16) - 1);
			}

			if(adr > 0xFF)
				err_show(TOO_FAR_JMP, 2, i);

			std::stringstream s;
			std::string hx;
			s << std::hex << std::uppercase << adr;
			hx = s.str();

			if(hx.length() < 2)
				hx = "0" + hx;

			if(nowlisting)
				listing += "00"+res[2]+hx + "\t" + i + "\n";

			prog[bank] += hx;
			position += 2*2;
		}

		else if(res[0] == LABEL_SIGN)
		{
			instr_num++;
			continue;
		}

		else
		{
			std::string opc = get_str(res);

			if(nowlisting)
			{
				std::string zopc = opc;
				while(zopc.size() < 6)
					zopc = "0" + zopc;
				listing += zopc + "\t" + i + "\n";
			}

			prog[bank] += opc;
			position += res.size()*2;
		}

		instr_num++;
	}

	std::string prog_res;

	for(size_t i(0); i <= *used_banks.rbegin(); i++)
	{
		if(used_banks.find(i) != used_banks.end())
		{
			std::string tmp = prog[i];

			while(tmp.length() < 8*1024*2)
				tmp += "F";
			prog_res += tmp;
		}

		else
			for(int i = 0; i != 8*1024*2; i++)
				prog_res += "F";
	}

	prog_res = pr.makeHeader() + prog_res;

	writeBinary(prog_res, resfilename);

	if(listed)
		writeListring(listing, resfilename+".lst");
}