/*
 * SDAT - Timer Track
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 *
 * This has been modified in order to be able to provide timing for an SSEQ.
 */

#ifndef SDAT_TIMERTRACK_H
#define SDAT_TIMERTRACK_H

#include <bitset>
#include "common.h"

const int TRACKSTACKSIZE = 3;

enum { TS_NOTEWAIT, TS_PORTABIT, TS_TIEBIT, TS_END, TS_BITS };

enum { TUF_VOL, TUF_PAN, TUF_TIMER, TUF_MOD, TUF_LEN, TUF_BITS };

struct TimerPlayer;

struct TimerTrack
{
	int8_t trackId;

	std::bitset<TS_BITS> state;
	uint8_t prio;
	TimerPlayer *ply;

	uint32_t startPos;
	PseudoReadFile file;
	uint32_t stack[TRACKSTACKSIZE];
	uint8_t stackPos, loopCount[TRACKSTACKSIZE];

	int wait;
	uint16_t patch;
	uint8_t portaKey, portaTime;
	int16_t sweepPitch;
	uint8_t vol, expr;
	int8_t pan;
	uint8_t pitchBendRange;
	int8_t pitchBend, transpose;

	uint8_t a, d, s, r;

	uint8_t modType, modSpeed, modDepth, modRange;
	uint16_t modDelay;

	std::bitset<TUF_BITS> updateFlags;

	bool hitLoop, hitEnd;

	TimerTrack();

	void ClearState();
	void Init(uint8_t handle, TimerPlayer *player, const PseudoReadFile &source);
	int NoteOn(int key, int vel, int len);
	int NoteOnTie(int key, int vel);
	void ReleaseAllNotes();
	void Run();
};

#endif
