/*------------------------------------------------------------------------*//*!

 \file QwParity.cc

 \brief main(...) function for the qwanalysis executable

*//*-------------------------------------------------------------------------*/

// System headers
#include <iostream>
#include <fstream>
#include <vector>
#include <new>
#include <memory>

// ROOT headers
#include "Rtypes.h"
#include "TROOT.h"
#include "TFile.h"
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTuple.hxx"
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#endif

// Qweak headers
#include "QwLog.h"
#include "QwRootFile.h"
#include "QwOptionsParity.h"
#include "QwEventBuffer.h"
#ifdef __USE_DATABASE__
#include "QwParityDB.h"
#endif //__USE_DATABASE__
#include "QwHistogramHelper.h"
#include "QwSubsystemArrayParity.h"
#include "QwHelicityPattern.h"
#include "QwEventRing.h"
#include "QwEPICSEvent.h"
#include "QwCombiner.h"
#include "QwCombinerSubsystem.h"
#include "QwPromptSummary.h"
#include "QwCorrelator.h"
#include "LRBCorrector.h"
#include "QwExtractor.h"
#include "QwDataHandlerArray.h"

// Qweak subsystems
// (for correct dependency generation)
#include "QwHelicity.h"
#include "QwFakeHelicity.h"
#include "QwBeamLine.h"
#include "QwBeamMod.h"
#include "QwIntegratedRaster.h"

// Valgrind headers
#if __has_include(<valgrind/callgrind.h>)
#include <valgrind/callgrind.h>
#endif

#include "QwRootFileHandler.h"

Int_t main(Int_t argc, Char_t* argv[])
{
  ///  Enable implicit multi-threading in e.g. TTree::Fill
  ROOT::EnableImplicitMT();

  ///  Define the command line options
  DefineOptionsParity(gQwOptions);

  ///  Define additional command line arguments and the configuration filename,
  ///  and we define the options that can be used in them (using QwOptions).
  gQwOptions.AddOptions()("print-errorcounters", po::value<bool>()->default_bool_value(true), "Print summary of error counters");
  gQwOptions.AddOptions()("write-promptsummary", po::value<bool>()->default_bool_value(false), "Write PromptSummary");
  gQwOptions.AddOptions()("callgrind-instr-start-event-loop", po::value<bool>()->default_bool_value(false), "Start callgrind instrumentation with main event loop (with --instr-atstart=no)");
  gQwOptions.AddOptions()("callgrind-instr-stop-event-loop", po::value<bool>()->default_bool_value(false), "Stop callgrind instrumentation with main event loop (with --instr-atstart=no)");

  ///  Without anything, print usage
  if (argc == 1) {
    gQwOptions.Usage();
    exit(0);
  }

  ///  First, fill the search paths for the parameter files; this sets a
  ///  static variable within the QwParameterFile class which will be used by
  ///  all instances.
  ///  The "scratch" directory should be first.
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QW_PRMINPUT"));
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Parity/prminput");
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Analysis/prminput");

  gQwOptions.SetCommandLine(argc, argv);
  gQwOptions.AddConfigFile("qweak_mysql.conf");

  gQwOptions.ListConfigFiles();

  /// Load command line options for the histogram/tree helper class
  gQwHists.ProcessOptions(gQwOptions);
  /// Setup screen and file logging
  gQwLog.ProcessOptions(&gQwOptions);


  ///  Create the event buffer
  QwEventBuffer eventbuffer;
  eventbuffer.ProcessOptions(gQwOptions);

  ///  Create the database connection
  #ifdef __USE_DATABASE__
  QwParityDB database(gQwOptions);
  #endif //__USE_DATABASE__

  //  QwPromptSummary promptsummary;

  ///  Start loop over all runs
  Int_t run_number = 0;
  while (eventbuffer.OpenNextStream() == CODA_OK) {

    ///  Begin processing for the first run

    run_number = eventbuffer.GetRunNumber();
    TString run_label = eventbuffer.GetRunLabel();

    ///  Set the current event number for parameter file lookup
    QwParameterFile::SetCurrentRunNumber(run_number);
    //  Parse the options again, in case there are run-ranged config files
    gQwOptions.Parse(kTRUE);
    eventbuffer.ProcessOptions(gQwOptions);

    //    if (gQwOptions.GetValue<bool>("write-promptsummary")) {
    QwPromptSummary promptsummary(run_number, eventbuffer.GetSegmentNumber());
    //    }
    ///  Create an EPICS event
    QwEPICSEvent epicsevent;
    epicsevent.ProcessOptions(gQwOptions);
    epicsevent.LoadChannelMap("EpicsTable.map");


    ///  Load the detectors from file
    QwSubsystemArrayParity detectors(gQwOptions);
    detectors.ProcessOptions(gQwOptions);
    detectors.ListPublishedValues();

    /// Create event-based correction subsystem
    //    TString name = "EvtCorrector";
    //    QwCombinerSubsystem corrector_sub(gQwOptions, detectors, name);
    //    detectors.push_back(corrector_sub.GetSharedPointerToStaticObject());
    
    /// Create the helicity pattern
    //    Instead of having run_label in the constructor of helicitypattern, it might
    //    make since to have it be an option for use globally
    QwHelicityPattern helicitypattern(detectors,run_label);
    helicitypattern.ProcessOptions(gQwOptions);
    
    ///  Create the event ring with the subsystem array
    QwEventRing eventring(gQwOptions,detectors);
    //  Make a copy of the detectors object to hold the
    //  events which pass through the ring.
    QwSubsystemArrayParity ringoutput(detectors);

    /// Create the data handler arrays
    QwDataHandlerArray datahandlerarray_evt(gQwOptions,ringoutput,run_label);
    QwDataHandlerArray datahandlerarray_mul(gQwOptions,helicitypattern,run_label);
    QwDataHandlerArray datahandlerarray_burst(gQwOptions,helicitypattern,run_label);

    ///  Create the burst sum
    QwHelicityPattern patternsum_per_burst(helicitypattern);
    patternsum_per_burst.DisablePairs();

    ///  Create the running sum
    QwSubsystemArrayParity eventsum(detectors);
    QwHelicityPattern patternsum(helicitypattern);
    patternsum.DisablePairs();
    QwHelicityPattern burstsum(helicitypattern);
    burstsum.DisablePairs();

    //  Initialize the database connection.
    #ifdef __USE_DATABASE__
    database.SetupOneRun(eventbuffer);
    #endif // __USE_DATABASE__

    //  Open the ROOT file (close when scope ends)
	QwRootFileHandler rootfilehandler{};
	rootfilehandler.ProcessOptions(gQwOptions);
	rootfilehandler.ConstructRootFiles(std::string(run_label.Data()));
	rootfilehandler.WriteParamFileList(std::string(run_label.Data()), detectors);


    detectors.PrintParamFileList();

    #ifdef __USE_DATABASE__
    if (database.AllowsWriteAccess()) {
      database.FillParameterFiles(detectors);
    }
    #endif // __USE_DATABASE__

    //  Construct histograms
	rootfilehandler.ConstructHistograms(QwRootFileHandler::TYPE::HISTO, "evt_histo"  , ringoutput);
	rootfilehandler.ConstructHistograms(QwRootFileHandler::TYPE::HISTO, "mul_histo"  , helicitypattern);
	rootfilehandler.ConstructHistograms(QwRootFileHandler::TYPE::BURST, "burst_histo", patternsum_per_burst);

    detectors.ShareHistograms(ringoutput);

    //  Construct tree branches
    rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::TREE, "evt" , "MPS event data tree"        , ringoutput);
    rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::TREE, "mul" , "Helicity event data tree"   , helicitypattern);
    rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::TREE, "slow", "EPICS and slow control tree", epicsevent);
    rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::BURST,"pr"  , "Pair tree", helicitypattern.GetPairYield()    , "yield_");
    rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::BURST,"pr"  , "Pair tree", helicitypattern.GetPairAsymmetry(), "asym_" );

    rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::BURST,"slow", "EPICS and slow control tree", epicsevent);

	rootfilehandler.ConstructHistograms(QwRootFileHandler::TYPE::HISTO,"evt_histo", datahandlerarray_evt);
	rootfilehandler.ConstructHistograms(QwRootFileHandler::TYPE::HISTO,"mul_histo", datahandlerarray_mul);

	rootfilehandler.ConstructHistograms(QwRootFileHandler::TYPE::BURST,"burst_histo", datahandlerarray_mul);

    datahandlerarray_evt.ConstructTreeBranches(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::TREE), "evt_");
    datahandlerarray_mul.ConstructTreeBranches(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::TREE));
    datahandlerarray_burst.ConstructTreeBranches(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::BURST), "burst_", "|stat");

    // Construct RNTuple fields for data handlers if enabled
#ifdef HAS_RNTUPLE_SUPPORT
    if (gQwOptions.GetValue<bool>("enable-rntuples")) {
      datahandlerarray_evt.ConstructNTupleFields(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::TREE), "evt_");
      datahandlerarray_mul.ConstructNTupleFields(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::TREE));
      datahandlerarray_burst.ConstructNTupleFields(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::BURST), "burst_", "|stat");
    }
#endif

	rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::TREE, "evts", "Running sum tree", eventsum  , "|stat");
	rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::TREE, "muls", "Running sum tree", patternsum, "|stat");

	rootfilehandler.ConstructTreeBranches(QwRootFileHandler::TYPE::BURST, "bursts", "Burst running sum tree", burstsum, "|stat");

    //  Clear the single-event running sum at the beginning of the runlet
    eventsum.ClearEventData();
    patternsum.ClearEventData();
    burstsum.ClearEventData();
    //  Clear the running sum of the burst values at the beginning of the runlet
    helicitypattern.ClearEventData();
    patternsum_per_burst.ClearEventData();



    //  Load the blinder seed from a random number generator for online mode
    if (eventbuffer.IsOnline() ){      
      helicitypattern.UpdateBlinder();//this routine will call update blinder mechanism using a random number
    }else{
      //  Load the blinder seed from the database for this runlet.
#ifdef __USE_DATABASE__
      helicitypattern.UpdateBlinder(&database);
#endif // __USE_DATABASE__      
    }
    

    //  Find the first EPICS event and try to initialize
    //  the blinder, but only for disk files, not online.
    if (! eventbuffer.IsOnline() ){
      QwMessage << "Finding first EPICS event" << QwLog::endl;
      while (eventbuffer.GetNextEvent() == CODA_OK) {
	if (eventbuffer.IsEPICSEvent()) {
	  eventbuffer.FillEPICSData(epicsevent);
	  if (epicsevent.HasDataLoaded()) {
	    helicitypattern.UpdateBlinder(epicsevent);
	    // and break out of this event loop
	    break;
	  }
	}
      }
      epicsevent.ResetCounters();
      //  Rewind stream
      QwMessage << "Rewinding stream" << QwLog::endl;
      eventbuffer.ReOpenStream();
    }

    // Start event loop instrumentation
#ifdef CALLGRIND_START_INSTRUMENTATION
    if (gQwOptions.GetValue<bool>("callgrind-instr-start-event-loop")) {
      QwMessage << "Starting callgrind instrumentation" << QwLog::endl;
      CALLGRIND_START_INSTRUMENTATION;
    }
#endif

    ///  Start loop over events
    while (eventbuffer.GetNextEvent() == CODA_OK) {

      //  First, do processing of non-physics events...
      if (eventbuffer.IsROCConfigurationEvent()) {
        //  Send ROC configuration event data to the subsystem objects.
        eventbuffer.FillSubsystemConfigurationData(detectors);
      }

      //  Secondly, process EPICS events, but not for online running,
      //  because the EPICS events get messed up by our 32-bit to 64-bit
      //  double ET system.
      if (! eventbuffer.IsOnline() && eventbuffer.IsEPICSEvent()) {
        eventbuffer.FillEPICSData(epicsevent);
	if (epicsevent.HasDataLoaded()){
	  epicsevent.CalculateRunningValues();
	  helicitypattern.UpdateBlinder(epicsevent);
	
	  rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::TREE, epicsevent);
	  rootfilehandler.FillTree(QwRootFileHandler::TYPE::TREE, "slow");
	  
	  // Fill RNTuple if enabled
	}
      }


      //  Now, if this is not a physics event, go back and get a new event.
      if (! eventbuffer.IsPhysicsEvent()) continue;


      //  Fill the subsystem objects with their respective data for this event.
      eventbuffer.FillSubsystemData(detectors);

      //  Process the subsystem data
      detectors.ProcessEvent();


      // The event pass the event cut constraints
      if (detectors.ApplySingleEventCuts()) {
	
        // Add event to the ring
        eventring.push(detectors);

        // Check to see ring is ready
        if (eventring.IsReady()) {
	  ringoutput = eventring.pop();
	  ringoutput.IncrementErrorCounters();


	  // Accumulate the running sum to calculate the event based running average
	  eventsum.AccumulateRunningSum(ringoutput);

	  // Fill the histograms
	  rootfilehandler.FillHistograms(QwRootFileHandler::TYPE::HISTO, ringoutput);

	  // Fill mps tree branches
	  rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::TREE, ringoutput);
	  rootfilehandler.FillTree(QwRootFileHandler::TYPE::TREE, "evt");



	  // Process data handlers
          datahandlerarray_evt.ProcessDataHandlerEntry();

          // Fill data handler histograms
		  rootfilehandler.FillHistograms(QwRootFileHandler::TYPE::HISTO, datahandlerarray_evt);

          // Fill data handler tree branches
          datahandlerarray_evt.FillTreeBranches(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::TREE));

          // Fill data handler RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
          if (gQwOptions.GetValue<bool>("enable-rntuples")) {
            datahandlerarray_evt.FillNTupleFields(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::TREE));
          }
#endif

          // Load the event into the helicity pattern
          helicitypattern.LoadEventData(ringoutput);

	  if (helicitypattern.PairAsymmetryIsGood()) {
            patternsum.AccumulatePairRunningSum(helicitypattern);

	    // Fill pair tree branches
		rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::TREE, helicitypattern.GetPairYield()     );
		rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::TREE, helicitypattern.GetPairAsymmetry() );
		rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::TREE, helicitypattern.GetPairDifference());
		rootfilehandler.FillTree(QwRootFileHandler::TYPE::TREE, "pr");

	    
	    // Clear the data
	    helicitypattern.ClearPairData();
	  }

          // Check to see if we can calculate helicity pattern asymmetry, do so, and report if it worked
          if (helicitypattern.IsGoodAsymmetry()) {
              patternsum.AccumulateRunningSum(helicitypattern);

              // Fill histograms
			  rootfilehandler.FillHistograms(QwRootFileHandler::TYPE::HISTO, helicitypattern);

              // Fill helicity tree branches
			  rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::TREE, helicitypattern);
			  rootfilehandler.FillTree(QwRootFileHandler::TYPE::TREE, "mul");

              // Process data handlers
              datahandlerarray_mul.ProcessDataHandlerEntry();
              datahandlerarray_burst.ProcessDataHandlerEntry();

              // Fill data handler histograms
			  rootfilehandler.FillHistograms(QwRootFileHandler::TYPE::HISTO, datahandlerarray_mul);

              // Fill data handler tree branches
              datahandlerarray_mul.FillTreeBranches(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::TREE));

              // Fill data handler RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
              if (gQwOptions.GetValue<bool>("enable-rntuples")) {
                datahandlerarray_mul.FillNTupleFields(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::TREE));
              }
#endif

              // Fill the pattern into the sum for this burst
              patternsum_per_burst.AccumulateRunningSum(helicitypattern);

              // Accumulate data handler arrays
              //datahandlerarray_burst.AccumulateRunningSum(datahandlerarray_mul);

              // Burst mode
              if (patternsum_per_burst.IsEndOfBurst()) {

                // Calculate average over this burst
                patternsum_per_burst.CalculateRunningAverage();

                // Fill the burst into the sum over all bursts
                burstsum.AccumulateRunningSum(patternsum_per_burst);

                if (gQwOptions.GetValue<bool>("print-burstsum")) {
                  QwMessage << " Running average of this burst" << QwLog::endl;
                  QwMessage << " =============================" << QwLog::endl;
                  patternsum_per_burst.PrintValue();
                }

                // Fill histograms
				rootfilehandler.FillHistograms(QwRootFileHandler::TYPE::BURST, patternsum_per_burst);

                // Fill burst tree branches
				rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::BURST, patternsum_per_burst);

				rootfilehandler.FillTree(QwRootFileHandler::TYPE::BURST, "burst");

                // Finish data handler for burst
                datahandlerarray_burst.FinishDataHandler();

                // Fill data handler histograms
				rootfilehandler.FillHistograms(QwRootFileHandler::TYPE::BURST, datahandlerarray_burst);

                // Fill data handler tree branches
                datahandlerarray_burst.FillTreeBranches(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::BURST));

                // Fill data handler RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
                if (gQwOptions.GetValue<bool>("enable-rntuples")) {
                  datahandlerarray_burst.FillNTupleFields(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::BURST));
                }
#endif

		helicitypattern.IncrementBurstCounter();
		datahandlerarray_mul.UpdateBurstCounter(helicitypattern.GetBurstCounter());
		datahandlerarray_burst.UpdateBurstCounter(helicitypattern.GetBurstCounter());
                // Clear the data
                patternsum_per_burst.ClearEventData();
                datahandlerarray_burst.ClearEventData();
              }

              // Clear the data
              helicitypattern.ClearEventData();

	  } // helicitypattern.IsGoodAsymmetry()

        } // eventring.IsReady()

      } // detectors.ApplySingleEventCuts()

    } // end of loop over events
    
    // Unwind event ring
    QwMessage << "Unwinding event ring" << QwLog::endl;
    eventring.Unwind();

    // Stop event loop instrumentation
#ifdef CALLGRIND_START_INSTRUMENTATION
    if (gQwOptions.GetValue<bool>("callgrind-instr-stop-event-loop")) {
      CALLGRIND_STOP_INSTRUMENTATION;
      QwMessage << "Stapped callgrind instrumentation" << QwLog::endl;
    }
#endif

    //  TODO Drain event run

    //  Finalize burst
    if (patternsum_per_burst.HasBurstData()){
      // Calculate average over this burst
      patternsum_per_burst.CalculateRunningAverage();

      // Fill the burst into the sum over all bursts
      burstsum.AccumulateRunningSum(patternsum_per_burst);

      if (gQwOptions.GetValue<bool>("print-burstsum")) {
	QwMessage << " Running average of this burst" << QwLog::endl;
	QwMessage << " =============================" << QwLog::endl;
	patternsum_per_burst.PrintValue();
      }

      // Fill histograms
	  rootfilehandler.FillHistograms(QwRootFileHandler::TYPE::BURST, patternsum_per_burst);

      // Fill burst tree branches
	  rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::BURST, patternsum_per_burst);

	  rootfilehandler.FillTree(QwRootFileHandler::TYPE::BURST, "burst");
    
      // Finish data handler for burst
      datahandlerarray_burst.FinishDataHandler();

      // Fill data handler histograms
	  rootfilehandler.FillHistograms(QwRootFileHandler::TYPE::BURST, datahandlerarray_burst);

      // Fill data handler tree branches
      datahandlerarray_burst.FillTreeBranches(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::BURST));

      // Fill data handler RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
      if (gQwOptions.GetValue<bool>("enable-rntuples")) {
        datahandlerarray_burst.FillNTupleFields(rootfilehandler.GetFilePtr(QwRootFileHandler::TYPE::BURST));
      }
#endif
      patternsum_per_burst.PrintIndexMapFile(run_number);
    }

    //  Perform actions at the end of the event loop on the
    //  detectors object, which ought to have handles for the
    //  MPS based histograms.
    ringoutput.AtEndOfEventLoop();

    QwMessage << "Number of events processed at end of run: "
              << eventbuffer.GetPhysicsEventNumber() << QwLog::endl;

    // Finish data handlers
    datahandlerarray_evt.FinishDataHandler();
    datahandlerarray_mul.FinishDataHandler();

    // Calculate running averages
    eventsum.CalculateRunningAverage();
    patternsum.CalculateRunningAverage();
    burstsum.CalculateRunningAverage();

    // This will calculate running averages over single helicity events
    if (gQwOptions.GetValue<bool>("print-runningsum")) {
      QwMessage << " Running average of events" << QwLog::endl;
      QwMessage << " =========================" << QwLog::endl;
      eventsum.PrintValue();
    }
	rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::TREE, eventsum);
	rootfilehandler.FillTree(QwRootFileHandler::TYPE::TREE, "evts");

    if (gQwOptions.GetValue<bool>("print-patternsum")) {
      QwMessage << " Running average of patterns" << QwLog::endl;
      QwMessage << " =========================" << QwLog::endl;
      patternsum.PrintValue();
    }
	rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::TREE, patternsum);
	rootfilehandler.FillTree(QwRootFileHandler::TYPE::TREE, "muls");

    if (gQwOptions.GetValue<bool>("print-burstsum")) {
      QwMessage << " Running average of bursts" << QwLog::endl;
      QwMessage << " =========================" << QwLog::endl;
      burstsum.PrintValue();
    }
	rootfilehandler.FillTreeBranches(QwRootFileHandler::TYPE::BURST, burstsum);
	rootfilehandler.FillTree(QwRootFileHandler::TYPE::BURST, "bursts");

    //  Construct objects
	rootfilehandler.ConstructObjects(QwRootFileHandler::TYPE::TREE, "objects", helicitypattern);

    /*  Write to the root file */
	rootfilehandler.Finish();

    //  Print the event cut error summary for each subsystem
    if (gQwOptions.GetValue<bool>("print-errorcounters")) {
      QwMessage << " ------------ error counters ------------------ " << QwLog::endl;
      ringoutput.PrintErrorCounters();
    }
    
    if (gQwOptions.GetValue<bool>("write-promptsummary")) {
      //      runningsum.WritePromptSummary(&promptsummary, "yield");
      // runningsum.WritePromptSummary(&promptsummary, "asymmetry");
      //      runningsum.WritePromptSummary(&promptsummary, "difference");
      datahandlerarray_mul.WritePromptSummary(&promptsummary, "asymmetry");
      patternsum.WritePromptSummary(&promptsummary);
      promptsummary.PrintCSV(eventbuffer.GetPhysicsEventNumber(),eventbuffer.GetStartSQLTime(), eventbuffer.GetEndSQLTime());
    }
    //  Read from the database
    #ifdef __USE_DATABASE__
    database.SetupOneRun(eventbuffer);

    // Each subsystem has its own Connect() and Disconnect() functions.
    if (database.AllowsWriteAccess()) {
      patternsum.FillDB(&database);
      patternsum.FillErrDB(&database);
      epicsevent.FillDB(&database);
      ringoutput.FillDB_MPS(&database, "optics");
    }
    #endif // __USE_DATABASE__    
  
    //epicsevent.WriteEPICSStringValues();

    //  Close event buffer stream
    eventbuffer.CloseStream();



    //  Report run summary
    eventbuffer.ReportRunSummary();
    eventbuffer.PrintRunTimes();
  } // end of loop over runs

  QwMessage << "I have done everything I can do..." << QwLog::endl;

  return 0;
}

