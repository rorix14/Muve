/*
File contains functionality for the application AI. It implements a finite state machine that has five states,
each state is directly correlated with the sensor values. Every state has different rules that affect the output.
States are connected to each other via transitions.
*/

#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <array>
#include <functional>

namespace AI
{
	enum ChordChange
	{
		NORMAL = 0,
		INVERTED = 1,
		DIMINISHED = -1
	};

	// variables that never change during the application run time.
	// they are helpful for calculating the AI output
	struct AIMusicHelper
	{
		static const char TwelveBarBluesCordProgression[12];
		static const std::map<int, float> MeasureMultiplier;
		static const std::map<char, std::array<char, 3>> BluesChordNotes;
	};

	// abstract class, and base class for every state
	class IState
	{
	public:
		virtual ~IState() = default;

		virtual void Tick() = 0;

		virtual void OnEnter() = 0;

		virtual void OnExit() = 0;
	};

	// structures for AI inputs and outputs
	struct AIInput
	{
		int MoodValue;
		int CurrentMeasure;
	};

	struct AIOutput
	{
		char FirstNote;
		float NumberOfNotes;
		ChordChange Change;
	};

	// transition class, used to define state transitions. Could be done in several ways, like being an
	// array variable in the states them selfs, but for this implementation they are saved in a map where the key
	// is the class to transition from
	struct Transition
	{
		Transition(IState* to, std::function<bool()> condition) : To(to), Condition(condition) {};

		std::function<bool()> Condition;

		IState* To;
	};

	// class that manages all the sates and their transitions
	class StateMachine
	{
	public:
		StateMachine();

		~StateMachine();

		void Tick(const int& moodValue, const int& currentMeasure);

		AIInput* Input;
		AIOutput* OutPut;

	private:
		void AddTransition(IState* from, IState* to, std::function<bool()> condition/* bool(*condition)()*/);

		void SetState(IState* newSate);

		Transition GetTransition();

		IState* _currentSate;

		std::map<IState*, std::vector<Transition>> _transitions;

		std::vector<Transition> _currentTransitions;

		std::vector<Transition> _emptyTransitions;
	};

	// State machine available states
	// written in header file for convenience 
	class MoodHigh : public IState
	{
	public:
		MoodHigh(AIInput* input, AIOutput* output) :
			_input(input), _outPut(output), _numberOfNotesMultiplier(6), _moodNotes{ 'C', 'F', 'G' }{};

		void Tick() override
		{
			std::cout << "High\n";
			int currentMeasure = _input->CurrentMeasure;

			_outPut->NumberOfNotes = _numberOfNotesMultiplier * AIMusicHelper::MeasureMultiplier.at(currentMeasure);

			std::array<char, 3> chordNotes = AIMusicHelper::BluesChordNotes
				.at(AIMusicHelper::TwelveBarBluesCordProgression[currentMeasure]);

			for (char cordNote : chordNotes)
			{
				for (char moodNote : _moodNotes)
				{
					if (moodNote == cordNote && moodNote != _outPut->FirstNote)
					{
						_outPut->FirstNote = moodNote;
						return;
					}
				}
				_outPut->FirstNote = 'B';
			}
		};

		void OnEnter() override
		{
			//std::cout << "High Enter\n";
			_outPut->Change = INVERTED;
		};

		void OnExit() override
		{
			//std::cout << "High Exit\n";
			_outPut->Change = DIMINISHED;
		};

	private:
		AIInput* _input;
		AIOutput* _outPut;
		int _numberOfNotesMultiplier;
		char _moodNotes[3];
	};

	class MoodMidHigh : public IState
	{
	public:
		MoodMidHigh(AIInput* input, AIOutput* output) :
			_input(input), _outPut(output), _numberOfNotesMultiplier(5), _moodNotes{ 'C', 'F', 'G' }{};

		void Tick() override
		{
			std::cout << "Mid High\n";

			int currentMeasure = _input->CurrentMeasure;

			_outPut->NumberOfNotes = _numberOfNotesMultiplier * AIMusicHelper::MeasureMultiplier.at(currentMeasure);

			std::array<char, 3> chordNotes = AIMusicHelper::BluesChordNotes
				.at(AIMusicHelper::TwelveBarBluesCordProgression[currentMeasure]);

			for (char cordNote : chordNotes)
			{
				for (char moodNote : _moodNotes)
				{
					if (moodNote == cordNote && moodNote != _outPut->FirstNote)
					{
						_outPut->FirstNote = moodNote;
						return;
					}
				}
				_outPut->FirstNote = 'B';
			}
		};

		void OnEnter() override
		{
			//std::cout << "Mid High Enter\n";
			if (_outPut->Change == NORMAL)
				_outPut->Change = INVERTED;
		};

		void OnExit() override
		{
			//std::cout << "Mid High Exit\n";
			if (_outPut->Change == NORMAL)
				_outPut->Change = DIMINISHED;
		};

	private:
		AIInput* _input;
		AIOutput* _outPut;
		int _numberOfNotesMultiplier;
		char _moodNotes[3];
	};

	class MoodMid : public IState
	{
	public:
		MoodMid(AIInput* input, AIOutput* output) :
			_input(input), _outPut(output), _numberOfNotesMultiplier(4), _moodNotes{ 'A', 'B', 'C', 'D', 'E', 'F', 'G' }{};

		void Tick() override
		{
			std::cout << "Mid\n";
			int currentMeasure = _input->CurrentMeasure;

			_outPut->NumberOfNotes = _numberOfNotesMultiplier * AIMusicHelper::MeasureMultiplier.at(currentMeasure);

			std::array<char, 3> chordNotes = AIMusicHelper::BluesChordNotes
				.at(AIMusicHelper::TwelveBarBluesCordProgression[currentMeasure]);

			for (char cordNote : chordNotes)
			{
				for (char moodNote : _moodNotes)
				{
					if (moodNote == cordNote && moodNote != _outPut->FirstNote)
					{
						_outPut->FirstNote = moodNote;
						return;
					}
				}
				_outPut->FirstNote = 'B';
			}
		};

		void OnEnter() override
		{
			//std::cout << "Mid Enter\n";
			/*if (_outPut->Change < 0)
				_outPut->Change = INVERTED;*/
				//std::cout << "mood input: " << _context->Input.MoodValue << std::endl;
		};

		void OnExit() override
		{
			//std::cout << "Mid Exit\n";
			/*if (_outPut->Change < 0)
				_outPut->Change = DIMINISHED;*/
				//std::cout << "mood input: " << _context->Input.MoodValue << std::endl;
		};

	private:
		AIInput* _input;
		AIOutput* _outPut;
		int _numberOfNotesMultiplier;
		char _moodNotes[7];
	};

	class MoodMidLow : public IState
	{
	public:
		MoodMidLow(AIInput* input, AIOutput* output) :
			_input(input), _outPut(output), _numberOfNotesMultiplier(3), _moodNotes{ 'A', 'D', 'E' }{};

		void Tick() override
		{
			std::cout << "Mid Low\n";
			int currentMeasure = _input->CurrentMeasure;

			_outPut->NumberOfNotes = _numberOfNotesMultiplier * AIMusicHelper::MeasureMultiplier.at(currentMeasure);

			std::array<char, 3> chordNotes = AIMusicHelper::BluesChordNotes
				.at(AIMusicHelper::TwelveBarBluesCordProgression[currentMeasure]);

			for (char cordNote : chordNotes)
			{
				for (char moodNote : _moodNotes)
				{
					if (moodNote == cordNote && moodNote != _outPut->FirstNote)
					{
						_outPut->FirstNote = moodNote;
						return;
					}
				}
				_outPut->FirstNote = 'B';
			}
		};

		void OnEnter() override
		{
			//std::cout << "Mid Low Enter\n";
			if (_outPut->Change == NORMAL)
				_outPut->Change = DIMINISHED;
			//std::cout << "ChordChange input: " << _outPut->Change << std::endl;
		};

		void OnExit() override
		{
			//std::cout << "Mid Low Exit\n";
			if (_outPut->Change == NORMAL)
				_outPut->Change = INVERTED;
		};

	private:
		AIInput* _input;
		AIOutput* _outPut;
		int _numberOfNotesMultiplier;
		char _moodNotes[3];
	};

	class MoodLow : public IState
	{
	public:
		MoodLow(AIInput* input, AIOutput* output) :
			_input(input), _outPut(output), _numberOfNotesMultiplier(2), _moodNotes{ 'A', 'D', 'E' }{};

		void Tick() override
		{
			std::cout << "Low\n";
			int currentMeasure = _input->CurrentMeasure;

			_outPut->NumberOfNotes = _numberOfNotesMultiplier * AIMusicHelper::MeasureMultiplier.at(currentMeasure);

			std::array<char, 3> chordNotes = AIMusicHelper::BluesChordNotes
				.at(AIMusicHelper::TwelveBarBluesCordProgression[currentMeasure]);

			for (char cordNote : chordNotes)
			{
				for (char moodNote : _moodNotes)
				{
					if (moodNote == cordNote && moodNote != _outPut->FirstNote)
					{
						_outPut->FirstNote = moodNote;
						return;
					}
				}
				_outPut->FirstNote = 'B';
			}
		};

		void OnEnter() override
		{
			//std::cout << "Low Enter\n";
			_outPut->Change = DIMINISHED;
		};

		void OnExit() override
		{
			//std::cout << "Low Exit\n";
			_outPut->Change = INVERTED;
			//std::cout << "ChordChange input: " << _outPut->Change << std::endl;
		};

	private:
		AIInput* _input;
		AIOutput* _outPut;
		int _numberOfNotesMultiplier;
		char _moodNotes[3];
	};
}
