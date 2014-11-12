/*
 * SDAT - Timer Track structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-23
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 *
 * This has been modified in order to be able to provide timing for an SSEQ.
 */

#include "TimerTrack.h"
#include "TimerPlayer.h"

static inline int Cnv_Attack(int attk)
{
	static const uint8_t lut[] =
	{
		0x00, 0x01, 0x05, 0x0E, 0x1A, 0x26, 0x33, 0x3F, 0x49, 0x54,
		0x5C, 0x64, 0x6D, 0x74, 0x7B, 0x7F, 0x84, 0x89, 0x8F
	};

	if (attk & 0x80) // Supposedly invalid value...
		attk = 0; // Use apparently correct default
	return attk >= 0x6D ? lut[0x7F - attk] : 0xFF - attk;
}

static inline int Cnv_Fall(int fall)
{
	if (fall & 0x80) // Supposedly invalid value...
		fall = 0; // Use apparently correct default
	if (fall == 0x7F)
		return 0xFFFF;
	else if (fall == 0x7E)
		return 0x3C00;
	else if (fall < 0x32)
		return ((fall << 1) + 1) & 0xFFFF;
	else
		return (0x1E00 / (0x7E - fall)) & 0xFFFF;
}

TimerTrack::TimerTrack() : trackId(-1), state(), prio(0), ply(nullptr), startPos(0), file(), stackPos(0), overriding(), lastComparisonResult(false), wait(0), patch(0), portaKey(0), portaTime(0),
	sweepPitch(0), vol(0), expr(0), pan(0), pitchBendRange(0), pitchBend(0), transpose(0), a(0), d(0), s(0), r(0), modType(0), modSpeed(0), modDepth(0), modRange(0), modDelay(0), updateFlags(),
	hitLoop(false), hitEnd(false)
{
	std::fill_n(&this->stack[0], TRACKSTACKSIZE, StackValue());
	memset(this->loopCount, 0, sizeof(this->loopCount));
	this->read8 = std::bind(&TimerTrack::Read8, this);
	this->read16 = std::bind(&TimerTrack::Read16, this);
	this->read24 = std::bind(&TimerTrack::Read24, this);
	this->readvl = std::bind(&TimerTrack::ReadVL, this);
}

// Original FSS Function: Track_ClearState
void TimerTrack::ClearState()
{
	this->state.reset();
	this->state.set(TS_NOTEWAIT);
	this->prio = this->ply->prio + 64;

	this->stackPos = 0;

	this->wait = 0;
	this->patch = 0;
	this->portaKey = 60;
	this->portaTime = 0;
	this->sweepPitch = 0;
	this->vol = this->expr = 127;
	this->pan = 0;
	this->pitchBendRange = 2;
	this->pitchBend = this->transpose = 0;

	this->a = this->d = this->s = this->r = 0xFF;

	this->modType = 0;
	this->modRange = 1;
	this->modSpeed = 16;
	this->modDelay = 0;
	this->modDepth = 0;

	this->hitLoop = this->hitEnd = false;
}

// Original FSS Function: Player_InitTrack
void TimerTrack::Init(uint8_t handle, TimerPlayer *player, const PseudoReadFile &source)
{
	this->trackId = handle;
	this->ply = player;
	this->file.GetDataFromVector(source.data->begin(), source.data->end());
	this->file.pos = this->startPos = source.pos;
	this->ClearState();
}

// Original FSS Function: Note_On
int TimerTrack::NoteOn(int key, int vel, int len)
{
	auto sbnk = this->ply->sbnk;

	if (this->patch >= sbnk->instruments.size())
		return -1;

	bool bIsPCM = true;
	TimerChannel *chn;
	int nCh = -1;

	auto &instrument = sbnk->instruments[this->patch];
	const SBNKInstrumentRange *noteDef = nullptr;
	int fRecord = instrument.record;

	if (fRecord == 16)
	{
		if (!(instrument.ranges[0].lowNote <= key && key <= instrument.ranges[instrument.ranges.size() - 1].highNote))
			return -1;
		int rn = key - instrument.ranges[0].lowNote;
		noteDef = &instrument.ranges[rn];
		fRecord = noteDef->record;
	}
	else if (fRecord == 17)
	{
		size_t reg, ranges;
		for (reg = 0, ranges = instrument.ranges.size(); reg < ranges; ++reg)
			if (key <= instrument.ranges[reg].highNote)
				break;
		if (reg == ranges)
			return -1;

		noteDef = &instrument.ranges[reg];
		fRecord = noteDef->record;
	}

	if (!fRecord)
		return -1;
	else if (fRecord == 1)
	{
		if (!noteDef)
			noteDef = &instrument.ranges[0];
	}
	else if (fRecord < 4)
	{
		// PSG
		// fRecord = 2 -> PSG tone, pNoteDef->wavid -> PSG duty
		// fRecord = 3 -> PSG noise
		bIsPCM = false;
		if (!noteDef)
			noteDef = &instrument.ranges[0];
		if (fRecord == 3)
		{
			nCh = this->ply->ChannelAlloc(TYPE_NOISE, this->prio);
			if (nCh < 0)
				return -1;
			chn = &this->ply->channels[nCh];
			chn->tempReg.CR = SOUND_FORMAT_PSG | SCHANNEL_ENABLE;
		}
		else
		{
			nCh = this->ply->ChannelAlloc(TYPE_PSG, this->prio);
			if (nCh < 0)
				return -1;
			chn = &this->ply->channels[nCh];
			chn->tempReg.CR = SOUND_FORMAT_PSG | SCHANNEL_ENABLE | SOUND_DUTY(noteDef->swav & 0x7);
		}
		// TODO: figure out what pNoteDef->tnote means for PSG channels
		chn->tempReg.TIMER = -SOUND_FREQ(440 * 8); // key #69 (A4)
		chn->reg.samplePosition = -1;
		chn->reg.psgX = 0x7FFF;
	}

	if (bIsPCM)
	{
		nCh = this->ply->ChannelAlloc(TYPE_PCM, this->prio);
		if (nCh < 0)
			return -1;
		chn = &this->ply->channels[nCh];

		auto swav = &this->ply->swar[noteDef->swar]->swavs.find(noteDef->swav)->second;
		chn->tempReg.CR = SOUND_FORMAT(swav->waveType & 3) | SOUND_LOOP(!!swav->loop) | SCHANNEL_ENABLE;
		chn->tempReg.SOURCE = swav;
		chn->tempReg.TIMER = swav->time;
		chn->tempReg.REPEAT_POINT = swav->loopOffset;
		chn->tempReg.LENGTH = swav->nonLoopLength;
		chn->reg.samplePosition = -3;
	}

	chn->state = CS_START;
	chn->trackId = this->trackId;
	chn->flags.reset();
	chn->prio = this->prio;
	chn->key = key;
	chn->orgKey = bIsPCM ? noteDef->noteNumber : 69;
	chn->velocity = Cnv_Sust(vel);
	chn->pan = static_cast<int>(noteDef->pan) - 64;
	chn->modDelayCnt = 0;
	chn->modCounter = 0;
	chn->noteLength = len;
	chn->reg.sampleIncrease = 0;

	chn->attackLvl = Cnv_Attack(this->a == 0xFF ? noteDef->attackRate : this->a);
	chn->decayRate = Cnv_Fall(this->d == 0xFF ? noteDef->decayRate : this->d);
	chn->sustainLvl = this->s == 0xFF ? noteDef->sustainLevel : this->s;
	chn->releaseRate = Cnv_Fall(this->r == 0xFF ? noteDef->releaseRate : this->r);

	chn->UpdateVol(*this);
	chn->UpdatePan(*this);
	chn->UpdateTune(*this);
	chn->UpdateMod(*this);
	chn->UpdatePorta(*this);

	this->portaKey = key;

	return nCh;
}

// Original FSS Function: Note_On_Tie
int TimerTrack::NoteOnTie(int key, int vel)
{
	// Find an existing note
	int i;
	TimerChannel *chn = nullptr;
	for (i = 0; i < 16; ++i)
	{
		chn = &this->ply->channels[i];
		if (chn->state > CS_NONE && chn->trackId == this->trackId && chn->state != CS_RELEASE)
			break;
	}

	if (i == 16)
		// Can't find note -> create an endless one
		return this->NoteOn(key, vel, -1);

	chn->flags.reset();
	chn->prio = this->prio;
	chn->key = key;
	chn->velocity = Cnv_Sust(vel);
	chn->modDelayCnt = 0;
	chn->modCounter = 0;

	chn->UpdateVol(*this);
	//chn->UpdatePan(*this);
	chn->UpdateTune(*this);
	chn->UpdateMod(*this);
	chn->UpdatePorta(*this);

	this->portaKey = key;
	chn->flags.set(CF_UPDTMR);

	return i;
}

// Original FSS Function: Track_ReleaseAllNotes
void TimerTrack::ReleaseAllNotes()
{
	for (int i = 0; i < 16; ++i)
	{
		TimerChannel &chn = this->ply->channels[i];
		if (chn.state > CS_NONE && chn.trackId == this->trackId && chn.state != CS_RELEASE)
			chn.Release();
	}
}

enum SseqCommand
{
	SSEQ_CMD_ALLOCTRACK = 0xFE, // Silently ignored
	SSEQ_CMD_OPENTRACK = 0x93,

	SSEQ_CMD_REST = 0x80,
	SSEQ_CMD_PATCH = 0x81,
	SSEQ_CMD_PAN = 0xC0,
	SSEQ_CMD_VOL = 0xC1,
	SSEQ_CMD_MASTERVOL = 0xC2,
	SSEQ_CMD_PRIO = 0xC6,
	SSEQ_CMD_NOTEWAIT = 0xC7,
	SSEQ_CMD_TIE = 0xC8,
	SSEQ_CMD_EXPR = 0xD5,
	SSEQ_CMD_TEMPO = 0xE1,
	SSEQ_CMD_END = 0xFF,

	SSEQ_CMD_GOTO = 0x94,
	SSEQ_CMD_CALL = 0x95,
	SSEQ_CMD_RET = 0xFD,
	SSEQ_CMD_LOOPSTART = 0xD4,
	SSEQ_CMD_LOOPEND = 0xFC,

	SSEQ_CMD_TRANSPOSE = 0xC3,
	SSEQ_CMD_PITCHBEND = 0xC4,
	SSEQ_CMD_PITCHBENDRANGE = 0xC5,

	SSEQ_CMD_ATTACK = 0xD0,
	SSEQ_CMD_DECAY = 0xD1,
	SSEQ_CMD_SUSTAIN = 0xD2,
	SSEQ_CMD_RELEASE = 0xD3,

	SSEQ_CMD_PORTAKEY = 0xC9,
	SSEQ_CMD_PORTAFLAG = 0xCE,
	SSEQ_CMD_PORTATIME = 0xCF,
	SSEQ_CMD_SWEEPPITCH = 0xE3,

	SSEQ_CMD_MODDEPTH = 0xCA,
	SSEQ_CMD_MODSPEED = 0xCB,
	SSEQ_CMD_MODTYPE = 0xCC,
	SSEQ_CMD_MODRANGE = 0xCD,
	SSEQ_CMD_MODDELAY = 0xE0,

	SSEQ_CMD_RANDOM = 0xA0,
	SSEQ_CMD_PRINTVAR = 0xD6,
	SSEQ_CMD_IF = 0xA2,
	SSEQ_CMD_FROMVAR = 0xA1,
	SSEQ_CMD_SETVAR = 0xB0,
	SSEQ_CMD_ADDVAR = 0xB1,
	SSEQ_CMD_SUBVAR = 0xB2,
	SSEQ_CMD_MULVAR = 0xB3,
	SSEQ_CMD_DIVVAR = 0xB4,
	SSEQ_CMD_SHIFTVAR = 0xB5,
	SSEQ_CMD_RANDVAR = 0xB6,
	SSEQ_CMD_CMP_EQ = 0xB8,
	SSEQ_CMD_CMP_GE = 0xB9,
	SSEQ_CMD_CMP_GT = 0xBA,
	SSEQ_CMD_CMP_LE = 0xBB,
	SSEQ_CMD_CMP_LT = 0xBC,
	SSEQ_CMD_CMP_NE = 0xBD,

	SSEQ_CMD_MUTE = 0xD7 // Unsupported
};

static const uint8_t VariableByteCount = 1 << 7;
static const uint8_t ExtraByteOnNoteOrVarOrCmp = 1 << 6;

static inline uint8_t SseqCommandByteCount(int cmd)
{
	if (cmd < 0x80)
		return 1 | VariableByteCount;
	else
		switch (cmd)
		{
			case SSEQ_CMD_REST:
			case SSEQ_CMD_PATCH:
				return VariableByteCount;

			case SSEQ_CMD_PAN:
			case SSEQ_CMD_VOL:
			case SSEQ_CMD_MASTERVOL:
			case SSEQ_CMD_PRIO:
			case SSEQ_CMD_NOTEWAIT:
			case SSEQ_CMD_TIE:
			case SSEQ_CMD_EXPR:
			case SSEQ_CMD_LOOPSTART:
			case SSEQ_CMD_TRANSPOSE:
			case SSEQ_CMD_PITCHBEND:
			case SSEQ_CMD_PITCHBENDRANGE:
			case SSEQ_CMD_ATTACK:
			case SSEQ_CMD_DECAY:
			case SSEQ_CMD_SUSTAIN:
			case SSEQ_CMD_RELEASE:
			case SSEQ_CMD_PORTAKEY:
			case SSEQ_CMD_PORTAFLAG:
			case SSEQ_CMD_PORTATIME:
			case SSEQ_CMD_MODDEPTH:
			case SSEQ_CMD_MODSPEED:
			case SSEQ_CMD_MODTYPE:
			case SSEQ_CMD_MODRANGE:
			case SSEQ_CMD_PRINTVAR:
			case SSEQ_CMD_MUTE:
				return 1;

			case SSEQ_CMD_ALLOCTRACK:
			case SSEQ_CMD_TEMPO:
			case SSEQ_CMD_SWEEPPITCH:
			case SSEQ_CMD_MODDELAY:
				return 2;

			case SSEQ_CMD_GOTO:
			case SSEQ_CMD_CALL:
			case SSEQ_CMD_SETVAR:
			case SSEQ_CMD_ADDVAR:
			case SSEQ_CMD_SUBVAR:
			case SSEQ_CMD_MULVAR:
			case SSEQ_CMD_DIVVAR:
			case SSEQ_CMD_SHIFTVAR:
			case SSEQ_CMD_RANDVAR:
			case SSEQ_CMD_CMP_EQ:
			case SSEQ_CMD_CMP_GE:
			case SSEQ_CMD_CMP_GT:
			case SSEQ_CMD_CMP_LE:
			case SSEQ_CMD_CMP_LT:
			case SSEQ_CMD_CMP_NE:
				return 3;

			case SSEQ_CMD_OPENTRACK:
				return 4;

			case SSEQ_CMD_FROMVAR:
				return 1 | ExtraByteOnNoteOrVarOrCmp; // Technically 2 bytes with an additional 1, leaving 1 off because we will be reading it to determine if the additional byte is needed

			case SSEQ_CMD_RANDOM:
				return 4 | ExtraByteOnNoteOrVarOrCmp; // Technically 5 bytes with an additional 1, leaving 1 off because we will be reading it to determine if the additional byte is needed

			default:
				return 0;
		}
}

static auto varFuncSet = [](int16_t, int16_t value) { return value; };
static auto varFuncAdd = [](int16_t var, int16_t value) -> int16_t { return var + value; };
static auto varFuncSub = [](int16_t var, int16_t value) -> int16_t { return var - value; };
static auto varFuncMul = [](int16_t var, int16_t value) -> int16_t { return var * value; };
static auto varFuncDiv = [](int16_t var, int16_t value) -> int16_t { return var / value; };
static auto varFuncShift = [](int16_t var, int16_t value) -> int16_t
{
	if (value < 0)
		return var >> -value;
	else
		return var << value;
};
static auto varFuncRand = [](int16_t, int16_t value) -> int16_t
{
	if (value < 0)
		return -(std::rand() % (-value + 1));
	else
		return std::rand() % (value + 1);
};

static inline std::function<int16_t (int16_t, int16_t)> VarFunc(int cmd)
{
	switch (cmd)
	{
		case SSEQ_CMD_SETVAR:
			return varFuncSet;
		case SSEQ_CMD_ADDVAR:
			return varFuncAdd;
		case SSEQ_CMD_SUBVAR:
			return varFuncSub;
		case SSEQ_CMD_MULVAR:
			return varFuncMul;
		case SSEQ_CMD_DIVVAR:
			return varFuncDiv;
		case SSEQ_CMD_SHIFTVAR:
			return varFuncShift;
		case SSEQ_CMD_RANDVAR:
			return varFuncRand;
		default:
			return nullptr;
	}
}

static auto compareFuncEq = [](int16_t a, int16_t b) { return a == b; };
static auto compareFuncGe = [](int16_t a, int16_t b) { return a >= b; };
static auto compareFuncGt = [](int16_t a, int16_t b) { return a > b; };
static auto compareFuncLe = [](int16_t a, int16_t b) { return a <= b; };
static auto compareFuncLt = [](int16_t a, int16_t b) { return a < b; };
static auto compareFuncNe = [](int16_t a, int16_t b) { return a != b; };

static inline std::function<bool (int16_t, int16_t)> CompareFunc(int cmd)
{
	switch (cmd)
	{
		case SSEQ_CMD_CMP_EQ:
			return compareFuncEq;
		case SSEQ_CMD_CMP_GE:
			return compareFuncGe;
		case SSEQ_CMD_CMP_GT:
			return compareFuncGt;
		case SSEQ_CMD_CMP_LE:
			return compareFuncLe;
		case SSEQ_CMD_CMP_LT:
			return compareFuncLt;
		case SSEQ_CMD_CMP_NE:
			return compareFuncNe;
		default:
			return nullptr;
	}
}

// Original FSS Function: Track_Run
void TimerTrack::Run()
{
	// Indicate "heartbeat" for this track
	this->updateFlags.set(TUF_LEN);

	// Exit if the track has already ended
	if (this->state[TS_END])
		return;

	if (this->wait)
	{
		--this->wait;
		if (this->wait)
			return;
	}

	while (!this->wait)
	{
		this->ply->LockMutex();
		bool doingLength = this->ply->doLength;
		this->ply->UnlockMutex();
		if (!doingLength)
			break;

		int cmd;
		if (this->overriding())
			cmd = this->overriding.cmd;
		else
			cmd = this->Read8();
		if (cmd < 0x80)
		{
			// Note on
			int key = cmd + this->transpose;
			int vel = this->overriding.val(this->read8, true);
			int len = this->overriding.val(this->readvl);
			if (this->state[TS_NOTEWAIT])
				this->wait = len;
			if (this->ply->doNotes)
			{
				if (this->state[TS_TIEBIT])
					this->NoteOnTie(key, vel);
				else
					this->NoteOn(key, vel, len);
			}
		}
		else
		{
			int value;
			switch (cmd)
			{
				//-----------------------------------------------------------------
				// Main commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_OPENTRACK:
				{
					this->Read8();
					PseudoReadFile trackFile = this->file;
					trackFile.pos = this->Read24();
					int newTrack = this->ply->nTracks++;
					this->ply->tracks[newTrack].Init(newTrack, this->ply, trackFile);
					break;
				}

				case SSEQ_CMD_REST:
					this->wait = this->overriding.val(this->readvl);
					break;

				case SSEQ_CMD_PATCH:
					this->patch = this->overriding.val(this->readvl);
					break;

				case SSEQ_CMD_GOTO:
					this->file.pos = this->Read24();
					this->hitLoop = true;
					break;

				case SSEQ_CMD_CALL:
					value = this->Read24();
					if (this->stackPos < TRACKSTACKSIZE)
					{
						this->stack[this->stackPos++] = StackValue(STACKTYPE_CALL, this->file.pos);
						this->file.pos = value;
					}
					break;

				case SSEQ_CMD_RET:
					if (this->stackPos && this->stack[this->stackPos - 1].type == STACKTYPE_CALL)
						this->file.pos = this->stack[--this->stackPos].destPos;
					break;

				case SSEQ_CMD_PAN:
					this->pan = this->overriding.val(this->read8) - 64;
					this->updateFlags.set(TUF_PAN);
					break;

				case SSEQ_CMD_VOL:
					this->vol = this->overriding.val(this->read8);
					this->updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_MASTERVOL:
					this->ply->masterVol = Cnv_Sust(this->overriding.val(this->read8));
					for (uint8_t i = 0; i < this->ply->nTracks; ++i)
						this->ply->tracks[i].updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_PRIO:
					this->prio = this->ply->prio + this->Read8();
					break;

				case SSEQ_CMD_NOTEWAIT:
					this->state.set(TS_NOTEWAIT, !!this->Read8());
					break;

				case SSEQ_CMD_TIE:
					this->state.set(TS_TIEBIT, !!this->Read8());
					this->ReleaseAllNotes();
					break;

				case SSEQ_CMD_EXPR:
					this->expr = this->overriding.val(this->read8);
					this->updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_TEMPO:
					this->ply->tempo = this->Read16();
					break;

				case SSEQ_CMD_END:
					this->state.set(TS_END);
					this->hitEnd = true;
					return;

				case SSEQ_CMD_LOOPSTART:
					value = this->overriding.val(this->read8);
					if (this->stackPos < TRACKSTACKSIZE)
					{
						this->loopCount[this->stackPos] = value;
						this->stack[this->stackPos++] = StackValue(STACKTYPE_LOOP, this->file.pos);
					}
					break;

				case SSEQ_CMD_LOOPEND:
					if (this->stackPos && this->stack[this->stackPos - 1].type == STACKTYPE_LOOP)
					{
						uint32_t rPos = this->stack[this->stackPos - 1].destPos;
						uint8_t &nR = this->loopCount[this->stackPos - 1];
						uint8_t prevR = nR;
						if (!prevR || --nR)
							this->file.pos = rPos;
						else
							--this->stackPos;
						if (!prevR)
							this->hitLoop = true;
					}
					break;

				//-----------------------------------------------------------------
				// Tuning commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_TRANSPOSE:
					this->transpose = this->overriding.val(this->read8);
					break;

				case SSEQ_CMD_PITCHBEND:
					this->pitchBend = this->overriding.val(this->read8);
					this->updateFlags.set(TUF_TIMER);
					break;

				case SSEQ_CMD_PITCHBENDRANGE:
					this->pitchBendRange = this->Read8();
					this->updateFlags.set(TUF_TIMER);
					break;

				//-----------------------------------------------------------------
				// Envelope-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_ATTACK:
					this->a = this->overriding.val(this->read8);
					break;

				case SSEQ_CMD_DECAY:
					this->d = this->overriding.val(this->read8);
					break;

				case SSEQ_CMD_SUSTAIN:
					this->s = this->overriding.val(this->read8);
					break;

				case SSEQ_CMD_RELEASE:
					this->r = this->overriding.val(this->read8);
					break;

				//-----------------------------------------------------------------
				// Portamento-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_PORTAKEY:
					this->portaKey = this->Read8() + this->transpose;
					this->state.set(TS_PORTABIT);
					break;

				case SSEQ_CMD_PORTAFLAG:
					this->state.set(TS_PORTABIT, !!this->Read8());
					break;

				case SSEQ_CMD_PORTATIME:
					this->portaTime = this->overriding.val(this->read8);
					break;

				case SSEQ_CMD_SWEEPPITCH:
					this->sweepPitch = this->overriding.val(this->read16);
					break;

				//-----------------------------------------------------------------
				// Modulation-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_MODDEPTH:
					this->modDepth = this->overriding.val(this->read8);
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODSPEED:
					this->modSpeed = this->overriding.val(this->read8);
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODTYPE:
					this->modType = this->Read8();
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODRANGE:
					this->modRange = this->Read8();
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODDELAY:
					this->modDelay = this->overriding.val(this->read16);
					this->updateFlags.set(TUF_MOD);
					break;

				//-----------------------------------------------------------------
				// Randomness-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_RANDOM:
				{
					this->overriding() = true;
					this->overriding.cmd = this->Read8();
					if ((this->overriding.cmd >= SSEQ_CMD_SETVAR && this->overriding.cmd <= SSEQ_CMD_CMP_NE) || this->overriding.cmd < 0x80)
						this->overriding.extraValue = this->Read8();
					int16_t minVal = this->Read16();
					int16_t maxVal = this->Read16();
					// Special case: If the overriden command is a Note-On command, just use whatever could've been the maximum for it.
					if (this->overriding.cmd < 0x80)
						this->overriding.value = maxVal;
					else
						this->overriding.value = (std::rand() % (maxVal - minVal + 1)) + minVal;
					break;
				}

				//-----------------------------------------------------------------
				// Variable-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_FROMVAR:
					this->overriding() = true;
					this->overriding.cmd = this->Read8();
					if ((this->overriding.cmd >= SSEQ_CMD_SETVAR && this->overriding.cmd <= SSEQ_CMD_CMP_NE) || this->overriding.cmd < 0x80)
						this->overriding.extraValue = this->Read8();
					this->overriding.value = this->ply->variables[this->Read8()];
					break;

				case SSEQ_CMD_SETVAR:
				case SSEQ_CMD_ADDVAR:
				case SSEQ_CMD_SUBVAR:
				case SSEQ_CMD_MULVAR:
				case SSEQ_CMD_DIVVAR:
				case SSEQ_CMD_SHIFTVAR:
				case SSEQ_CMD_RANDVAR:
				{
					int8_t varNo = this->overriding.val(this->read8, true);
					value = this->overriding.val(this->read16);
					if (cmd == SSEQ_CMD_DIVVAR && !value) // Division by 0, skip it to prevent crashing
						break;
					this->ply->variables[varNo] = VarFunc(cmd)(this->ply->variables[varNo], value);
					break;
				}

				//-----------------------------------------------------------------
				// Conditional-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_CMP_EQ:
				case SSEQ_CMD_CMP_GE:
				case SSEQ_CMD_CMP_GT:
				case SSEQ_CMD_CMP_LE:
				case SSEQ_CMD_CMP_LT:
				case SSEQ_CMD_CMP_NE:
				{
					int8_t varNo = this->overriding.val(this->read8, true);
					value = this->overriding.val(this->read16);
					this->lastComparisonResult = CompareFunc(cmd)(this->ply->variables[varNo], value);
					break;
				}

				case SSEQ_CMD_IF:
					if (!this->lastComparisonResult)
					{
						int nextCmd = this->Read8();
						uint8_t cmdBytes = SseqCommandByteCount(nextCmd);
						bool variableBytes = !!(cmdBytes & VariableByteCount);
						bool extraByte = !!(cmdBytes & ExtraByteOnNoteOrVarOrCmp);
						cmdBytes &= ~(VariableByteCount | ExtraByteOnNoteOrVarOrCmp);
						if (extraByte)
						{
							int extraCmd = this->Read8();
							if ((extraCmd >= SSEQ_CMD_SETVAR && extraCmd <= SSEQ_CMD_CMP_NE) || extraCmd < 0x80)
								++cmdBytes;
						}
						this->file.pos += cmdBytes;
						if (variableBytes)
							this->ReadVL();
					}
					break;

				default:
					this->file.pos += SseqCommandByteCount(cmd);
			}
		}

		if (cmd != SSEQ_CMD_RANDOM && cmd != SSEQ_CMD_FROMVAR)
			this->overriding() = false;
	}
}

std::pair<std::vector<uint16_t>, std::vector<uint32_t>> TimerTrack::GetPatches(const SSEQ *sseq)
{
	std::vector<uint16_t> patches;
	std::vector<uint32_t> positions;

	PseudoReadFile file(sseq->info.origFilename);
	file.GetDataFromVector(sseq->data.begin(), sseq->data.end());

	uint32_t dataSize = sseq->data.size();

	uint8_t tracks = 1;
	uint8_t finishedTracks = 0;
	while (file.pos < dataSize)
	{
		int cmd = file.ReadLE<uint8_t>();
		switch (cmd)
		{
			case SSEQ_CMD_OPENTRACK:
				file.pos += 4;
				++tracks;
				break;
			case SSEQ_CMD_PATCH:
				positions.push_back(file.pos);
				patches.push_back(file.ReadVL());
				break;
			case SSEQ_CMD_END:
				++finishedTracks;
				break;
			default:
			{
				uint8_t cmdBytes = SseqCommandByteCount(cmd);
				bool variableBytes = !!(cmdBytes & VariableByteCount);
				bool extraByte = !!(cmdBytes & ExtraByteOnNoteOrVarOrCmp);
				cmdBytes &= ~(VariableByteCount | ExtraByteOnNoteOrVarOrCmp);
				if (extraByte)
				{
					int extraCmd = file.ReadLE<uint8_t>();
					if ((extraCmd >= SSEQ_CMD_SETVAR && extraCmd <= SSEQ_CMD_CMP_NE) || extraCmd < 0x80)
						++cmdBytes;
				}
				file.pos += cmdBytes;
				if (variableBytes)
					file.ReadVL();
			}
		}
		if (finishedTracks == tracks)
			break;
	}

	return std::make_pair(patches, positions);
}

int TimerTrack::Read8()
{
	return this->file.ReadLE<uint8_t>();
}

int TimerTrack::Read16()
{
	return this->file.ReadLE<uint16_t>();
}

int TimerTrack::Read24()
{
	return this->file.Read24();
}

int TimerTrack::ReadVL()
{
	return this->file.ReadVL();
}
