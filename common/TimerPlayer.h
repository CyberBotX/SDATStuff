/*
 * SDAT - Timer Player structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-23
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 *
 * This has been modified in order to be able to provide timing for an SSEQ.
 */

#pragma once

#include <bitset>
#include "TimerTrack.h"
#include "TimerChannel.h"
#include "SSEQ.h"
#include "SBNK.h"
#include "SWAR.h"
#ifdef _WIN32
# include "windowsh_wrapper.h"
#else
# include <ctime>
# include <cerrno>
# include <pthread.h>
#endif

enum TimeType
{
	LOOP,
	END
};

struct Time
{
	double time;
	TimeType type;

	Time(double tim = 0.0, TimeType typ = LOOP) : time(tim), type(typ)
	{
	}
};

const int TRACKCOUNT = 16;
const int MAXTRACKS = 32;

enum { TYPE_PCM, TYPE_PSG, TYPE_NOISE };

struct TimerPlayer
{
	uint8_t prio, nTracks;
	uint16_t tempo, tempoCount, tempoRate;
	int16_t masterVol, sseqVol;

	TimerTrack tracks[MAXTRACKS];
	std::vector<Time> trackTimes[MAXTRACKS];
	double trailingSilenceSeconds;
	TimerChannel channels[16];
	int16_t variables[32];

	const SSEQ *sseq;
	const SBNK *sbnk;
	const SWAR *swar[4];

	double seconds;

#ifdef _WIN32
	HANDLE mutex, thread;
#else
	pthread_mutex_t mutex;
	pthread_t thread;
#endif
	uint32_t maxSeconds, loops;
	bool doLength, doNotes;
	Time length;

	TimerPlayer();

#ifdef _WIN32
	~TimerPlayer()
	{
		CloseHandle(this->mutex);
	}
#endif

	void Setup(const SSEQ *sseqToPlay);
	int ChannelAlloc(int type, int priority);
	void Run();
	void UpdateTracks();
	Time Length();
	void LockMutex();
	void UnlockMutex();
	void GetLength();

#ifdef _WIN32
	static DWORD WINAPI GetLengthThread(void *handle);
#else
	static void *GetLengthThread(void *handle);
#endif
	void StartLengthThread();
	void WaitForThread();
};
