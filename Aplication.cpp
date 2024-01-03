/*
	Connects the Synthesizer functionality to the sound card lib
	Allows for multiple "different" notes to be played at the same time (Polyphony)
	Generates data to be feed to the sound card, and how and when it is passed along to it
*/

#include <iostream>
#include <algorithm>
#include "NoiseMaker.h"
#include "SynthUtils.h"
#include "NoteGenarator.h"
#include "StateMachine.h"
#include "SessionEvaluator.h"
#include "SocketServer.h"

SocketServer* Server;

AI::StateMachine* AISystem;

std::atomic<double> DeltaTime;

std::vector<synth::Note> NotesPlaying;
std::mutex notesMutex;
// Instruments
synth::TestInstrument Test;
synth::InstrumentBell Bell;
synth::InstrumentBell8 Bell8;
synth::InstrumentHarmonica Harmonica;
synth::InstrumentDrumKick Kick;
synth::InstrumentDrumSnare Snare;
synth::InstrumentDrumHihat HitHat;

synth::InstrumentCordPlayer CordPlayer;
// The base works for both normal and diminished chords
synth::InstrumentCordBase CordBase;
synth::InstrumentUserSensor UserSensor;

synth::InstrumentCordDiminished CordDiminished;
synth::InstrumentUserSensorDiminished UserDiminished;

synth::InstrumentCordInversion CordInversion;
synth::InstrumentCordBaseInverted BaseInversion;
synth::InstrumentUserSensorInversion UserInversion;

synth::LowPassFilter LowFilter;

// backing track cords according to their measure
// test AI input
int testAIIndex = 0;
NGen::AIOutput TetsNoteGenaration[24] = { {'A', 2, NGen::NORMAL}, {'D', 3, NGen::NORMAL}, {'A', 3, NGen::NORMAL},
{'A', 4, NGen::INVERTED}, {'D', 4, NGen::NORMAL}, {'D', 5, NGen::NORMAL}, {'A', 5, NGen::INVERTED}, {'A', 6, NGen::NORMAL},
{'E', 6, NGen::NORMAL}, {'D', 5, NGen::NORMAL}, {'A', 5, NGen::NORMAL}, {'E', 3, NGen::NORMAL},
// Second loop
{'A', 2, NGen::NORMAL}, {'D', 3, NGen::NORMAL}, {'A', 3, NGen::DIMINISHED},
{'A', 4, NGen::NORMAL}, {'D', 4, NGen::NORMAL}, {'D', 5, NGen::DIMINISHED}, {'A', 5, NGen::NORMAL}, {'A', 6, NGen::NORMAL},
{'E', 6, NGen::NORMAL}, {'D', 5, NGen::INVERTED}, {'A', 5, NGen::NORMAL}, {'E', 3, NGen::NORMAL} };

int CurrentBarIndex = 0;
// twelve bar blues cord progression in the A minor scale
char TwelveBarBluesCordProgressionTest[12] = { 'A', 'D', 'A', 'A', 'D', 'D', 'A', 'A', 'E', 'D', 'A', 'E' };

// utility function to safely remove objects from a vector
//Note: there multiple ways for doing this
void SafeRemove(std::vector<synth::Note>& notes, bool(*lambda)(synth::Note const& note))
{
	auto noteToRemove = NotesPlaying.begin();
	while (noteToRemove != notes.end())
	{
		if (!lambda(*noteToRemove))
			noteToRemove = notes.erase(noteToRemove);
		else
			++noteToRemove;
	}
}

// utility function for mapping values. 
// Here result is the reversed because it was useful for this particular case
double MapValueReverse(const double& value, const double& max1, const double& min1, 
	const double& max2, const double& min2)
{
	double percentage = (value - min1) / (max1 - min1);
	double resultMaped = (percentage * (max2 + min2)) - min2;
	return  max2 - resultMaped;
}

double MakeNoise(int channel, double time)
{
	double mixedOutput = 0.0;
	std::lock_guard<std::mutex> lg(notesMutex);

	for (synth::Note& note : NotesPlaying)
	{
		mixedOutput += note.Sound(time);
	}

	SafeRemove(NotesPlaying, [](synth::Note const& note) {return note.IsActive; });

	LowFilter.SetFilterPresets(0.1, MapValueReverse(Server->Mood, 90.0, 10.0, 3.0, 0.0));
	return LowFilter.FilterOutput(mixedOutput) * 0.1;
	//return mixedOutput * 0.1; // master volume
}

void RefreshPhrase(synth::Sequencer* sequencer)
{
	// could do a for loop and change every instrument note to play 
	char currentCordBar[] = "................";

	sequencer->PlayBar(&CordPlayer, currentCordBar);
	sequencer->PlayBar(&CordBase, currentCordBar);
	sequencer->PlayBar(&UserSensor, currentCordBar);

	sequencer->PlayBar(&CordInversion, currentCordBar);
	sequencer->PlayBar(&BaseInversion, currentCordBar);
	sequencer->PlayBar(&UserInversion, currentCordBar);

	sequencer->PlayBar(&CordDiminished, currentCordBar);
	sequencer->PlayBar(&UserDiminished, currentCordBar);

	currentCordBar[0] = TwelveBarBluesCordProgressionTest[CurrentBarIndex];

	AISystem->Tick(Server->Mood, CurrentBarIndex);
	AI::AIOutput* outPut = AISystem->OutPut;

	std::string currentPalyerBar = NGen::GetNewPhrase((int)outPut->NumberOfNotes,
		outPut->FirstNote);

	switch (outPut->Change)
	{
	case AI::INVERTED:
		sequencer->PlayBar(&CordInversion, currentCordBar);
		sequencer->PlayBar(&BaseInversion, currentCordBar);
		sequencer->PlayBar(&UserInversion, currentPalyerBar);
		break;
	case AI::DIMINISHED:
		sequencer->PlayBar(&CordDiminished, currentCordBar);
		sequencer->PlayBar(&CordBase, currentCordBar);
		sequencer->PlayBar(&UserDiminished, currentPalyerBar);
		break;
	default:
		sequencer->PlayBar(&CordPlayer, currentCordBar);
		sequencer->PlayBar(&CordBase, currentCordBar);
		sequencer->PlayBar(&UserSensor, currentPalyerBar);
		break;
	}

	//std::cout << "Value: " << MapValue(Server->Mood)  << std::endl;
	Evalautor::OutPutHistory.push_back({Server->Mood, outPut->Change});
	++CurrentBarIndex %= 12;
	//++testAIIndex %= 24;
}

int main()
{
	std::cout << "Program Started\n";
	Server = new SocketServer();
	Server->StartServer();
	while (!Server->HasClient) {};

	AISystem = new AI::StateMachine();

	std::vector<std::wstring> devices = NoiseMaker<short>::Enumerate();
	NoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);
	sound.SetUserFunction(&MakeNoise);

	auto oldTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	double wallTime = 0.0;

	// Create Sequencer
	synth::Sequencer sequencer(RefreshPhrase, 120);
	sequencer.PlayBar(&Snare, "....A.......A...");
	sequencer.PlayBar(&Kick, "A...A...A...A...");
	sequencer.PlayBar(&HitHat, "A.A.A.A.A.A.A.A.");

	bool sessionIsOn = true;
	while (sessionIsOn)
	{
		currentTime = std::chrono::high_resolution_clock::now();
		DeltaTime = std::chrono::duration<double>(currentTime - oldTime).count();
		wallTime += DeltaTime;
		oldTime = currentTime;
		double timeNow = sound.GetTime();

		if (sequencer.Update(DeltaTime, timeNow) > 0)
		{
			std::lock_guard<std::mutex> lg(notesMutex);
			NotesPlaying.insert(NotesPlaying.end(), std::make_move_iterator(sequencer.Notes.begin()),
				std::make_move_iterator(sequencer.Notes.end()));
		}

		for (unsigned int k = 0; k < 17; k++)
		{
			short keyState = GetAsyncKeyState((unsigned char)("1Q2WE4R5TY7U8I9OP"[k]));

			// Check if played note already exists in the current notes being played by the user 
			notesMutex.lock();
			auto noteFound = std::find_if(NotesPlaying.begin(), NotesPlaying.end(),
				[&k](synth::Note const& note) {return note.ScalePosition == k - 1 && note.Channel == &Test; });

			// does not have note
			if (noteFound == NotesPlaying.end())
			{
				if (keyState & 0x8000)
				{
					synth::Note newNote(k - 1, timeNow, 0.0, true, &Test);
					NotesPlaying.emplace_back(newNote);
				}
			}
			else // note exists in vector 
			{
				if (keyState & 0x8000) // note is being held
				{
					if (noteFound->OffTime > noteFound->OnTime) // note was pressed again during released phase
					{
						noteFound->OnTime = timeNow;
						noteFound->IsActive = true;
					}
				}
				else
				{
					if (noteFound->OffTime < noteFound->OnTime) // Key released, enter note release phase
						noteFound->OffTime = timeNow;
				}
			}

			notesMutex.unlock();
		}

		// end session if the z key has been pressed
		sessionIsOn = !(GetAsyncKeyState('Z') & 0x8000);

		/*std::cout << "\rNotes: " << NotesPlaying.size() << "  Real Time: " << wallTime << "  CPU Time: " << timeNow <<
			"  Latency: " << wallTime - timeNow << "   ";*/
	}

	Evalautor::EvalauteSession();

	delete AISystem;
	//delete Server;
	std::cout << "Program Ended\n";
}

// OLD WAY TO DO THE AI SYSTEM
//using namespace AI;
//
//void AISetup();
//
//void Tick(int moodValue, int currentMeasure);
//
//void SetState(IState* newSate);
//
//void AddTransition(IState* from, IState* to, std::function<bool()> condition);
//
//Transition GetTransition();
//
//AIInput* Input;
//AIOutput* OutPut;
//IState* CurrentSate = nullptr;
//std::map<IState*, std::vector<Transition>> Transitions;
//std::vector<Transition> CurrentTransitions;
//
//void Tick(int moodValue, int currentMeasure)
//{
//	Input->MoodValue = moodValue;
//	Input->CurrentMeasure = currentMeasure;
//
//	Transition trasition = GetTransition();
//	int it = 0;
//
//	while (trasition.To != nullptr && it < 2)
//	{
//		SetState(trasition.To);
//		trasition = GetTransition();
//		it++;
//	};
//
//	if (CurrentSate != nullptr)
//		CurrentSate->Tick();
//
//	if (it < 2)
//		OutPut->Change = NORMAL;
//
//	std::cout << "Note: " << OutPut->FirstNote << ", Number: " << OutPut->NumberOfNotes
//		<< ", Chord: " << OutPut->Change << std::endl;
//}
//
//void SetState(IState* newSate)
//{
//	if (newSate == CurrentSate)
//		return;
//
//	if (CurrentSate != nullptr)
//		CurrentSate->OnExit();
//
//	CurrentSate = newSate;
//
//	CurrentTransitions = Transitions[CurrentSate];
//
//	CurrentSate->OnEnter();
//}
//
//Transition GetTransition()
//{
//	for (Transition currentTransition : CurrentTransitions)
//	{
//		if (currentTransition.Condition())
//			return currentTransition;
//	}
//
//	std::function<bool()> tt;
//	return Transition(nullptr, tt);
//}
//
//void AddTransition(IState* from, IState* to, std::function<bool()> condition)
//{
//	std::vector<Transition> transitions;
//	if (Transitions.find(from) == Transitions.end())
//		Transitions[from] = transitions;
//
//	Transitions[from].emplace_back(to, condition);
//}
//
//void AISetup()
//{
//	Input = new AIInput();
//
//	OutPut = new AIOutput();
//	OutPut->Change = NORMAL;
//	OutPut->FirstNote = ' ';
//	OutPut->NumberOfNotes = 1;
//
//	auto high = new MoodHigh(Input, OutPut);
//	auto midHigh = new MoodMidHigh(Input, OutPut);
//	auto mid = new MoodMid(Input, OutPut);
//	auto midLow = new MoodMidLow(Input, OutPut);
//	auto low = new MoodLow(Input, OutPut);
//
//	auto highToMidHigh = [] { return Input->MoodValue < 80; };
//
//	auto midHighToHigh = [] { return Input->MoodValue > 80; };
//	auto midHighToMid = [] { return Input->MoodValue < 60; };
//
//	auto midToMidHigh = [] {return Input->MoodValue > 60; };
//
//	auto midToMidLow = [] {return Input->MoodValue < 40; };
//
//	auto midLowToMid = [] { return Input->MoodValue > 40; };
//	auto midLowToLow = [] { return Input->MoodValue < 20; };
//
//	auto lowToMidLow = [] { return Input->MoodValue > 20; };
//
//	AddTransition(high, midHigh, highToMidHigh);
//
//	AddTransition(midHigh, high, midHighToHigh);
//	AddTransition(midHigh, mid, midHighToMid);
//
//	AddTransition(mid, midHigh, midToMidHigh);
//	AddTransition(mid, midLow, midToMidLow);
//
//	AddTransition(midLow, mid, midLowToMid);
//	AddTransition(midLow, low, midLowToLow);
//
//	AddTransition(low, midLow, lowToMidLow);
//
//	SetState(low);
//}

// fun way to generate music
//sequencer->PlayBar(&UserSensor, NGen::GetNewPhrase(rand() % 7 + 65, rand() % 5 + 3));

// Interesting beat sequences, found in a you-tube video
// https://www.youtube.com/watch?v=c7ffMObdxro&ab_channel=LANDR

	// impeach the president
	//sequencer.PlayBar(&Kick, "A......AA.A...A.");
	//sequencer.PlayBar(&Snare, "....A.......A...");
	//sequencer.PlayBar(&HitHat, "A.A.A.AAA...A.A.");

	// iconic eights
	//sequencer.PlayBar(&Kick, "A.........A.....");
	//sequencer.PlayBar(&Snare, "....A.......A...");
	//sequencer.PlayBar(&HitHat, "A.A.A.A.A.A.A.A.");

	// 4 on the floor  
	/*sequencer.PlayBar(&Kick, "A.........A.....");
	sequencer.PlayBar(&Snare, "....A.......A...");
	sequencer.PlayBar(&HitHat, "..A...A...A...A.");*/

	// two beat
	//sequencer.PlayBar(&Kick, "A.........A.....");
	//sequencer.PlayBar(&Snare, "....A.......A...");
	//sequencer.PlayBar(&HitHat, "A...A...A...A...");

	// boom-bap
	/*sequencer.PlayBar(&Kick, "AA......AA.A....");
	sequencer.PlayBar(&Snare, "....A..A....A..A");
	sequencer.PlayBar(&HitHat, "A.A.A.A.A.A.A.A.");*/