/*
 * SDAT - Timer Track
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-11-12
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 *
 * This has been modified in order to be able to provide timing for an SSEQ.
 */

#pragma once

#include <functional>
#include <bitset>
#include "SSEQ.h"
#include "common.h"

const int TRACKSTACKSIZE = 3;

enum { TS_NOTEWAIT, TS_PORTABIT, TS_TIEBIT, TS_END, TS_BITS };

enum { TUF_VOL, TUF_PAN, TUF_TIMER, TUF_MOD, TUF_LEN, TUF_BITS };

struct TimerPlayer;

enum StackType
{
	STACKTYPE_CALL,
	STACKTYPE_LOOP
};

struct StackValue
{
	StackType type;
	uint32_t destPos;

	StackValue() : type(STACKTYPE_CALL), destPos(0) { }
	StackValue(StackType newType, uint32_t newDestPos) : type(newType), destPos(newDestPos) { }
};

struct Override
{
	bool overriding;
	int cmd;
	int value;
	int extraValue;

	Override() : overriding(false) { }
	bool operator()() const { return this->overriding; }
	bool &operator()() { return this->overriding; }
	int val(std::function<int ()> reader, bool returnExtra = false)
	{
		if (this->overriding)
			return returnExtra ? this->extraValue : this->value;
		else
			return reader();
	}
};

struct TimerTrack
{
	int8_t trackId;

	std::bitset<TS_BITS> state;
	uint8_t prio;
	TimerPlayer *ply;

	uint32_t startPos;
	PseudoReadFile file;
	StackValue stack[TRACKSTACKSIZE];
	uint8_t stackPos, loopCount[TRACKSTACKSIZE];
	Override overriding;
	bool lastComparisonResult;

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
	static std::pair<std::vector<uint16_t>, std::vector<uint32_t>> GetPatches(const SSEQ *sseq);
	static std::pair<std::vector<uint16_t>, std::vector<uint32_t>> GetPatches(const std::vector<uint8_t> &data);

	int Read8();
	int Read16();
	int Read24();
	int ReadVL();

	std::function<int ()> read8, read16, read24, readvl;
};
