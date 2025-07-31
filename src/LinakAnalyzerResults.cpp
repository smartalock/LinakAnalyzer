#include "LinakAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "LinakAnalyzer.h"
#include "LinakAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <sstream>

LinakAnalyzerResults::LinakAnalyzerResults( LinakAnalyzer* analyzer, LinakAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

LinakAnalyzerResults::~LinakAnalyzerResults()
{
}

void LinakAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	char number_str[128];
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	if( ( frame.mType & LIN_FLAG_SYNC ) != 0 ) {
		AddResultString( "SYNC" );

	} else {
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		if ( ( frame.mType & LIN_FLAG_ID ) != 0 ) {

			std::stringstream ss;
			ss << "ID: " << number_str;
			AddResultString( ss.str().c_str() );			

		} else if ( ( frame.mType & LIN_FLAG_CHK_VALID ) != 0 ) {
			std::stringstream ss;
			ss << "CHK: " << number_str;
			AddResultString( ss.str().c_str() );			

		} else if ( ( frame.mType & LIN_FLAG_CHK_INVALID ) != 0 ) {
			std::stringstream ss;
			ss << "!CHK: " << number_str;
			AddResultString( ss.str().c_str() );			

		} else {
			AddResultString( number_str );
		}
	
	}
}

void LinakAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	char buf[200] = {0};
	char time_str[128] = {0};
	char byte_buf[20];

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame(i);

		if (frame.mType & LIN_FLAG_SYNC) {
			// Write out
			if (strlen(buf) > 0) {
				file_stream << time_str << "," << buf << std::endl;
			}

			// Start next buffer
			snprintf(buf, sizeof(buf), "SYNC ");
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
		} else {
			if (frame.mType & LIN_FLAG_ID ) {
				snprintf(byte_buf, sizeof(byte_buf), "ID: %02X ", (uint32_t)frame.mData1);
			} else if (frame.mType & LIN_FLAG_CHK_VALID) {
				snprintf(byte_buf, sizeof(byte_buf), "CHK: %02X ", (uint32_t)frame.mData1);
			} else if (frame.mType & LIN_FLAG_CHK_INVALID) {
				snprintf(byte_buf, sizeof(byte_buf), "CHK: !%02X ", (uint32_t)frame.mData1);
			} else {
				snprintf(byte_buf, sizeof(byte_buf), "%02X ", (uint32_t)frame.mData1);
			}
			if (strlen(buf) + strlen(byte_buf) < sizeof(buf) - 5) {
				strcat(buf, byte_buf);
			} else {
				// OVERFLOW!
			}
		}

		if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true) {
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void LinakAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	Frame frame = GetFrame( frame_index );
	ClearTabularText();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	AddTabularText( number_str );
#endif
}

void LinakAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not supported

}

void LinakAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}