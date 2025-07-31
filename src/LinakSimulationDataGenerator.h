#ifndef LINAK_SIMULATION_DATA_GENERATOR
#define LINAK_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class LinakAnalyzerSettings;

class LinakSimulationDataGenerator
{
public:
	LinakSimulationDataGenerator();
	~LinakSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, LinakAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	LinakAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //LINAK_SIMULATION_DATA_GENERATOR