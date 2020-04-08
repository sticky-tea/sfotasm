#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>

#include "instructions.hpp"

const std::string HEADER_START = "4E45531A";

const std::string PREPROC_ERROR = "-1";
const std::string PREPROC_DONE = "0";

const std::string PREPROC_BANK_SIGN = "b";

const std::string PREPROC_OFFSET_SIGN = "o";
const std::string PREPROC_DB_SIGN = "db";
const std::string PREPROC_DW_SIGN = "dw";
const std::string PREPROC_DW_LABEL_SIGN = "dwl";
const std::string PREPROC_RSSET_SIGN = "rss";
const std::string PREPROC_RS_SIGN = "rs";

const std::string PREPROC_INCLUDE_SIGN = "ic";
const std::string PREPROC_INCBIN_SIGN = "ib";

const std::string PREPROC_LIST_SIGN = "lst";
const std::string PREPROC_NOLIST_SIGN = "nlst";

const std::string PREPROC_USE_ILLOPCODES_SIGN = "uo";
const std::string PREPROC_USE_DEFS_SIGN = "ud";

const std::string PREPROC_DEFINE_SIGN = "df";

class preproc
{
public:
	preproc();

	std::vector<std::string> parsePreprocInstruction(std::string inst);
	std::string makeHeader();

	bool isPreprocKeyword(std::string key);

	int getChrSizeKb();

	int makeDec(std::string w);
private:
	instructions ins;

	std::vector<std::string> keywords;

	std::string prg;
	std::string chr; 
	std::string mapper;
	std::string mirroring;

	std::string makeHex(std::string w);

};