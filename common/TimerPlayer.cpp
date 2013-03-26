/*
 * SDAT - Timer Player structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 *
 * This has been modified in order to be able to provide timing for an SSEQ.
 */

#include "TimerPlayer.h"

TimerPlayer::TimerPlayer() : prio(0), nTracks(0), tempo(120), tempoCount(0), tempoRate(0x100), masterVol(0), trailingSilenceSeconds(0), sseq(NULL), sbnk(NULL), seconds(0),
#ifdef _WIN32
	mutex(CreateMutex(NULL, false, NULL)), thread(NULL),
#else
	mutex(PTHREAD_MUTEX_INITIALIZER), thread(0),
#endif
	maxSeconds(0), loops(0), doLength(false), doNotes(false), length()
{
	memset(this->swar, 0, sizeof(this->swar));
	for (int i = 0; i < 16; ++i)
		this->channels[i].chnId = i;
}

void TimerPlayer::Setup(const SSEQ *sseqToPlay)
{
	this->sseq = sseqToPlay;

	PseudoReadFile file((this->sseq->info.origFilename));
	file.GetDataFromVector(this->sseq->data.begin(), this->sseq->data.end());

	this->tracks[0].Init(0, this, file);

	this->nTracks = 1;

	if ((*file.data)[0] == 0xFE)
		for (file.pos += 3; (*file.data)[file.pos] == 0x93; )
		{
			++file.pos;
			file.ReadLE<uint8_t>();
			PseudoReadFile trackFile((this->sseq->info.origFilename));
			trackFile.GetDataFromVector(this->sseq->data.begin(), this->sseq->data.end());
			trackFile.pos = file.Read24();
			int newTrack = this->nTracks++;
			this->tracks[newTrack].Init(newTrack, this, trackFile);
		}

	this->tracks[0].file = file;
	this->tracks[0].startPos = file.pos;
}

int TimerPlayer::ChannelAlloc(int type, int priority)
{
	static const uint8_t pcmChnArray[] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
	static const uint8_t psgChnArray[] = { 13, 12, 11, 10, 9, 8 };
	static const uint8_t noiseChnArray[] = { 15, 14 };
	static const uint8_t arraySizes[] = { sizeof(pcmChnArray), sizeof(psgChnArray), sizeof(noiseChnArray) };
	static const uint8_t *const arrayArray[] = { pcmChnArray, psgChnArray, noiseChnArray };

	const uint8_t *const chnArray = arrayArray[type];
	int arraySize = arraySizes[type];

	int curChnNo = -1;
	for (int i = 0; i < arraySize; ++i)
	{
		int thisChnNo = chnArray[i];
		TimerChannel &thisChn = this->channels[thisChnNo];
		TimerChannel &curChn = this->channels[curChnNo];
		if (curChnNo != -1 && thisChn.prio >= curChn.prio)
		{
			if (thisChn.prio != curChn.prio)
				continue;
			if (curChn.vol <= thisChn.vol)
				continue;
		}
		curChnNo = thisChnNo;
	}

	if (curChnNo == -1 || priority < this->channels[curChnNo].prio)
		return -1;
	this->channels[curChnNo].ply = this;
	this->channels[curChnNo].noteLength = -1;
	this->channels[curChnNo].vol = 0;
	return curChnNo;
}

const double SecondsPerClockCycle = 64.0 * 2728.0 / ARM7_CLOCK;

void TimerPlayer::Run()
{
	while (this->tempoCount > 240)
	{
		this->tempoCount -= 240;
		for (uint8_t i = 0; i < this->nTracks; ++i)
		{
			this->tracks[i].Run();

			if (this->tracks[i].hitLoop)
			{
				this->trackTimes[i].push_back(Time(this->seconds, LOOP));
				this->tracks[i].hitLoop = false;
			}
			if (this->tracks[i].hitEnd)
			{
				this->trackTimes[i].push_back(Time(this->seconds, END));
				this->tracks[i].hitEnd = false;
			}
		}
	}
	this->tempoCount += (static_cast<int>(this->tempo) * static_cast<int>(this->tempoRate)) >> 8;

	this->seconds += SecondsPerClockCycle;
}

void TimerPlayer::UpdateTracks()
{
	for (int i = 0; i < 16; ++i)
		this->channels[i].UpdateTrack();
	for (int i = 0; i < MAXTRACKS; ++i)
		this->tracks[i].updateFlags.reset();
}

Time TimerPlayer::Length()
{
	uint32_t tracksLooped = 0, tracksEnded = 0;
	double len = -1;
	TimeType lastType = LOOP;
	for (uint8_t i = 0; i < this->nTracks; ++i)
	{
		auto &times = this->trackTimes[i];
		if (times.empty())
			continue;
		auto &time = times[times.size() - 1];
		if (time.type == LOOP && times.size() >= this->loops)
			++tracksLooped;
		else if (time.type == END)
			++tracksEnded;
		if (time.time > len)
		{
			len = time.time;
			lastType = time.type;
		}
	}
	if (tracksLooped == this->nTracks)
		return Time(len, LOOP);
	else if (tracksEnded == this->nTracks)
		return Time(len, END);
	else if (tracksLooped + tracksEnded == this->nTracks)
		return Time(len, lastType);
	return Time(-1, LOOP);
}

void TimerPlayer::LockMutex()
{
#ifdef _WIN32
	WaitForSingleObject(this->mutex, INFINITE);
#else
	pthread_mutex_lock(&this->mutex);
#endif
}

void TimerPlayer::UnlockMutex()
{
#ifdef _WIN32
	ReleaseMutex(this->mutex);
#else
	pthread_mutex_unlock(&this->mutex);
#endif
}

template<typename T1, typename T2> static inline void clamp(T1 &valueToClamp, const T2 &minValue, const T2 &maxValue)
{
	if (valueToClamp < minValue)
		valueToClamp = minValue;
	else if (valueToClamp > maxValue)
		valueToClamp = maxValue;
}

static inline int32_t muldiv7(int32_t val, uint8_t mul)
{
	return mul == 127 ? val : (val * mul) >> 7;
}

void TimerPlayer::GetLength()
{
	bool success = false;
	try
	{
		this->length = Time();
		for (;;)
		{
			this->LockMutex();
			bool doingLength = this->doLength;
			this->UnlockMutex();
			if (!doingLength)
			{
				this->length = Time(-1, LOOP);
				return;
			}

			if (this->doNotes)
			{
				int32_t leftChannel = 0, rightChannel = 0;

				// I need to advance the sound channels here
				for (int i = 0; i < 16; ++i)
				{
					TimerChannel &chn = this->channels[i];

					if (chn.state > CS_NONE)
					{
						int32_t sample = chn.GenerateSample();
						chn.IncrementSample();

						uint8_t datashift = chn.reg.volumeDiv;
						if (datashift == 3)
							datashift = 4;
						sample = muldiv7(sample, chn.reg.volumeMul) >> datashift;

						leftChannel += muldiv7(sample, 127 - chn.reg.panning);
						rightChannel += muldiv7(sample, chn.reg.panning);
					}
				}

				leftChannel = muldiv7(leftChannel, 127 - this->masterVol);
				rightChannel = muldiv7(rightChannel, 127 - this->masterVol);

				clamp(leftChannel, -0x8000, 0x7FFF);
				clamp(rightChannel, -0x8000, 0x7FFF);

				if (!leftChannel && !rightChannel)
					this->trailingSilenceSeconds += SecondsPerClockCycle;
				else if (trailingSilenceSeconds > 0)
					this->trailingSilenceSeconds = 0;

				this->UpdateTracks();

				for (int i = 0; i < 16; ++i)
					this->channels[i].Update();
			}

			this->Run();

			if (this->doNotes && this->trailingSilenceSeconds >= 20.0)
			{
				double time = this->seconds - this->trailingSilenceSeconds;
				this->length = Time(time < 0 ? 0 : time, END);
				success = true;
				break;
			}

			if (!this->doNotes)
			{
				this->length = this->Length();
				if (static_cast<int>(this->length.time) != -1)
				{
					success = true;
					break;
				}
			}
			if (this->seconds > maxSeconds)
				break;
		}
		this->LockMutex();
		this->doLength = false;
		this->UnlockMutex();
	}
	catch (const std::exception &)
	{
		success = false;
	}
	if (!success)
		this->length = Time(-1, LOOP);
}

#ifdef _WIN32
DWORD WINAPI TimerPlayer::GetLengthThread(void *handle)
#else
void *TimerPlayer::GetLengthThread(void *handle)
#endif
{
	TimerPlayer *player = reinterpret_cast<TimerPlayer *>(handle);
	player->GetLength();
#ifdef _WIN32
	return 0;
#else
	return NULL;
#endif
}

void TimerPlayer::StartLengthThread()
{
	this->doLength = true;
#ifdef _WIN32
	DWORD threadID;
	this->thread = CreateThread(NULL, 0, TimerPlayer::GetLengthThread, this, 0, &threadID);
#else
	pthread_create(&this->thread, NULL, TimerPlayer::GetLengthThread, this);
#endif
}

void TimerPlayer::WaitForThread()
{
#ifdef _WIN32
	WaitForSingleObject(this->thread, INFINITE);
#else
	pthread_join(this->thread, NULL);
#endif
}
