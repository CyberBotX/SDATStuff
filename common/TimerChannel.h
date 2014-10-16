/*
 * SDAT - Timer Channel structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-15
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 *
 * Some code/concepts from DeSmuME
 * http://desmume.org/
 *
 * This has been modified in order to be able to provide timing for an SSEQ.
 */

#pragma once

#include <bitset>
#include "SWAV.h"
#include "TimerTrack.h"

inline int Cnv_Sust(int sust)
{
	static const int16_t lut[] =
	{
		-32768, -722, -721, -651, -601, -562, -530, -503,
		-480, -460, -442, -425, -410, -396, -383, -371,
		-360, -349, -339, -330, -321, -313, -305, -297,
		-289, -282, -276, -269, -263, -257, -251, -245,
		-239, -234, -229, -224, -219, -214, -210, -205,
		-201, -196, -192, -188, -184, -180, -176, -173,
		-169, -165, -162, -158, -155, -152, -149, -145,
		-142, -139, -136, -133, -130, -127, -125, -122,
		-119, -116, -114, -111, -109, -106, -103, -101,
		-99, -96, -94, -91, -89, -87, -85, -82,
		-80, -78, -76, -74, -72, -70, -68, -66,
		-64, -62, -60, -58, -56, -54, -52, -50,
		-49, -47, -45, -43, -42, -40, -38, -36,
		-35, -33, -31, -30, -28, -27, -25, -23,
		-22, -20, -19, -17, -16, -14, -13, -11,
		-10, -8, -7, -6, -4, -3, -1, 0
	};

	if (sust & 0x80) // Supposedly invalid value...
		sust = 0x7F; // Use apparently correct default
	return lut[sust];
}

enum { CS_NONE, CS_START, CS_ATTACK, CS_DECAY, CS_SUSTAIN, CS_RELEASE };

enum { CF_UPDVOL, CF_UPDPAN, CF_UPDTMR, CF_BITS };

const uint32_t ARM7_CLOCK = 33513982;

inline int SOUND_FREQ(int n) { return -0x1000000 / n; }

inline uint32_t SOUND_VOL(int n) { return n; }
inline uint32_t SOUND_VOLDIV(int n) { return n << 8; }
inline uint32_t SOUND_PAN(int n) { return n << 16; }
inline uint32_t SOUND_DUTY(int n) { return n << 24; }
const uint32_t SOUND_REPEAT = 1 << 27;
const uint32_t SOUND_ONE_SHOT = 1 << 28;
inline uint32_t SOUND_LOOP(bool a) { return a ? SOUND_REPEAT : SOUND_ONE_SHOT; }
const uint32_t SOUND_FORMAT_PSG = 3 << 29;
inline uint32_t SOUND_FORMAT(int n) { return n << 29; }
const uint32_t SCHANNEL_ENABLE = 1 << 31;

/*
 * This structure is meant to be similar to what is stored in the actual
 * Nintendo DS's sound registers.  Items that were not being used by this
 * player have been removed, and items which help the simulated registers
 * have been added.
 */
struct NDSSoundRegister
{
	// Control Register
	uint8_t volumeMul;
	uint8_t volumeDiv;
	uint8_t panning;
	uint8_t waveDuty;
	uint8_t repeatMode;
	uint8_t format;
	bool enable;

	// Data Source Register
	const SWAV *source;

	// Timer Register
	uint16_t timer;

	// PSG Handling, not a DS register
	uint16_t psgX;
	int16_t psgLast;
	uint32_t psgLastCount;

	// The following are taken from DeSmuME
	double samplePosition;
	double sampleIncrease;

	// Loopstart Register
	uint32_t loopStart;

	// Length Register
	uint32_t length;

	uint32_t totalLength;

	NDSSoundRegister();

	void ClearControlRegister();
	void SetControlRegister(uint32_t reg);
};

/*
 * From FeOS Sound System, this is temporary storage of what will go into
 * the Nintendo DS sound registers.  It is kept separate as the original code
 * from FeOS Sound System utilized this to hold data prior to passing it into
 * the DS's registers.
 */
struct TempSndReg
{
	uint32_t CR;
	const SWAV *SOURCE;
	uint16_t TIMER;
	uint32_t REPEAT_POINT, LENGTH;

	TempSndReg();
};

struct TimerPlayer;

struct TimerChannel
{
	int8_t chnId;

	TempSndReg tempReg;
	uint8_t state;
	int8_t trackId; // -1 = none
	uint8_t prio;
	bool manualSweep;

	std::bitset<CF_BITS> flags;
	int8_t pan; // -64 .. 63
	int16_t extAmpl;

	int16_t velocity;
	int8_t extPan;
	uint8_t key;

	int ampl; // 7 fractionary bits
	int extTune; // in 64ths of a semitone

	uint8_t orgKey;

	uint8_t modType, modSpeed, modDepth, modRange;
	uint16_t modDelay, modDelayCnt, modCounter;

	uint32_t sweepLen, sweepCnt;
	int16_t sweepPitch;

	uint8_t attackLvl, sustainLvl;
	uint16_t decayRate, releaseRate;

	/*
	 * These were originally global variables in FeOS Sound System, but
	 * since they were linked to a certain channel anyways, I moved them
	 * into this class.
	 */
	int noteLength;
	uint16_t vol;

	const TimerPlayer *ply;
	NDSSoundRegister reg;

	TimerChannel();

	void UpdateVol(const TimerTrack &trk);
	void UpdatePan(const TimerTrack &trk);
	void UpdateTune(const TimerTrack &trk);
	void UpdateMod(const TimerTrack &trk);
	void UpdatePorta(const TimerTrack &trk);
	void Release();
	void Kill();
	void UpdateTrack();
	void Update();
	int32_t GenerateSample();
	void IncrementSample();
};
