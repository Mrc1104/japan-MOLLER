#include <fstream>
#include <regex>
#include <chrono>
#include <algorithm>

// Qweak Headers
#include "QwLog.h"
#include "QwOptions.h"

QwLogProxy QwLogProxy::Log(QwLogLevel level, std::string const& func_sig)
{
	QwLogProxy llog = QwLogProxy{level};
	auto& glogger = QwLog::Instance();

  	// Override log level of this sink when in a debugged function
  	if( glogger.IsDebugFunction(func_sig) ) { level = QwLogLevel::kAlways; }
	if(glogger.PrintWithColor()) { llog << GetLevelColor(level); }

	auto current_time = GetTime();
	llog << '[' << level;
	llog << " | " << std::put_time(std::localtime(&current_time), "%T") << "]: ";

	if(glogger.PrintFuncSignature()) {
		llog << func_sig << "->";
	}

	return llog;
}
std::time_t const QwLogProxy::GetTime()
{
	using namespace std::chrono;
	return system_clock::to_time_t(system_clock::now());
}

QwLogProxy::QwLogProxy(QwLogLevel level)
: fLevel(level)
{ }

QwLogProxy::QwLogProxy(QwLogProxy&& other) noexcept
: fLevel(other.fLevel)
, fBuffer(std::move(other.fBuffer))
{ }

void QwLogProxy::FlushBuffer()
{
	if(fBuffer.tellp() <= 0 ) return;

#if __cplusplus >= 202002L
	QwLog::Instance().Write(fLevel, fBuffer.view());
#else
	// Pre-c++20 incurs a copy
	QwLog::Instance().Write(fLevel, fBuffer.str());
#endif

	// Using an lvalue preserves the internal capacity
	static const std::string kEmptyStr;
	fBuffer.str(kEmptyStr);
	fBuffer.clear();
}

#if (__GNUC__ >= 3)
QwLogProxy& QwLogProxy::operator<<(std::ios_base& (*manip)(std::ios_base&))
{
	// Does not handle std::endl or std::flush properly
	fBuffer << manip;
    return *this;
}
#endif

QwLogProxy& QwLogProxy::operator<<(std::ostream& (*manip)(std::ostream&))
{
	using io_manip = std::ostream&(*)(std::ostream&);
	if( manip == static_cast<io_manip>(std::endl) ) {
		fBuffer << '\n';
		FlushBuffer();
	} else if( manip == static_cast<io_manip>(std::flush) ) {
		FlushBuffer();
	} else {
		fBuffer << manip;
	}
    return *this;
}

QwLogProxy::~QwLogProxy()
{
	std::cout << "Dtor called\n";
	FlushBuffer();
}

QwLog::QwLog()
: fScreenThreshold(QwLogLevel::kMessage)
, fScreen(std::cout)
, fFileThreshold(QwLogLevel::kMessage)
, fFile{nullptr}
, fPrintFunctionSignature{false}
, fUseColor{true}
{ }

QwLog& QwLog::Instance()
{
	static QwLog instance;
	return instance;
}

void QwLog::Write(QwLogLevel level, std::string_view log)
{
	{
		std::lock_guard lk(fScreenMutex);
		if(level >= fScreenThreshold) {
    		fScreen << log;
		}
	}
	{
		std::lock_guard lk(fFileMutex);
		if(fFile && level >= fFileThreshold) {
			*fFile << log;
		}
	}
}

void QwLog::InitLogFile(std::string const& name, const std::ios_base::openmode mode)
{
	std::ios_base::openmode flags = std::ios::out | mode;
	fFile.reset( new std::ofstream(name, flags) );
	fFileThreshold = QwLogLevel::kMessage;
}

/*!
 *  Determine whether the function name matches a specified list of regular expressions
 */
bool QwLog::IsDebugFunction(std::string const& func_sig)
{
	// Using a temporary bool, we avoid acquiring the unique lock twice
	// but risk of running this loop N times for N threads (unlikely)
	bool is_debug_func = false;
	auto opt = fIsDebugFunction.Get(func_sig);
	if( !opt.has_value() ) {
		for (size_t i = 0; i < fDebugFunctionRegexString.size(); i++) {
			// When we find a match, break
			std::regex regex(fDebugFunctionRegexString.at(i));
			if (std::regex_match(func_sig, regex)) {
				is_debug_func = true;
				break;
			}
		}
		// cache it for future lookups
		fIsDebugFunction.InsertOrAssign(func_sig, is_debug_func);
	} else {
		is_debug_func = *opt;
	}
	return is_debug_func;
}

bool QwLog::PrintFuncSignature() const { return fPrintFunctionSignature; }
bool QwLog::PrintWithColor()     const { return fUseColor; }

QwColor GetLevelColor(QwLogLevel level) {
	auto color = QwColor(Qw::kNormal);
	switch (level) {
		case QwLogLevel::kError:   color = QwColor(Qw::kRed); break;
		case QwLogLevel::kWarning: color = QwColor(Qw::kMagenta); break;
		case QwLogLevel::kDebug:   color = QwColor(Qw::kBlue); break;
		default: break;
	}
	return color;
}
std::ostream& operator<<(std::ostream& stream, QwLogLevel level) {
	switch (level) {
		case QwLogLevel::kError:   stream << "ERROR"  ; break;
		case QwLogLevel::kWarning: stream << "WARN"   ; break;
		case QwLogLevel::kMessage: stream << "INFO"   ; break;
		case QwLogLevel::kVerbose: stream << "VERBOSE"; break;
		case QwLogLevel::kDebug:   stream << "DEBUG"  ; break;
		default: break;
	}
	return stream;
}

void QwLog::SetScreenColor(bool flag)
{
	fUseColor = flag;
}

QwLogLevel QwLog::ConvertToEnum(int thr)
{
	auto level = QwLogLevel::kAlways;
	switch(static_cast<QwLogLevel>(thr)) {
		case QwLogLevel::kError:   level = QwLogLevel::kError  ; break; 
		case QwLogLevel::kWarning: level = QwLogLevel::kWarning; break;
		case QwLogLevel::kMessage: level = QwLogLevel::kMessage; break;
		case QwLogLevel::kVerbose: level = QwLogLevel::kVerbose; break;
		case QwLogLevel::kDebug:   level = QwLogLevel::kDebug  ; break;
		default: level = QwLogLevel::kAlways; break;
	}
	return level;
}
void QwLog::SetScreenThreshold(int thr)
{
	fScreenThreshold = ConvertToEnum(thr);
}

void QwLog::SetFileThreshold(int thr)
{
	fFileThreshold = ConvertToEnum(thr);
}

QwLogLevel QwLog::GetLogLevel() const {
	return std::max(fScreenThreshold, fFileThreshold);
};

std::ostream& QwLog::endl(std::ostream& stream) { return std::endl(stream); }


/**
 * Defines configuration options for logging class using QwOptions
 * functionality.
 *
 * Note: this uses a pointer as opposed to a reference, because as indicated
 * above the QwLog class cannot depend on the QwOptions class.  When using a
 * pointer we only need a forward declaration and we do not need to include
 * the header file QwOptions.h.
 *
 * @param options Options object
 */
void QwLog::DefineOptions(QwOptions* options)
{
	// Define the logging options
	options->AddOptions("Logging options")("QwLog.color",
			po::value<bool>()->default_value(true),
			"colored screen output");
	options->AddOptions("Logging options")("QwLog.logfile",
			po::value<std::string>(),
			"log file");
	options->AddOptions("Logging options")("QwLog.loglevel-file",
			po::value<int>()->default_value(static_cast<int>(QwLogLevel::kMessage)),
			"log level for file output");
	options->AddOptions("Logging options")("QwLog.loglevel-screen",
			po::value<int>()->default_value(static_cast<int>(QwLogLevel::kMessage)),
			"log level for screen output");
	options->AddOptions("Logging options")("QwLog.print-signature",
			po::value<bool>()->default_bool_value(false),
			"print signature on error or warning");
	options->AddOptions("Logging options")("QwLog.debug-function",
			po::value< std::vector<std::string> >()->multitoken(),
			"print debugging output of function with signatures satisfying the specified regex");
}

/**
 * Process configuration options for logging class using QwOptions
 * functionality.
 *
 * Note: this uses a pointer as opposed to a reference, because as indicated
 * above the QwLog class cannot depend on the QwOptions class.  When using a
 * pointer we only need a forward declaration and we do not need to include
 * the header file QwOptions.h.
 *
 * @param options Options object
 */
void QwLog::ProcessOptions(QwOptions* options)
{
  // Initialize log file
  if (options->HasValue("QwLog.logfile"))
    InitLogFile(options->GetValue<std::string>("QwLog.logfile"));

  // Set the logging thresholds
  SetFileThreshold(options->GetValue<int>("QwLog.loglevel-file"));
  SetScreenThreshold(options->GetValue<int>("QwLog.loglevel-screen"));

  // Set color flag
  SetScreenColor(options->GetValue<bool>("QwLog.color"));

  // Set the flags for function name and signature printing
  fPrintFunctionSignature = options->GetValue<bool>("QwLog.print-signature");

  // Set the list of regular expressions for functions to debug
  fDebugFunctionRegexString = options->GetValueVector<std::string>("QwLog.debug-function");
  if (fDebugFunctionRegexString.size() > 0)
    std::cout << "Debug regex list:" << std::endl;
  for (size_t i = 0; i < fDebugFunctionRegexString.size(); i++) {
    std::cout << fDebugFunctionRegexString.back() << std::endl;
  }
}
