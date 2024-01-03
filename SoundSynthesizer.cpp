//#include <iostream>
//#include "NoiseMaker.h"
//#include "SynthUtils.h"
//
//enum WaveFormat { SINE, SQUARE, TRINAGLE, SAW_ANALOG, SAW_DIGITAL, NOISE };
//
//// oscillator
//double Oscillator(double hertz, double time, WaveFormat type, double LFOHertz = 0.0, double LFOAmplitude = 0.0);
//
//// Converts frequency (Hz) to angular velocity
//double FrequencyToAngularVelocity(double hertz);
//
//// function used by NoiseMaker to generate sound waves 
//double MakeNoise(int channel, double time);
//
//// amplitude modulation of output to give texture
//struct EnvolopeADSR
//{
//	double AttackTime;
//	double DecayTime;
//	double ReleaseTime;
//	double SustainAmpllitude; // value is a percentage of the original sound
//	double StartAmplitude; // value is a percentage of the original sound
//	double TriggerOnTime;
//	double TriggerOffTime;
//	bool noteOn;
//
//	EnvolopeADSR() : AttackTime(0.1), DecayTime(0.05), ReleaseTime(0.25), SustainAmpllitude(0.75),
//		StartAmplitude(1.0), TriggerOnTime(0.0), TriggerOffTime(0.0), noteOn(false) {}
//
//	double GetAmplitude(double time)
//	{
//		double amplitude = 0.0;
//
//		if (noteOn)
//		{
//			double lifeTime = time - TriggerOnTime;
//
//			// attack phase
//			if (lifeTime <= AttackTime)
//				amplitude = (lifeTime / AttackTime) * StartAmplitude;
//
//			// decay phase
//			if (lifeTime > AttackTime && lifeTime <= (AttackTime + DecayTime))
//				amplitude = ((lifeTime - AttackTime) / DecayTime) * (SustainAmpllitude - StartAmplitude) + StartAmplitude;
//
//			// sustain phase
//			if (lifeTime > (AttackTime + DecayTime))
//				amplitude = SustainAmpllitude;
//
//		}
//		// not fully completed: should take into account the last frequency state and value,
//		// as well as have release time based on were the last amplitude was
//		else
//		{
//			double deathTime = time - TriggerOffTime;
//			// release phase
//			if (deathTime <= ReleaseTime)
//				amplitude = (deathTime / ReleaseTime) * (0.0 - SustainAmpllitude) + SustainAmpllitude;
//		}
//
//		// amplitude should not be negative
//		if (amplitude <= 0.001)
//			amplitude = 0;
//
//		return amplitude;
//	}
//
//	void NoteOn(double timeOn)
//	{
//		TriggerOnTime = timeOn;
//		noteOn = true;
//	}
//
//	void NoteOff(double timeOff)
//	{
//		TriggerOffTime = timeOff;
//		noteOn = false;
//	}
//};
//
//struct Instrument
//{
//	double Volume;
//	EnvolopeADSR Envolope;
//
//	virtual double Sound(double time, double frequency) = 0;
//};
//
//struct Bell : public Instrument
//{
//	Bell()
//	{
//		Envolope.AttackTime = 0.01;
//		Envolope.DecayTime = 1.0;
//		Envolope.ReleaseTime = 1.0;
//		Envolope.SustainAmpllitude = 0.0;
//	}
//
//	double Sound(double time, double frequency) override
//	{
//		double output = Envolope.GetAmplitude(time) *
//			(
//				1.0 * Oscillator(frequency * 2.0, time, SINE, 5, 0.001)
//				+ 0.5 * Oscillator(frequency * 3.0, time, SINE)
//				+ 0.25 * Oscillator(frequency * 4.0, time, SINE)
//				);
//
//		return output;
//	}
//};
//
//struct Harmonica : public Instrument
//{
//	Harmonica() {}
//
//	double Sound(double time, double frequency) override
//	{
//		double output = Envolope.GetAmplitude(time) *
//			(
//				1.0 * Oscillator(frequency, time, SAW_ANALOG)
//				+ 0.5 * Oscillator(frequency * 1.5, time, SQUARE)
//				+ 0.25 * Oscillator(frequency * 2.0, time, SQUARE)
//				+ 0.05 * Oscillator(0, time, NOISE)
//				);
//
//		return output;
//	}
//};
//
//// Global synthesizer variables
//std::atomic<double> frequencyOutut = 0.0; // dominant output frequency of instrument
//double octiveBaseFrequency = 110.0; // A2 frequency of octave represented by the keyboard
//double d12thRootOf2 = std::pow(2.0, 1.0 / 12.0); // assuming western 12 notes per octave
//Instrument* voice = nullptr;
//
//int main()
//{
//	std::cout << "Program Started\n";
//
//	// Get all sound hardware
//	std::vector<std::wstring> devices = NoiseMaker<short>::Enumerate();
//	for (const auto& device : devices)
//		std::wcout << "Found Output Devise: " << device << std::endl;
//
//	voice = new Harmonica();
//
//	NoiseMaker<short> sound(devices[0], 44100, 2, 8, 512);
//	sound.SetUserFunction(MakeNoise);
//
//	int currentKey = -1;
//	bool keyPressed = false;
//	while (true)
//	{
//		keyPressed = false;
//		// Add a keyboard like a piano
//		for (int k = 0; k < 16; k++)
//		{
//			if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[k])) & 0x8000)
//			{
//				if (currentKey != k)
//				{
//					frequencyOutut = octiveBaseFrequency * std::pow(d12thRootOf2, k);
//					voice->Envolope.NoteOn(sound.GetTime());
//					currentKey = k;
//				}
//
//				keyPressed = true;
//			}
//		}
//
//		if (!keyPressed)
//		{
//			if (currentKey != -1)
//			{
//				voice->Envolope.NoteOff(sound.GetTime());
//				currentKey = -1;
//			}
//		}
//	}
//
//	delete voice;
//	std::cout << "Program Ended\n";
//}
//
//double MakeNoise(int channel, double time)
//{
//	double output = voice->Sound(time, frequencyOutut);
//	return output * 0.2; // master volume
//}
//
//double Oscillator(double hertz, double time, WaveFormat type, double LFOHertz /*= 0.0*/, double LFOAmplitude /*= 0.0*/)
//{
//	double frequency = FrequencyToAngularVelocity(hertz) * time + LFOAmplitude * hertz
//		* std::sin(FrequencyToAngularVelocity(LFOHertz) * time);
//	double sinWave = std::sin(frequency);
//	double output = 0.0;
//
//	switch (type)
//	{
//	case SINE: // sin wave
//		return sinWave;
//	case SQUARE: // square wave
//		return sinWave > 0.0 ? 0.5 : -0.5;
//	case TRINAGLE: // triangle wave
//		return std::asin(sinWave) * (2.0 / synth::PI);
//	case SAW_ANALOG: // Saw wave (analogue/ warm / slow performance)
//		for (double n = 1.0; n < 50.0; n++)
//			output += std::sin(n * frequency) / n;
//
//		return output * (2.0 / synth::PI);
//	case SAW_DIGITAL: // Saw wave (optimized/ harsh / fast performance)
//		return (2.0 / synth::PI) * (hertz * synth::PI * std::fmod(time, 1.0 / hertz) - (synth::PI / 2));
//
//	case NOISE: // Pseudo Random Noise
//		return 2.0 * ((double)std::rand() / (double)RAND_MAX) - 1.0;
//	default:
//		return 0;
//	}
//}
//
//double FrequencyToAngularVelocity(double hertz)
//{
//	return hertz * 2 * synth::PI;
//}
