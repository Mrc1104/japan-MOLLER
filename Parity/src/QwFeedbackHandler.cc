#include "QwFeedbackHandler.h"
#include <string_view>
QwFeedbackHandler::QwFeedbackHandler(TString const& name)
: VQwDataHandler(name)
, fSlope{}
{
  	fKeepRunningSum = true;
}
QwFeedbackHandler::QwFeedbackHandler(QwFeedbackHandler const& source)
: VQwDataHandler(source)
, fSlope{}
{ }

void QwFeedbackHandler::ParseConfigFile(QwParameterFile& file)
{
	VQwDataHandler::ParseConfigFile(file);
  	file.PopValue("slope_ihwp_in", fSlope[IHWP::kIN]);
  	file.PopValue("slope_ihwp_out",fSlope[IHWP::kOUT]);
}
Int_t QwFeedbackHandler::LoadChannelMap(std::string const& mapfile)
{
	std::cout << "Opening: " << mapfile<< '\n';
	// Open the file
	QwParameterFile map(mapfile);

	while (map.ReadNextLine()) {
		std::cout << "ReadNextLine()\n";
		// Throw away comments, whitespace, empty lines
		map.TrimComment();
		map.TrimWhitespace();
		if (map.LineIsEmpty()) continue;
		std::string token = map.GetNextToken(" ");
		configuration config;
		while ( !token.empty() ) {
			config.parse_pair(token);
			token = map.GetNextToken(" ");
		}
		if(config.isValid()) {
			QwWarning << "Invalid Feedback Config:" << config << '\n';
			continue;
		}
		std::cout << config << '\n';
	}

	return 0;
}

	return 0;
}

bool QwFeedbackHandler::configuration::parse_pair(std::string_view token) {
	bool status = false;
	if( auto sv = parse_pair_impl(token, "type" ); sv != std::string_view{} ) {
		if(sv.compare("target"))   type = TYPE::kTARGET;
		else if(sv.compare("ioc")) type = TYPE::kIOC;
		else type = TYPE::kUNKNOWN;
		status = true;
	}
	else if( auto sv = parse_pair_impl(token, "name" ); sv != std::string_view{} ) {
		name = std::string(sv);
		status = true;
	}
	else if( auto sv = parse_pair_impl(token, "Decr" ); sv != std::string_view{} ) {
		descr = std::string(sv);
		status = true;
	}
	return status;
}
std::string_view QwFeedbackHandler::configuration::parse_pair_impl(std::string_view token, std::string_view target)
{
	auto res = std::string_view{};
	if( auto targ_pos = token.find(target), sep_pos = token.find('=', targ_pos+1);
		targ_pos!= std::string_view::npos &&
		sep_pos != std::string_view::npos)
	{
		std::cout << "substr = " << token.substr(sep_pos+1) << '\n';
		res = token.substr(sep_pos+1);
	}
	return res;
}

bool QwFeedbackHandler::configuration::isValid() const
{
	bool valid = false;
	if( (type == TYPE::kTARGET || type == TYPE::kIOC)
	    && !name.empty() ) {
		valid = true;
	}
	return valid;
}


TYPE QwFeedbackHandler::configuration::getType() const { return type; }

