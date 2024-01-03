/*
  This file includes all Synthesizers functions that produce and transform sound waves
  It also contains examples of instruments that can be used recreate their sounds
*/
#pragma once
#include <map>
//#include "StateMachine.h"

namespace synth
{
	const double PI = 2.0 * std::acos(0.0);
	double OCTIVE_BASE_FREQUENCY = 16.35; // C0 frequency of octave for equal-tempered scale, A4 = 440 Hz
	int	STARTING_HALF_STEP = 45; // assuming base frequency is C0, 45 represents A3
	double D12TH_ROOT_OF2 = std::pow(2.0, 1.0 / 12.0); // assuming western 12 notes per octave

	// Oscillator wave forms
	const int OSC_SINE = 0;
	const int OSC_SQUARE = 1;
	const int OSC_TRIANGLE = 2;
	const int OSC_SAW_ANALOG = 3;
	const int OSC_SAW_DIGITAL = 4;
	const int OSC_NOISE = 5;

	std::map<char, int> NoteToScaleMap{
	{ 'A', 0 }, { 'a', 1 }, { 'B', 2 }, { 'C', 3 }, { 'c', 4 }, { 'D', 5 }, { 'd', 6 }, { 'E', 7 },
	{ 'F', 8 }, { 'f', 9 }, { 'G', 10 }, { 'g', 11 }
	};

	std::map<int, char> SacleToNoteMap{
	{ 0, 'A' }, { 1, 'a' }, { 2, 'B' }, { 3, 'C' }, { 4, 'c' }, { 5, 'D' }, { 6, 'd' }, { 7,'E' },
	{ 8, 'F' }, { 9, 'f' }, { 10, 'G' }, { 11, 'g' }
	};

	// Convert frequency (Hz) to angular velocity
	double FrequencyToAngularVelocity(const double& hertz)
	{
		return hertz * 2.0 * PI;
	}

	// Scale to Frequency conversion 
	double ScaleToFrequency(const int& notePosition)
	{
		return OCTIVE_BASE_FREQUENCY * std::pow(D12TH_ROOT_OF2, notePosition + STARTING_HALF_STEP);
	}

	// used for the A minor key
	int NegativeHarmonyTransformation(int scaleNote)
	{
		int basicNote = scaleNote % 12;
		return ((13 - basicNote)) % 12;
	}

	struct LFO
	{
		double Amplitude;
		double Hertz;
	};

	double Oscillator(const double& time, const double& hertz, const int& type = OSC_SINE,
		const LFO& fm = { 0.0, 0.0 }, const LFO& am = { 0.0, 0.0 })
	{
		double freq = FrequencyToAngularVelocity(hertz) * time +
			fm.Amplitude * fm.Hertz * (std::sin(FrequencyToAngularVelocity(fm.Hertz) * time));

		double offset = 1 - am.Amplitude;
		double AM = offset + am.Amplitude * std::sin(FrequencyToAngularVelocity(am.Hertz) * time);

		switch (type)
		{
		case OSC_SINE:
			return std::sin(freq) * AM;
		case OSC_SQUARE:
			return (std::sin(freq) > 0 ? 1.0 : -1.0) * AM;
		case OSC_TRIANGLE:
			return std::asin(std::sin(freq)) * (2.0 / PI) * AM;
		case OSC_SAW_ANALOG:
		{
			double output = 0.0;
			for (unsigned int n = 1; n < 50; n++)
				output += (std::sin(n * freq) / n);
			return output * (2.0 / PI) * AM;
		}
		case OSC_SAW_DIGITAL: // has some problems when mixed with other waves
			return (2.0 / PI) * (hertz * PI * std::fmod(time, 1.0 / hertz) - (PI / 2.0));
		case OSC_NOISE:
			return 2.0 * ((double)std::rand() / (double)RAND_MAX) - 1.0;
		default:
			return 0.0;
		}
	};

	struct Envelope
	{
		virtual double Amplitude(const double& time, const double& timeOn, const double& timeOff) = 0;
	};

	struct EnvolopeADSR : Envelope
	{
		double AttackTime, DecayTime, SustainAmplitude, ReleaseTime, StartAmplitude;

		EnvolopeADSR(double attack = 0.1, double decay = 0.1, double sustainAmplitude = 1.0,
			double release = 0.2, double startAmplitude = 1.0) : AttackTime(attack), DecayTime(decay),
			SustainAmplitude(sustainAmplitude), ReleaseTime(release), StartAmplitude(startAmplitude) {};

		// TODO: try to fix bug where playing the note again does a click sound.. might be happening because of 
		//sudden change change in sound
		double Amplitude(const double& time, const double& timeOn, const double& timeOff) override
		{
			double amplitude = 0.0;
			double releaseAmplitude = 0.0;

			if (timeOn > timeOff) // note is being held
			{
				double lifeTime = time - timeOn;

				// attack phase
				if (lifeTime <= AttackTime)
					amplitude = (lifeTime / AttackTime) * StartAmplitude;

				// decay phase
				if (lifeTime > AttackTime && lifeTime <= AttackTime + DecayTime)
					amplitude = ((lifeTime - AttackTime) / DecayTime) * (SustainAmplitude - StartAmplitude) + StartAmplitude;

				if (lifeTime > AttackTime + DecayTime)
					amplitude = SustainAmplitude;
			}
			else // note was released
			{
				double lifeTime = timeOff - timeOn;

				if (lifeTime <= AttackTime)
					releaseAmplitude = (lifeTime / AttackTime) * StartAmplitude;

				if (lifeTime > AttackTime && lifeTime <= AttackTime + DecayTime)
					releaseAmplitude = ((lifeTime - AttackTime) / DecayTime) * (SustainAmplitude - StartAmplitude) + StartAmplitude;

				if (lifeTime > AttackTime + DecayTime)
					releaseAmplitude = SustainAmplitude;

				amplitude = ((time - timeOff) / ReleaseTime) * (0.0 - releaseAmplitude) + releaseAmplitude;
			}

			if (amplitude <= 0.001)
				amplitude = 0.0;

			return amplitude;
		}
	};

	struct InstrumentBase
	{
		double Volume;
		EnvolopeADSR Env;
		LFO FM;				// Note Frequency modulation (used to give a vibrato effect)
		LFO AM;				// Note Amplitude modulation ( used to give a tremolo effect)
		double MaxLifeTime = -1.0;
		virtual double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) = 0;
	};

	// Basic note
	struct Note
	{
		int ScalePosition;	// Position in the scale
		double OnTime;		// Time that note was activated
		double OffTime;		// Time that note was deactivated
		bool IsActive;
		InstrumentBase* Channel; // might need to delete the pointer in a destructor

		Note(int pos = 0, double on = 0.0, double off = 0.0, bool active = false, InstrumentBase* channel = nullptr) :
			ScalePosition(pos), OnTime(on), OffTime(off), IsActive(active), Channel(channel)
		{
			/*std::cout << "Harmonic strcture:\n";
			std::cout << ScaleToFrequency(ScalePosition) << std::endl;
			std::cout << 2 * ScaleToFrequency(ScalePosition) << std::endl;
			std::cout << 3 * ScaleToFrequency(ScalePosition) << std::endl;
			std::cout << 4 * ScaleToFrequency(ScalePosition) << std::endl;
			std::cout << 5 * ScaleToFrequency(ScalePosition) << std::endl;
			std::cout << 6 * ScaleToFrequency(ScalePosition) << std::endl;
			std::cout << 7 * ScaleToFrequency(ScalePosition) << std::endl;
			std::cout << 8 * ScaleToFrequency(ScalePosition) << std::endl;*/
		}

		double Sound(const double& time)
		{
			double noise = 0.0;
			bool isNoteFinished = false;

			if (Channel != nullptr)
				noise = Channel->Sound(time, OnTime, OffTime, ScalePosition, isNoteFinished);

			IsActive = !isNoteFinished;

			return noise;
		}
	};

	// Uses BPM to play notes of specified sequences of beats
	struct Sequencer
	{
		struct Channel
		{
			InstrumentBase* Instrument;
			std::string BeatSequence;

			Channel(InstrumentBase* instrument, const std::string& beat) : Instrument(instrument), BeatSequence(beat) {}
		};

		double BeatTime;
		double Accumulate;
		int CurrentBeat;
		int TotalBeats;
		void(*EndOffSequenceCallBack)(Sequencer*);
		//TODO: convert these into a dictionary, if we want an instrument to only be able to play one note at a time
		// if this is made some changes would also need to happen on the application.cpp file
		std::vector<Channel> Channels;
		std::vector<Note> Notes;

		// default four quarter notes, and a bar is composed of 16th notes
		Sequencer(void(*func)(Sequencer*), float tempo = 120.0f, int beats = 4, int subBeats = 4)
		{
			BeatTime = (60.0f / tempo) / subBeats;
			CurrentBeat = 0;
			TotalBeats = subBeats * beats;
			Accumulate = 0;
			EndOffSequenceCallBack = func;
		}

		int Update(const double& deltaTime, const double& currentTime)
		{
			Notes.clear();

			Accumulate += deltaTime;
			while (Accumulate >= BeatTime)
			{
				for (Channel channel : Channels)
				{
					if (channel.BeatSequence[CurrentBeat] != '.')
					{
						Note newNote(NoteToScaleMap[channel.BeatSequence[CurrentBeat]],
							currentTime, 0.0, false, channel.Instrument);
						Notes.emplace_back(newNote);
					}
				}

				Accumulate -= BeatTime;
				CurrentBeat++;
				CurrentBeat %= TotalBeats;

				if (CurrentBeat == 0)
					EndOffSequenceCallBack(this);
			}

			return Notes.size();
		}

		void PlayBar(InstrumentBase* instrument, const std::string& bar)
		{
			auto instFound = std::find_if(Channels.begin(), Channels.end(),
				[&instrument](Channel const& chl) {return chl.Instrument == instrument; });

			if (instFound == Channels.end())
			{
				Channel channel(instrument, bar);
				Channels.emplace_back(channel);
			}
			else
			{
				instFound->BeatSequence = bar;
			}
		}
	};

	struct Filter
	{
		double Output = 0.0;

		virtual void SetFilterPresets(double sampleTimeFrequency = 0.0, double cutoffFrequency = 0.0) = 0;

		virtual double FilterOutput(double mixedOutput) = 0;
	};

	// Lowers amplitude of high frequency sounds
	struct LowPassFilter : Filter
	{
		double ePow = 0.0;

		// cutoff should be between values of 0 (no effect) to 50 (very hight effect) 
		LowPassFilter(/*double sampleTimeFrequency = 0.0, double cutoffFrequency = 0.0*/)
		{
			//ePow = 1 - std::exp(-sampleTimeFrequency * FrequencyToAngularVelocity(cutoffFrequency));
		}

		void SetFilterPresets(double sampleTimeFrequency = 0.0, double cutoffFrequency = 0.0) override
		{
			ePow = 1 - std::exp(-sampleTimeFrequency * FrequencyToAngularVelocity(cutoffFrequency));
		}

		double FilterOutput(double mixedOutput) override
		{
			Output = mixedOutput + (Output - mixedOutput) * ePow;
			return Output;
		}
	};

	// Lowers amplitude of low frequency sounds 
	struct HighPassFilter : Filter
	{
		double amplFac = 0.0;
		double y1c = 0.0;
		double x1 = 0.0;

		// cutoff should be between values of 0 (no effect) to 5 (very hight effect) 
		HighPassFilter(/*double sampleTimeFrequency = 0.0, double cutoffFrequency = 0.0*/)
		{
			/*double tranformedValue = sampleTimeFrequency * FrequencyToAngularVelocity(cutoffFrequency) / 2;
			amplFac = 1 / (tranformedValue + 1);
			y1c = tranformedValue - 1;*/
		}

		void SetFilterPresets(double sampleTimeFrequency = 0.0, double cutoffFrequency = 0.0) override
		{
			double tranformedValue = sampleTimeFrequency * FrequencyToAngularVelocity(cutoffFrequency) / 2;
			amplFac = 1 / (tranformedValue + 1);
			y1c = tranformedValue - 1;
		}

		double FilterOutput(double mixedOutput) override
		{
			Output = amplFac * (mixedOutput - x1 - Output * y1c);
			x1 = mixedOutput;
			return Output;
		}
	};

	// Bellow is the list of created instruments
	// Bellow instruments are meant to be used by the user only
	struct TestInstrument : public InstrumentBase
	{
		TestInstrument()
		{
			FM = { 1.5, 1.5 };
			AM = { 0.3, 3.0 };
			Env.ReleaseTime = 0.1;
			Volume = 0.5;

			Env.AttackTime = 0.01;
			Env.DecayTime = 0.1;
			Env.SustainAmplitude = 0.65;
		};

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			if (amplitude <= 0.0)
			{
				noteFinished = timeOff > timeOn; // only finish playing if note is release phase and amplitude is 0
				return 0.0;
			}

			int scaleTest = scalePos - 12;
			int waveForm = OSC_SINE;
			double lifeTime = time - timeOn;
			/*double sound = 1 * Oscillator(lifeTime, ScaleToFrequency(scaleTest), waveForm, FM) +
				1.3 * Oscillator(lifeTime, 2 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.35 * Oscillator(lifeTime, 3 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.35 * Oscillator(lifeTime, 4 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.3 * Oscillator(lifeTime, 5 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.31 * Oscillator(lifeTime, 6 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.32 * Oscillator(lifeTime, 7 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.25 * Oscillator(lifeTime, 8 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.0 * Oscillator(lifeTime, 9 * ScaleToFrequency(scaleTest), waveForm, FM) +
				0.6 * Oscillator(lifeTime, 10 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.0 * Oscillator(lifeTime, 11 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.1 * Oscillator(lifeTime, 12 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.2 * Oscillator(lifeTime, 13 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1.25 * Oscillator(lifeTime, 14 * ScaleToFrequency(scaleTest), waveForm, FM) +
				1 * Oscillator(lifeTime, 15 * ScaleToFrequency(scaleTest), waveForm, FM) +
				0.6 * Oscillator(lifeTime, 16 * ScaleToFrequency(scaleTest), waveForm, FM) +
				0.5 * Oscillator(lifeTime, 17 * ScaleToFrequency(scaleTest), waveForm, FM) +
				0.45 * Oscillator(lifeTime, 18 * ScaleToFrequency(scaleTest), waveForm, FM) +
				0.4 * Oscillator(lifeTime, 19 * ScaleToFrequency(scaleTest), waveForm, FM) +
				0.35 * Oscillator(lifeTime, 20 * ScaleToFrequency(scaleTest), waveForm, FM);*/
			double sound = 2 * Oscillator(lifeTime, ScaleToFrequency(scalePos - 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos + 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos), OSC_SINE, FM, AM);

			return  amplitude * sound * Volume;
		}
	};

	struct InstrumentBell : public InstrumentBase
	{
		InstrumentBell()
		{
			FM = { 5.0, 0.001 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 1.0;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 1.0;
			Volume = 1.0;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			if (amplitude <= 0.0)
			{
				noteFinished = timeOff > timeOn;
				return 0.0;
			}

			double lifeTime = time - timeOn;
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(scalePos + 12), OSC_SINE, FM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos + 24))
				+ 0.25 * Oscillator(lifeTime, ScaleToFrequency(scalePos + 36));

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentBell8 : public InstrumentBase
	{
		InstrumentBell8()
		{
			FM = { 5.0, 0.001 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 0.5;
			Env.SustainAmplitude = 0.8;
			Env.ReleaseTime = 1.0;
			Volume = 1.0;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			if (amplitude <= 0.0)
			{
				noteFinished = timeOff > timeOn;
				return 0.0;
			}

			double lifeTime = time - timeOn;
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(scalePos), OSC_SQUARE, FM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos + 12))
				+ 0.25 * Oscillator(lifeTime, ScaleToFrequency(scalePos + 24));

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentHarmonica : public InstrumentBase
	{
		InstrumentHarmonica()
		{
			FM = { 5.0, 0.001 };
			Env.AttackTime = 0.05;
			Env.DecayTime = 1.0;
			Env.SustainAmplitude = 0.95;
			Env.ReleaseTime = 0.1;
			Volume = 0.3;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			if (amplitude <= 0.0)
			{
				noteFinished = timeOff > timeOn;
				return 0.0;
			}

			double lifeTime = time - timeOn;
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(scalePos - 12), OSC_SAW_ANALOG, FM)
				+ 1.0 * Oscillator(lifeTime, ScaleToFrequency(scalePos), OSC_SQUARE, FM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos + 12), OSC_SQUARE)
				+ 0.25 * Oscillator(lifeTime, ScaleToFrequency(0), OSC_NOISE);

			return amplitude * sound * Volume;
		}
	};

	// Bellow instruments are meant to be used by the sequencer only
	// TODO: consider passing in a life time so that a note can play use its sustain and release time
	struct InstrumentDrumKick : public InstrumentBase
	{
		InstrumentDrumKick()
		{
			/*FM = { 1.0, 2.0 };
			AM = { 0.2, 2 };*/
			FM = { 0.0, 0.0 };
			AM = { 0.0, 0 };
			Env.AttackTime = 0.001;
			Env.DecayTime = 0.5;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 0.5;
			// volume for presentation
			//Volume = 1.2;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(-STARTING_HALF_STEP + 12), OSC_SINE, FM, AM)
				+ 0.8 * Oscillator(lifeTime, 2 * ScaleToFrequency(-STARTING_HALF_STEP + 12), OSC_SINE, FM, AM)
				/*+ 0.6 * Oscillator(lifeTime, 3 * ScaleToFrequency(-STARTING_HALF_STEP + 12), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, 4 * ScaleToFrequency(-STARTING_HALF_STEP + 12), OSC_SINE, FM, AM)
				+ 0.3 * Oscillator(lifeTime, 5 * ScaleToFrequency(-STARTING_HALF_STEP + 12), OSC_SINE, FM, AM)*/
				+0.01 * Oscillator(lifeTime, ScaleToFrequency(0), OSC_NOISE);

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentDrumSnare : public InstrumentBase
	{
		InstrumentDrumSnare()
		{
			FM = { 0.5, 1.0 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 0.6;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 0.05;
			// volume for presentation
			//Volume = 0.08;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;
			double sound = 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos - 24), OSC_SINE, FM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(0), OSC_NOISE);

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentDrumHihat : public InstrumentBase
	{
		InstrumentDrumHihat()
		{
			FM = { 1.5, 1.0 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 0.05;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 0.02;
			// volume for presentation
			//Volume = 0.05;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;
			double sound = 0.1 * Oscillator(lifeTime, ScaleToFrequency(scalePos - 12), OSC_SQUARE, FM)
				+ 0.9 * Oscillator(lifeTime, ScaleToFrequency(0), OSC_NOISE);

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentUserSensor : public InstrumentBase
	{
		InstrumentUserSensor()
		{
			//FM = { 2.0, 1.0 };
			//AM = { 0.5, 4 };
			FM = { 1.5, 1.5 };
			AM = { 0.3, 3.0 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 0.5;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 1.3;
			// volume for presentation
			//Volume = 0.15;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;

			//double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(scalePos - 12), OSC_SINE, FM)
			//	+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos), OSC_SINE)
			//	+ 0.25 * Oscillator(lifeTime, ScaleToFrequency(scalePos - 24), OSC_SINE);
			double sound = 1 * Oscillator(lifeTime, ScaleToFrequency(scalePos - 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos + 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(scalePos), OSC_SINE, FM, AM);

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentCordPlayer : public InstrumentBase
	{
		InstrumentCordPlayer()
		{
			FM = { 1.0, 1.0 };
			AM = { 0.5, 4 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 1.1;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 0.5;
			// volume for presentation
			//Volume = 0.15;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;

			int cordRoot = scalePos - 12;
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(cordRoot), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 7), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 12), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 15), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 19), OSC_SINE, FM)
				//+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 22), OSC_SINE, FM)
				;

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentCordBase : public InstrumentBase
	{
		InstrumentCordBase()
		{
			FM = { 0.8, 0.8 };
			AM = { 0.0, 0.0 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 2.0;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 0.3;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;

			int cordRoot = scalePos - 24;
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(cordRoot), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 12), OSC_SINE, FM, AM)
				+ 0.2 * Oscillator(lifeTime, 3 * ScaleToFrequency(cordRoot), OSC_SINE)
				+ 0.05 * Oscillator(lifeTime, 5 * ScaleToFrequency(cordRoot), OSC_SINE);

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentCordDiminished : public InstrumentBase
	{
		InstrumentCordDiminished()
		{
			FM = { 1.0, 1.0 };
			AM = { 0.5, 4 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 1.1;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 0.5;
			// volume for presentation
			//Volume = 0.15;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;

			int cordRoot = scalePos - 12;

			// first diminished
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(cordRoot), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 6), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 12), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 15), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 18), OSC_SINE, FM)
				//+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 21), OSC_SINE, FM)
				;

			//double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(cordRoot +1), OSC_SINE, FM)
			//	+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 7), OSC_SINE, FM)
			//	+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 13), OSC_SINE, FM)
			//	+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 16), OSC_SINE, FM)
			//	+ 1.4 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 22), OSC_SINE, FM);

			/*double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(cordRoot+2), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 8), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 14), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 17), OSC_SINE, FM)
				+ 1.4 * Oscillator(lifeTime, ScaleToFrequency(cordRoot + 23), OSC_SINE, FM);*/

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentCordInversion : public InstrumentBase
	{
		InstrumentCordInversion()
		{
			FM = { 1.0, 1.0 };
			AM = { 0.5, 4 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 1.1;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 0.5;
			// volume for presentation
			//Volume = 0.15;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;

			int cordRoot = scalePos - 12;

			// chord inversion 
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos) - 12), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos + 7) - 12), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos)), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos + 3)), OSC_SINE, FM)
				+ 1 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos + 7)), OSC_SINE, FM)
				//+ 1 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(cordRoot + 22)), OSC_SINE, FM)
				;
			return amplitude * sound * Volume;
		}
	};

	struct InstrumentCordBaseInverted : public InstrumentBase
	{
		InstrumentCordBaseInverted()
		{
			FM = { 0.8, 0.8 };
			AM = { 0.0, 0.0 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 2.0;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 0.3;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;

			int cordRoot = scalePos - 24;
			double sound = 1.0 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos) - 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos) - 12), OSC_SINE, FM, AM)
				+ 0.2 * Oscillator(lifeTime, 3 * ScaleToFrequency(NegativeHarmonyTransformation(scalePos) - 24), OSC_SINE)
				+ 0.05 * Oscillator(lifeTime, 5 * ScaleToFrequency(NegativeHarmonyTransformation(scalePos) - 24), OSC_SINE);

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentUserSensorInversion : public InstrumentBase
	{
		InstrumentUserSensorInversion()
		{
			FM = { 1.5, 1.5 };
			AM = { 0.3, 3.0 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 0.5;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 1.3;
			// volume for presentation
			//Volume = 0.15;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;

			double sound = 1 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos) - 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos) + 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(scalePos)), OSC_SINE, FM, AM);

			return amplitude * sound * Volume;
		}
	};

	struct InstrumentUserSensorDiminished : public InstrumentBase
	{
		InstrumentUserSensorDiminished()
		{
			FM = { 1.5, 1.5 };
			AM = { 0.3, 3.0 };
			Env.AttackTime = 0.01;
			Env.DecayTime = 0.5;
			Env.SustainAmplitude = 0.0;
			Env.ReleaseTime = 0.0;
			MaxLifeTime = Env.AttackTime + Env.DecayTime;
			Volume = 1.3;
			// volume for presentation
			//Volume = 0.15;
		}

		double Sound(const double& time, const double& timeOn, const double& timeOff, const int& scalePos,
			bool& noteFinished) override
		{
			if (time - timeOn >= MaxLifeTime)
			{
				noteFinished = true;
				return 0.0;
			}

			double amplitude = Env.Amplitude(time, timeOn, timeOff);
			double lifeTime = time - timeOn;

			int diminishedNote = scalePos;
			if (scalePos == 7 || scalePos == 8 || scalePos == 10)
				diminishedNote--;

			double sound = 1 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(diminishedNote) - 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(diminishedNote) + 24), OSC_SINE, FM, AM)
				+ 0.5 * Oscillator(lifeTime, ScaleToFrequency(NegativeHarmonyTransformation(diminishedNote)), OSC_SINE, FM, AM);

			return amplitude * sound * Volume;
		}
	};
}