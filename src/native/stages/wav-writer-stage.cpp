#include "../common.h"

#include "wav-writer-stage.h"

typedef struct WaveHeader
{
	// Riff Wave Header
	char chunkId[4];
	int chunkSize;
	char format[4];

	// Format Subchunk
	char subChunk1Id[4];
	int subChunk1Size;
	short int audioFormat;
	short int numChannels;
	int sampleRate;
	int byteRate;
	short int blockAlign;
	short int bitsPerSample;
	//short int extraParamSize;

	// Data Subchunk
	char subChunk2Id[4];
	int subChunk2Size;

} WaveHeader;

WaveHeader makeWaveHeader(int const sampleRate,
						  short int const numChannels,
						  short int const bitsPerSample)
{
	WaveHeader myHeader;

	// RIFF WAVE Header
	myHeader.chunkId[0] = 'R';
	myHeader.chunkId[1] = 'I';
	myHeader.chunkId[2] = 'F';
	myHeader.chunkId[3] = 'F';
	myHeader.format[0] = 'W';
	myHeader.format[1] = 'A';
	myHeader.format[2] = 'V';
	myHeader.format[3] = 'E';

	// Format subchunk
	myHeader.subChunk1Id[0] = 'f';
	myHeader.subChunk1Id[1] = 'm';
	myHeader.subChunk1Id[2] = 't';
	myHeader.subChunk1Id[3] = ' ';
	myHeader.audioFormat = 1;				// FOR PCM
	myHeader.numChannels = numChannels;		// 1 for MONO, 2 for stereo
	myHeader.sampleRate = sampleRate;		// ie 44100 hertz, cd quality audio
	myHeader.bitsPerSample = bitsPerSample; //
	myHeader.byteRate = myHeader.sampleRate * myHeader.numChannels * myHeader.bitsPerSample / 8;
	myHeader.blockAlign = myHeader.numChannels * myHeader.bitsPerSample / 8;

	// Data subchunk
	myHeader.subChunk2Id[0] = 'd';
	myHeader.subChunk2Id[1] = 'a';
	myHeader.subChunk2Id[2] = 't';
	myHeader.subChunk2Id[3] = 'a';

	// All sizes for later:
	// chuckSize = 4 + (8 + subChunk1Size) + (8 + subChubk2Size)
	// subChunk1Size is constanst, i'm using 16 and staying with PCM
	// subChunk2Size = nSamples * nChannels * bitsPerSample/8
	// Whenever a sample is added:
	//    chunkSize += (nChannels * bitsPerSample/8)
	//    subChunk2Size += (nChannels * bitsPerSample/8)
	myHeader.chunkSize = 4 + 8 + 16 + 8 + 0;
	myHeader.subChunk1Size = 16;
	myHeader.subChunk2Size = 0;

	return myHeader;
}

void WavWriterStage::initialize(PipelineConfig *pipelineConfig,
								PipelineContext *pipelineContext)
{
	unsigned err = fopen_s(&file, pipelineConfig->output.fileName.c_str(), "wb");
	if (err != 0)
	{
		throw std::runtime_error("Failed to open file, error code=" + std::to_string(err));
	}
	WaveHeader header = makeWaveHeader(pipelineContext->samplesPerSecond, pipelineContext->channels, pipelineContext->bitsPerSample);
	fwrite(&header, sizeof(WaveHeader), 1, file);
}

void *WavWriterStage::process(void *data)
{
	unsigned size = ((DataAndSize *)data)->size;
	void *rawData = ((DataAndSize *)data)->rawData;

	fwrite(rawData, 1, size, file);
	return nullptr;
}

void WavWriterStage::shutdown()
{
	if (file)
	{
		fflush(file);
		fclose(file);
	}
}
