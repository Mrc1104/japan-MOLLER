/*!
 * \file   QwLog.h
 * \brief  A logfile class, based on an identical class in the Hermes analyzer
 *
 * \author Wouter Deconinck
 * \date   2009-11-25
 */

#pragma once
#include <iostream>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <vector>
#include <map>
#include <chrono>
#include "QwColor.h"

/*!
 * \note Because QwOptions depends on QwLog, and QwLog depends also on QwOptions,
 * we cannot include QwOptions in the QwLog header file here.  QwLog is treated
 * as more basic than QwOptions.
 */

// Forward declarations
class QwOptions;


/*! \def QwOut
 *  \brief Predefined log drain for explicit output
 */
#define QwOut      QwLogProxy::Log(QwLogLevel::kAlways,__PRETTY_FUNCTION__)

/*! \def QwError
 *  \brief Predefined log drain for errors
 */
#define QwError    QwLogProxy::Log(QwLogLevel::kError,__PRETTY_FUNCTION__)

/*! \def QwWarning
 *  \brief Predefined log drain for warnings
 */
#define QwWarning  QwLogProxy::Log(QwLogLevel::kWarning,__PRETTY_FUNCTION__)

/*! \def QwMessage
 *  \brief Predefined log drain for regular messages
 */
#define QwMessage  QwLogProxy::Log(QwLogLevel::kMessage,__PRETTY_FUNCTION__)

/*! \def QwVerbose
 *  \brief Predefined log drain for verbose messages
 */
#define QwVerbose  if (QwLog::Instance().GetLogLevel() >= QwLogLevel::kVerbose) QwLogProxy::Log(QwLogLevel::kVerbose,__PRETTY_FUNCTION__)

/*! \def QwDebug
 *  \brief Predefined log drain for debugging output
 */
#define QwDebug    if (QwLog::Instance().GetLogLevel() >= QwLogLevel::kDebug) QwLogProxy::Log(QwLogLevel::kDebug,__PRETTY_FUNCTION__)

//! Loglevels
/*! enum of possible log levels */
enum class QwLogLevel : int {
	kAlways    = -1, /*!< Explicit output  */
	kError     =  0, /*!< Error loglevel   */
	kWarning   =  1, /*!< Warning loglevel */
	kMessage   =  2, /*!< Message loglevel */
	kVerbose   =  3, /*!< Verbose loglevel */
	kDebug     =  4  /*!< Debug loglevel   */
};
QwColor GetLevelColor(QwLogLevel level);
std::ostream& operator<<(std::ostream& stream, QwLogLevel level);

template<typename Key, typename Value>
class QwThreadSafeMap
{
private:
	std::map<Key, Value> fMap;
	mutable std::shared_mutex fRW_mutex;
public:
	void InsertOrAssign(Key const& key, Value const& value);
	std::optional<Value> Get(Key const& key) const;
	bool Find(Key const& key) const;
	void Remove(Key const& key);
};



class QwLogProxy
{
	QwLogLevel fLevel;
	std::ostringstream fBuffer;
	void FlushBuffer();
	QwLogProxy(QwLogLevel level);
	QwLogProxy(QwLogProxy&& other) noexcept;
	QwLogProxy& operator=(QwLogProxy && other) = delete;
	QwLogProxy(QwLogProxy const& other) = delete;
	QwLogProxy& operator=(QwLogProxy const& other) = delete;
	static std::time_t const GetTime();
public:
	static QwLogProxy Log(QwLogLevel level, std::string const& func_sig  = "<unknown>");
	~QwLogProxy();
	template<typename T>
	QwLogProxy& operator<<(T const& val);
#if (__GNUC__ >= 3)
	QwLogProxy& operator<<(std::ios_base& (*manip)(std::ios_base&));
#endif
	QwLogProxy& operator<<(std::ostream& (*manip)(std::ostream&));
};



class QwLog
{

    //! File thresholds and stream
	mutable std::mutex fScreenMutex;
    QwLogLevel    fScreenThreshold;
	std::ostream& fScreen;

    //! File thresholds and stream
	mutable std::mutex fFileMutex;
    QwLogLevel    fFileThreshold;
	std::unique_ptr<std::ostream> fFile;

    //! List of regular expressions for functions that will have increased log level
    QwThreadSafeMap<std::string,bool> fIsDebugFunction;
    std::vector<std::string> fDebugFunctionRegexString;

    //! Flag to print function signature on warning or error
    bool fPrintFunctionSignature;
    //! Flag to disable color
    bool fUseColor;

	QwLogLevel ConvertToEnum(int thr);

	QwLog();
public:
	static QwLog& Instance();
	void Write(QwLogLevel level, std::string_view log);

	/* Confuguration Accessors */
    /// \brief Define available class options for QwOptions
    static void DefineOptions(QwOptions* options);

    /// \brief Process class options for QwOptions
    // Note: this uses pointers as opposed to references, because as indicated
    // above the QwLog class cannot depend on the QwOptions class.  When using a
    // pointer we only need a forward declaration and we do not need to include
    // the header file QwOptions.h.
    void ProcessOptions(QwOptions* options);

	/*! Initialize the log file with name 'name'
	*/
	void InitLogFile(std::string const& name, const std::ios_base::openmode mode = std::ios::app);
	bool IsDebugFunction(std::string const& func_sig);
	bool PrintFuncSignature() const;

    /*! \brief Set the screen color mode
     */
    void SetScreenColor(bool flag);
	bool PrintWithColor() const;

    /*! \brief Set the screen log level
     */
    void SetScreenThreshold(int thr);
    /*! \brief Set the file log level
     */
    void SetFileThreshold(int thr);

    /*! \brief Get highest log level
     */
    QwLogLevel GetLogLevel() const;

	/*! \def QwLog::endl
	 *  \brief Backward compat.
	 */
	static std::ostream& endl(std::ostream& stream);

};

template<typename T>
QwLogProxy& QwLogProxy::operator<<(T const& val)
{
	fBuffer << val;
	return *this;
}

template<typename Key, typename Value>
void QwThreadSafeMap<Key, Value>::InsertOrAssign(Key const& key, Value const& value)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	fMap[key] = value;
}
template<typename Key, typename Value>
std::optional<Value> QwThreadSafeMap<Key, Value>::Get(Key const& key) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	auto it = fMap.find(key);
	return (it == fMap.end()) ? std::nullopt : std::optional(it->second);
}
template<typename Key, typename Value>
bool QwThreadSafeMap<Key, Value>::Find(Key const& key) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	auto it = fMap.find(key);
	return (it == fMap.end()) ? false : true;
}
template<typename Key, typename Value>
void QwThreadSafeMap<Key, Value>::Remove(Key const& key)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	fMap.erase(key);
}


