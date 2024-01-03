#include "StateMachine.h"

namespace AI
{
	const char AIMusicHelper::TwelveBarBluesCordProgression[12]
	{ 'A', 'D', 'A', 'A', 'D', 'D', 'A', 'A', 'E', 'D', 'A', 'E' };

	const std::map<int, float> AIMusicHelper::MeasureMultiplier
	{
		{0, 1.0f}, {1, 1.0f} ,{2, 1.2f},{3, 1.3f},{4, 1.4f},{5, 1.4f},
		{6, 1.8f}, {7, 1.8f}, {8, 1.6f} ,{9, 1.4f},{10, 1.3f},{11, 1.2f}
	};

	const std::map<char, std::array<char, 3>> AIMusicHelper::BluesChordNotes
	{
		{'A', {'A', 'E', 'C'}}, {'D', {'D', 'F', 'A'}}, {'E', {'E', 'G', 'B'}}
	};

	StateMachine::StateMachine()
	{
		Input = new AIInput();

		OutPut = new AIOutput();
		OutPut->Change = NORMAL;
		OutPut->FirstNote = ' ';
		OutPut->NumberOfNotes = 1;
		
		MoodHigh* high = new MoodHigh(Input, OutPut);
		MoodMidHigh* midHigh = new MoodMidHigh(Input, OutPut);
		MoodMid* mid = new MoodMid(Input, OutPut);
		MoodMidLow* midLow = new MoodMidLow(Input, OutPut);
		MoodLow* low = new MoodLow(Input, OutPut);

		auto highToMidHigh = [this] { return Input->MoodValue < 80; };

		auto midHighToHigh = [this] { return Input->MoodValue > 80; };
		auto midHighToMid = [this] { return Input->MoodValue < 60; };

		auto midToMidHigh = [this] {return Input->MoodValue > 60; };

		auto midToMidLow = [this] {return Input->MoodValue < 40;};

		auto midLowToMid = [this] { return Input->MoodValue > 40; };
		auto midLowToLow = [this] { return Input->MoodValue < 20; };

		auto lowToMidLow = [this] { return Input->MoodValue > 20; };

		AddTransition(high, midHigh, highToMidHigh);

		AddTransition(midHigh, high, midHighToHigh);
		AddTransition(midHigh, mid, midHighToMid);

		AddTransition(mid, midHigh, midToMidHigh);
		AddTransition(mid, midLow, midToMidLow);

		AddTransition(midLow, mid, midLowToMid);
		AddTransition(midLow, low, midLowToLow);

		AddTransition(low, midLow, lowToMidLow);

		SetState(low);
	}

	StateMachine::~StateMachine()
	{
		delete Input;
		delete OutPut;
	}

	void StateMachine::Tick(const int& moodValue, const int& currentMeasure)
	{
		Input->MoodValue = moodValue;
		Input->CurrentMeasure = currentMeasure;

		Transition trasition = GetTransition();
		int it = 0;

		while (trasition.To != nullptr && it < 2)
		{
			SetState(trasition.To);
			trasition = GetTransition();
			it++;
		};

		if (_currentSate != nullptr)
			_currentSate->Tick();

		if (it < 2)
			OutPut->Change = NORMAL;

		/*std::cout << "Note: " << OutPut->FirstNote << ", Number: " << OutPut->NumberOfNotes
			<< ", Chord: " << OutPut->Change << std::endl;*/
	}

	void StateMachine::SetState(IState* newSate)
	{
		if (newSate == _currentSate)
			return;

		if (_currentSate != nullptr)
			_currentSate->OnExit();

		_currentSate = newSate;

		_currentTransitions = _transitions[_currentSate];

		_currentSate->OnEnter();
	}

	void StateMachine::AddTransition(IState* from, IState* to, std::function<bool()> condition)
	{
		std::vector<Transition> transitions;
		if (_transitions.find(from) == _transitions.end())
			_transitions[from] = transitions;

		_transitions[from].emplace_back(to, condition);
	}

	Transition StateMachine::GetTransition()
	{
		for (Transition currentTransition : _currentTransitions)
		{
			if (currentTransition.Condition())
				return currentTransition;
		}

		std::function<bool()> tt;
		return Transition(nullptr, tt);
	}
}
