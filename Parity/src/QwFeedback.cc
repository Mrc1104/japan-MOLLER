#include "QwFeedback.h"

bool QwFeedbackConfig::parse_pair(std::string_view token) {
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
std::string_view QwFeedbackConfig::parse_pair_impl(std::string_view token, std::string_view target)
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

bool QwFeedbackConfig::isValid() const
{
	bool valid = false;
	if( (type == TYPE::kTARGET || type == TYPE::kIOC)
	    && !name.empty() ) {
		valid = true;
	}
	return valid;
}


QwFeedbackConfig::TYPE QwFeedbackConfig::getType() const { return type; }

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

QwFeedback::QwFeedback()
: fPimpl(nullptr)
, fSlope{}
{}

QwFeedback::QwFeedback(QwFeedback const& other)
: fPimpl( other.fPimpl ? other.fPimpl->Clone() : nullptr )
{
}


void QwFeedback::ConfigureFeedbackType(std::string_view sv)
{
	if(sv.find("PITA") != std::string_view::npos) {
		ConfigureFeedbackType(TYPE::PITA);
	}
}
void QwFeedback::ConfigureFeedbackType(TYPE type)
{
	if(fPimpl) std::cout << "WARNING: FeedbackType is already configured! Ignoring...\n";
	switch(type) {
		case TYPE::PITA:
			fPimpl = std::make_unique<QwPITAFeedback>();
			break;
		default:
			break;
	}
	return;
}

void    QwFeedback::SetSlope(IHWP state, double val) { fSlope[state] = val ; }
double  QwFeedback::GetSlope(IHWP state) const       { return fSlope[state]; }
double& QwFeedback::GetSlope(IHWP state)             { return fSlope[state]; }


void QwFeedback::Configure(QwFeedbackConfig const& config)
{
	if(fPimpl) fPimpl->ConfigureImpl(config);
}

void QwFeedback::CalculateCorection(double const running_average)
{
	if(fPimpl) fPimpl->CalculateCorectionImpl(running_average);
}

std::string_view const QwFeedback::RequestTargetDevice() const
{
	return fPimpl ? fPimpl->RequestTargetDeviceImpl() : std::string_view{};
}
std::unique_ptr<VQwFeedbackImpl> QwPITAFeedback::Clone() const
{
	return std::make_unique<QwPITAFeedback>( *this );
}

