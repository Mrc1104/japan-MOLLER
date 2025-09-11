#include "QwPitaFeedback.h"
#include "VQwDataHandler.h"
#include "QwParameterFile.h"
#include "QwFactory.h"


// Register this handler with the factory
// RegisterHandlerFactory(QwPitaFeedback);
template<>
const VQwDataHandlerFactory* MQwCloneable<VQwDataHandler,QwPitaFeedback>::fFactory = new QwFactory<VQwDataHandler,QwPitaFeedback>("QwPitaFeedback");

QwPitaFeedback::QwPitaFeedback(const TString& name)
	: VQwDataHandler(name),
	setpoint1("IGL0I00C1068_DAC05"),// For now, we are hardcoding these in
	setpoint2("IGL0I00C1068_DAC06"),// I would like to open them up to be
	setpoint3("IGL0I00C1068_DAC07"),// configurable from mapfiles
	setpoint4("IGL0I00C1068_DAC08"),
	setpoint5("IGL0I00C1068_DAC09"),
	setpoint6("IGL0I00C1068_DAC10"),
	setpoint7("IGL0I00C1068_DAC11"),
	setpoint8("IGL0I00C1068_DAC12"),
	fSlope_IN(0),
	fSlope_OUT(0)
{

	fKeepRunningSum = kTRUE; // We require the Charge Asymmetry
							 // for PITA Feedback
}

QwPitaFeedback::QwPitaFeedback(const QwPitaFeedback &source)
	: VQwDataHandler(source),
	setpoint1(source.setpoint1),// For now, we are hardcoding these in
	setpoint2(source.setpoint2),// I would like to open them up to be
	setpoint3(source.setpoint3),// configurable from mapfiles
	setpoint4(source.setpoint4),
	setpoint5(source.setpoint5),
	setpoint6(source.setpoint6),
	setpoint7(source.setpoint7),
	setpoint8(source.setpoint8),
	fSlope_IN(source.fSlope_IN),
	fSlope_OUT(source.fSlope_OUT) { }

QwPitaFeedback::~QwPitaFeedback() { }

// I need to overload this because the default VQwDataHandler::LoadChannelMao(mapfile)
// just returns 0 (fails quietly)
Int_t QwPitaFeedback::LoadChannelMap(const std::string& mapfile)
{
	// Open the Parameter File
	QwParameterFile map(mapfile);

	// Read the sections of dependent variables
	std::pair<EQwHandleType, std::string> type_name;

	// Loop over mapfile and look for keys
	// Format: [label] [type]_[name]
	//    ie :     dv    asym_tq05r2
	while(map.ReadNextLine()) {
		// Remove comments, spaces, and empty lines.
		map.TrimComment();
		map.TrimWhitespace();
		if(map.LineIsEmpty()) continue;

		// Get first token: dv, iv, ep (dependent, idependent, epics)
		// Get second token [asym, diff, mps]
		std::string primary_token = map.GetNextToken(" ");
		std::string current_token = map.GetNextToken(" ");

		type_name = ParseHandledVariable(current_token);

		if(primary_token == "dv") {
			// The Charge Asymmetry depends on these Parameters
			fDependentType.push_back( type_name.first  );
			fDependentName.push_back( type_name.second );
			fDependentFull.push_back(  current_token   );
		} else {
			QwError << "LoadChannelMan in QwCorrelator read invalid primary token: " << primary_token << QwLog::endl;
		}
	}

	/* For Reassurance -- mrc */
	for(auto const & v : fDependentType)
		QwMessage << v << QwLog::endl;
	for(auto const & v : fDependentName)
		QwMessage << v << QwLog::endl;
	for(auto const & v : fDependentFull)
		QwMessage << v << QwLog::endl;
}

void QwPitaFeedback::ParseConfigFile(QwParameterFile& file)
{
	/* Parameters I need to parse
	 * 	PITA_VOLTAGE_IN
	 * 	PITA_VOLTAGE_IN
	 */
}
