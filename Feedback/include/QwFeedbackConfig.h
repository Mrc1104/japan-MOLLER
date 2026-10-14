#pragma once
#include <ostream>
#include <string>
#include <string_view>

class QwFeedbackConfig
{
public:
	enum class TYPE	{ kTARGET, kIOC, kUNKNOWN };
private:
	TYPE type{TYPE::kUNKNOWN};
	std::string name;
	std::string descr;
private:
	std::string_view parse_pair_impl(std::string_view token, std::string_view target);
public:
	// Config Parsing
	bool parse_pair(std::string_view token);
	[[nodiscard]] bool isValid() const;
public:
	// Accesors
	TYPE getType() const;
	std::string const& getName() const& noexcept;
	std::string&& getName() && noexcept;
	std::string const& getDescr() const& noexcept;
	std::string&& getDescr() && noexcept;

public:
	// We all need friends
	friend std::ostream& operator<<(std::ostream& out, QwFeedbackConfig const& config);
};


