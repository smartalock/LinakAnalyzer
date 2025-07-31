#include "LinakAnalyzer.h"
#include "LinakAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

LinakAnalyzer::LinakAnalyzer()
:	Analyzer2(),  
	mSettings( new LinakAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

LinakAnalyzer::~LinakAnalyzer()
{
	KillThread();
}

void LinakAnalyzer::SetupResults()
{
	mResults.reset( new LinakAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

void LinakAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();

	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

	U32 expected_samples_per_bit = mSampleRateHz / mSettings->mBitRate;


	mResults->CancelPacketAndStartNewPacket();

	// Skip to first idle level data in capture
	// First wait until we are high
	if (mSerial->GetBitState() == BIT_LOW) {
		mSerial->AdvanceToNextEdge();
	}

	do {
		// Attempt to find the SYNC - we should be in the HIGH state

		// Now skip to high -> low transition (start of SYNC)
		if (mSerial->GetBitState() == BIT_HIGH) {
			mSerial->AdvanceToNextEdge();
		}

		U64 syncStartEdge = mSerial->GetSampleNumber();

		// Now skip to low -> high transition (end of SYNC)
		mSerial->AdvanceToNextEdge();

		U64 syncFinishEdge = mSerial->GetSampleNumber();
		U64 syncBits = (syncFinishEdge - syncStartEdge) / expected_samples_per_bit;
		if ((syncBits >= 12) && (syncBits <= 14)) {
			// Calculate the actual bit timing
			U32 samples_per_bit = (syncFinishEdge - syncStartEdge) / 13;

			Frame syncFrame;
			syncFrame.mStartingSampleInclusive = syncStartEdge;
			syncFrame.mEndingSampleInclusive = syncFinishEdge;
			syncFrame.mFlags = 0;
			syncFrame.mType = LIN_FLAG_SYNC;
			syncFrame.mData1 = syncBits; //0x00; // TODO:

			mResults->AddFrame( syncFrame );
			mResults->CommitResults();
			ReportProgress( syncFrame.mEndingSampleInclusive );


			// We are now at end of sync, data is high

			// Process data bytes - start bit (high -> low), 8 data bits, stop bit (high)
			U8 byteNumber = 0;
			U16 chksum = 0;

			bool validByte;
			do {
				validByte = false;
				// Skip to HIGH -> LOW transition (do this on first byte after sync - multi-byte done below)
				if (mSerial->GetBitState() == BIT_HIGH) {
					mSerial->AdvanceToNextEdge();
				}
				U64 frameStart = mSerial->GetSampleNumber();

				U32 samples_to_first_center_of_start_bit = U32(0.5 * (double)samples_per_bit); //U32( 0.5 * double( mSampleRateHz ) / double( mSettings->mBitRate ) );
				mSerial->Advance(samples_to_first_center_of_start_bit);
				U8 data = 0;
				if (mSerial->GetBitState() == BIT_LOW) {
					// Start bit looks good
					mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Start, mSettings->mInputChannel );

					U64 dataStart = mSerial->GetSampleNumber();
					U64 dataEnd = dataStart + 1;
					// Position on LSB
					mSerial->Advance(samples_per_bit);
					for( U32 i=0; i<8; i++ ) {
						mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
						if (mSerial->GetBitState() == BIT_HIGH) {
							data |= (1 << i);
						}

						if (i == 7) {
							dataEnd = mSerial->GetSampleNumber();
						}
						mSerial->Advance(samples_per_bit);
					}
					if (mSerial->GetBitState() == BIT_HIGH) {
						// Stop bit looks good
						mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mInputChannel );
						validByte = true;

						U64 frameEnd = mSerial->GetSampleNumber() + (0.5 * samples_per_bit);

						bool lastFrame = false;

						mSerial->AdvanceToNextEdge(); // We are now positioned on the low bit of sync pulse
						U64 current = mSerial->GetSampleNumber();
						U64 nextEdge = mSerial->GetSampleOfNextEdge();
						U64 lowBits = (nextEdge - current) / samples_per_bit;

						if (lowBits > 9) {
							validByte = false;
							lastFrame = true;
							// Normal end of packet

						} else {

							// Expect next byte for this packet
						}


						// // Do we think there are any more data bytes?
						// U64 current = mSerial->GetSampleNumber();
						// U64 nextEdge = mSerial->GetSampleOfNextEdge();
						// if (nextEdge - current > double(samples_per_bit) * 1.5) {
						// 	validByte = false;
						// 	lastFrame = true;
						// 	// Normal end of packet

						// } else {

						// 	// Expect next byte for this packet
						// }

						U8 flags = 0;
						if (!lastFrame) {
							chksum += data;
							if (chksum > 0xFF) {
								chksum -= 0xFF;
							}
						}
						if (byteNumber == 0) {
							flags = LIN_FLAG_ID;
						} else if (lastFrame) {
							if ((0xFF - chksum) == data) {
								flags = LIN_FLAG_CHK_VALID;
							} else {
								flags = LIN_FLAG_CHK_INVALID;
							}
						}

						Frame dataFrame;
						dataFrame.mStartingSampleInclusive = frameStart; //dataStart;
						dataFrame.mEndingSampleInclusive = frameEnd; //dataEnd;
						dataFrame.mFlags = 0;
						dataFrame.mType = flags;
						dataFrame.mData1 = data;

						mResults->AddFrame( dataFrame );

						if (lastFrame) {
							mResults->CommitPacketAndStartNewPacket();
						}

						mResults->CommitResults();
						ReportProgress( dataFrame.mEndingSampleInclusive );

						byteNumber++;


					} else {
						mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mInputChannel );
//						mResults->CancelPacketAndStartNewPacket();
					}
				} else {
					mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorSquare, mSettings->mInputChannel );
					// ERROR XXX
 					mResults->CancelPacketAndStartNewPacket();
				}

			} while (validByte);
		} else {
			// Re-sync
			mResults->AddMarker( syncFinishEdge, AnalyzerResults::ErrorSquare, mSettings->mInputChannel );
		}

	} while (1);

}

bool LinakAnalyzer::NeedsRerun()
{
	return false;
}

U32 LinakAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 LinakAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mBitRate * 4;
}

const char* LinakAnalyzer::GetAnalyzerName() const
{
	return "Linak LIN Bus";
}

const char* GetAnalyzerName()
{
	return "Linak LIN Bus";
}

Analyzer* CreateAnalyzer()
{
	return new LinakAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}