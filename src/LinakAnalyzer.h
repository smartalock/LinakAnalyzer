#ifndef LINAK_ANALYZER_H
#define LINAK_ANALYZER_H

#include <Analyzer.h>
#include "LinakAnalyzerResults.h"
#include "LinakSimulationDataGenerator.h"

class LinakAnalyzerSettings;
class ANALYZER_EXPORT LinakAnalyzer : public Analyzer2
{
public:
	LinakAnalyzer();
	virtual ~LinakAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< LinakAnalyzerSettings > mSettings;
	std::auto_ptr< LinakAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	LinakSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //LINAK_ANALYZER_H
