#ifndef LINAK_ANALYZER_RESULTS
#define LINAK_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define LIN_FLAG_SYNC ( 1 << 0 )
#define LIN_FLAG_ID ( 1 << 1 )
#define LIN_FLAG_CHK_VALID (1 << 2)
#define LIN_FLAG_CHK_INVALID (1 << 3)

class LinakAnalyzer;
class LinakAnalyzerSettings;

class LinakAnalyzerResults : public AnalyzerResults
{
public:
	LinakAnalyzerResults( LinakAnalyzer* analyzer, LinakAnalyzerSettings* settings );
	virtual ~LinakAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	LinakAnalyzerSettings* mSettings;
	LinakAnalyzer* mAnalyzer;
};

#endif //LINAK_ANALYZER_RESULTS
