#include "QwFeedbackConfig.h"

bool QwFeedbackConfig::parse_pair(std::string_view token) {
	bool status = false;
	if( auto sv = parse_pair_impl(token, "type" ); sv != std::string_view{} ) {
		if(sv == "target")   type = TYPE::kTARGET;
		else if(sv == "ioc") type = TYPE::kIOC;
		else                 type = TYPE::kUNKNOWN;
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
std::string_view QwFeedbackConfig::parse_pair_impl(std::string_view token, std::string_view target)
{
	auto res = std::string_view{};
	if( auto targ_pos = token.find(target), sep_pos = token.find('=', targ_pos+1);
		targ_pos!= std::string_view::npos &&
		sep_pos != std::string_view::npos)
	{
		res = token.substr(sep_pos+1);
	}
	return res;
}

bool QwFeedbackConfig::isValid() const
{
	bool valid = false;
	if( (type == TYPE::kTARGET || type == TYPE::kIOC)
	    && !name.empty() ) {
		valid = true;
	}
	return valid;
}

QwFeedbackConfig::TYPE QwFeedbackConfig::getType() const
{
	return type;
}
std::string const& QwFeedbackConfig::getName() const& noexcept { return name; }
std::string&& QwFeedbackConfig::getName() && noexcept { return std::move(name); }
std::string const& QwFeedbackConfig::getDescr() const& noexcept { return descr; }
std::string&& QwFeedbackConfig::getDescr() && noexcept { return std::move(descr); }


std::ostream& operator<<(std::ostream& out, QwFeedbackConfig const& config)
{
	std::string_view type_sv{};
	switch (config.type) {
		case QwFeedbackConfig::TYPE::kTARGET:
			type_sv = "target";
			break;
		case QwFeedbackConfig::TYPE::kIOC:
			type_sv = "ioc";
			break;
		default:
			type_sv = "unknown";
			break;
	}
	out << "Type: " << type_sv << ", Name: " << config.name << ", Descr: " << config.descr << '\n';
	return out;
}

