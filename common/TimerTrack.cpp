/*
 * SDAT - Timer Track structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
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

	return attk >= 0x6D ? lut[0x7F - attk] : 0xFF - attk;
}

static inline int Cnv_Fall(int fall)
{
	if (fall == 0x7F)
		return 0xFFFF;
	else if (fall == 0x7E)
		return 0x3C00;
	else if (fall < 0x32)
		return ((fall << 1) + 1) & 0xFFFF;
	else
		return (0x1E00 / (0x7E - fall)) & 0xFFFF;
}

TimerTrack::TimerTrack() : trackId(-1), state(), prio(0), ply(NULL), startPos(0), file(), stackPos(0), wait(0), patch(0), portaKey(0), portaTime(0), sweepPitch(0), vol(0),
	expr(0), pan(0), pitchBendRange(0), pitchBend(0), transpose(0), a(0), d(0), s(0), r(0), modType(0), modSpeed(0), modDepth(0), modRange(0), modDelay(0), updateFlags(),
	hitLoop(false), hitEnd(false)
{
	memset(this->stack, 0, sizeof(this->stack));
	memset(this->loopCount, 0, sizeof(this->loopCount));
}

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
	this->vol = 64;
	this->expr = 127;
	this->pan = 0;
	this->pitchBendRange = 2;
	this->pitchBend = this->transpose = 0;

	this->a = this->d = this->s = this->r = 0xFF;

	this->modType = 0;
	this->modRange = 1;
	this->modSpeed = 16;
	this->modDelay = 10;
	this->modDepth = 0;

	this->hitLoop = this->hitEnd = false;
}

void TimerTrack::Init(uint8_t handle, TimerPlayer *player, const PseudoReadFile &source)
{
	this->trackId = handle;
	this->ply = player;
	this->file.GetDataFromVector(source.data->begin(), source.data->end());
	this->file.pos = this->startPos = source.pos;
	this->ClearState();
}

int TimerTrack::NoteOn(int key, int vel, int len)
{
	auto sbnk = this->ply->sbnk;

	if (this->patch >= sbnk->instruments.size())
		return -1;

	bool bIsPCM = true;
	TimerChannel *chn;
	int nCh;

	auto &instrument = sbnk->instruments[this->patch];
	const SBNKInstrumentRange *noteDef = NULL;
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

int TimerTrack::NoteOnTie(int key, int vel)
{
	// Find an existing note
	int i;
	TimerChannel *chn;
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
	SSEQ_CMD_UNSUP1 = 0xA1,
	SSEQ_CMD_UNSUP2_LO = 0xB0,
	SSEQ_CMD_UNSUP2_HI = 0xBD
};

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

		int cmd = this->file.ReadLE<uint8_t>();
		if (cmd < 0x80)
		{
			// Note on
			int key = cmd + this->transpose;
			int vel = this->file.ReadLE<uint8_t>();
			int len = this->file.ReadVL();
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
			switch (cmd)
			{
				//-----------------------------------------------------------------
				// Main commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_REST:
					this->wait = this->file.ReadVL();
					break;

				case SSEQ_CMD_PATCH:
					this->patch = this->file.ReadVL();
					break;

				case SSEQ_CMD_GOTO:
					this->file.pos = this->file.Read24();
					this->hitLoop = true;
					break;

				case SSEQ_CMD_CALL:
				{
					uint32_t newPos = this->file.Read24();
					this->stack[this->stackPos++] = this->file.pos;
					this->file.pos = newPos;
					break;
				}

				case SSEQ_CMD_RET:
					this->file.pos = this->stack[--this->stackPos];
					break;

				case SSEQ_CMD_PAN:
					this->pan = this->file.ReadLE<uint8_t>() - 64;
					this->updateFlags.set(TUF_PAN);
					break;

				case SSEQ_CMD_VOL:
					this->vol = this->file.ReadLE<uint8_t>();
					this->updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_MASTERVOL:
					this->ply->masterVol = Cnv_Sust(this->file.ReadLE<uint8_t>());
					for (uint8_t i = 0; i < this->ply->nTracks; ++i)
						this->ply->tracks[i].updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_PRIO:
					this->prio = this->ply->prio + this->file.ReadLE<uint8_t>();
					break;

				case SSEQ_CMD_NOTEWAIT:
					this->state.set(TS_NOTEWAIT, !!this->file.ReadLE<uint8_t>());
					break;

				case SSEQ_CMD_TIE:
					this->state.set(TS_TIEBIT, !!this->file.ReadLE<uint8_t>());
					this->ReleaseAllNotes();
					break;

				case SSEQ_CMD_EXPR:
					this->expr = this->file.ReadLE<uint8_t>();
					this->updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_TEMPO:
					this->ply->tempo = this->file.ReadLE<uint16_t>();
					break;

				case SSEQ_CMD_END:
					this->state.set(TS_END);
					this->hitEnd = true;
					return;

				case SSEQ_CMD_LOOPSTART:
					this->loopCount[this->stackPos] = this->file.ReadLE<uint8_t>();
					this->stack[this->stackPos++] = this->file.pos;
					break;

				case SSEQ_CMD_LOOPEND:
				{
					if (this->stackPos)
					{
						uint32_t rPos = this->stack[this->stackPos - 1];
						uint8_t &nR = this->loopCount[this->stackPos - 1];
						uint8_t prevR = nR;
						if (prevR && !--nR)
							--this->stackPos;
						if (!prevR)
							this->hitLoop = true;
						this->file.pos = rPos;
					}
					break;
				}

				//-----------------------------------------------------------------
				// Tuning commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_TRANSPOSE:
					this->transpose = this->file.ReadLE<uint8_t>();
					break;

				case SSEQ_CMD_PITCHBEND:
					this->pitchBend = this->file.ReadLE<uint8_t>();
					this->updateFlags.set(TUF_TIMER);
					break;

				case SSEQ_CMD_PITCHBENDRANGE:
					this->pitchBendRange = this->file.ReadLE<uint8_t>();
					this->updateFlags.set(TUF_TIMER);
					break;

				//-----------------------------------------------------------------
				// Envelope-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_ATTACK:
					this->a = this->file.ReadLE<uint8_t>();
					break;

				case SSEQ_CMD_DECAY:
					this->d = this->file.ReadLE<uint8_t>();
					break;

				case SSEQ_CMD_SUSTAIN:
					this->s = this->file.ReadLE<uint8_t>();
					break;

				case SSEQ_CMD_RELEASE:
					this->r = this->file.ReadLE<uint8_t>();
					break;

				//-----------------------------------------------------------------
				// Portamento-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_PORTAKEY:
					this->portaKey = this->file.ReadLE<uint8_t>() + this->transpose;
					this->state.set(TS_PORTABIT);
					break;

				case SSEQ_CMD_PORTAFLAG:
					this->state.set(TS_PORTABIT, !!this->file.ReadLE<uint8_t>());
					break;

				case SSEQ_CMD_PORTATIME:
					this->portaTime = this->file.ReadLE<uint8_t>();
					break;

				case SSEQ_CMD_SWEEPPITCH:
					this->sweepPitch = this->file.ReadLE<uint16_t>();
					break;

				//-----------------------------------------------------------------
				// Modulation-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_MODDEPTH:
					this->modDepth = this->file.ReadLE<uint8_t>();
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODSPEED:
					this->modSpeed = this->file.ReadLE<uint8_t>();
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODTYPE:
					this->modType = this->file.ReadLE<uint8_t>();
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODRANGE:
					this->modRange = this->file.ReadLE<uint8_t>();
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODDELAY:
					this->modDelay = this->file.ReadLE<uint16_t>();
					this->updateFlags.set(TUF_MOD);
					break;

				//-----------------------------------------------------------------
				// Commands unused for timing
				//-----------------------------------------------------------------

				case SSEQ_CMD_RANDOM:
					this->file.pos += 5;
					break;

				case SSEQ_CMD_PRINTVAR:
					++this->file.pos;
					break;

				case SSEQ_CMD_UNSUP1:
				{
					int t = this->file.ReadLE<uint8_t>();
					if (t >= SSEQ_CMD_UNSUP2_LO && t <= SSEQ_CMD_UNSUP2_HI)
						++this->file.pos;
					++this->file.pos;
					break;
				}

				case SSEQ_CMD_IF:
					break;

				default:
					if (cmd >= SSEQ_CMD_UNSUP2_LO && cmd <= SSEQ_CMD_UNSUP2_HI)
						this->file.pos += 3;
			}
	}
}
