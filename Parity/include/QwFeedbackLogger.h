#pragma once
#include <string>
#include <fstream>
#include <memory>
#include "QwLog.h"
class QwFeedbackLogger : public VQwLogger//, public Observer<std::string>
{
	QwLogger<std::unique_ptr<std::ofstream>> fSink;
public:
	void SetSink(std::string const& sink, const std::ios_base::openmode mode = std::ios::app);
	explicit operator bool() const;

public: // VQwLogger Inherited Functions
	void Write(QwLogLevel level, std::string_view log) override;
	QwLogProxy Log(QwLogLevel level, std::string const& msg = "") override;
public: // Observer Inherited Functions
	// void Update(std::string const& data) override;
};


