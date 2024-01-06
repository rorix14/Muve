#include "StateMachine.h"

namespace AI {
    const char AIMusicHelper::TwelveBarBluesCordProgression[12]
            {'A', 'D', 'A', 'A', 'D', 'D', 'A', 'A', 'E', 'D', 'A', 'E'};

    const std::map<int, float> AIMusicHelper::MeasureMultiplier
    {
        {0, 1.0f},
        {1, 1.0f},
        {2, 1.2f},
        {3, 1.3f},
        {4, 1.4f},
        {5, 1.4f},
        {6, 1.8f},
        {7, 1.8f},
        {8, 1.6f},
        {9, 1.4f},
        {10, 1.3f},
        {11, 1.2f}
    };

    const std::map<char, std::array<char, 3> > AIMusicHelper::BluesChordNotes
    {
        {'A', {'A', 'E', 'C'}},
        {'D', {'D', 'F', 'A'}},
        {'E', {'E', 'G', 'B'}}
    };

    StateMachine::StateMachine() : Input(new AIInput()), OutPut(new AIOutput()), _currentSate(nullptr) {
        OutPut->Change = NORMAL;
        OutPut->FirstNote = ' ';
        OutPut->NumberOfNotes = 1;

        auto *high = new MoodHigh(Input, OutPut);
        auto *midHigh = new MoodMidHigh(Input, OutPut);
        auto *mid = new MoodMid(Input, OutPut);
        auto *midLow = new MoodMidLow(Input, OutPut);
        auto *low = new MoodLow(Input, OutPut);

        AddTransition(high, midHigh, [this] { return Input->MoodValue < 80; });

        AddTransition(midHigh, high, [this] { return Input->MoodValue > 80; });
        AddTransition(midHigh, mid, [this] { return Input->MoodValue < 60; });

        AddTransition(mid, midHigh, [this] { return Input->MoodValue > 60; });
        AddTransition(mid, midLow, [this] { return Input->MoodValue < 40; });

        AddTransition(midLow, mid, [this] { return Input->MoodValue > 40; });
        AddTransition(midLow, low, [this] { return Input->MoodValue < 20; });

        AddTransition(low, midLow, [this] { return Input->MoodValue > 20; });

        SetState(low);
    }

    StateMachine::~StateMachine() {
        for (const auto &test: _transitions)
            delete test.first;

        delete Input;
        delete OutPut;
    }

    void StateMachine::Tick(const int &moodValue, const int &currentMeasure) {
        Input->MoodValue = moodValue;
        Input->CurrentMeasure = currentMeasure;

        Transition transition = GetTransition();
        int it = 0;

        while (transition.To != nullptr && it < 2) {
            SetState(transition.To);
            transition = GetTransition();
            it++;
        }

        if (_currentSate != nullptr)
            _currentSate->Tick();

        if (it < 2)
            OutPut->Change = NORMAL;

        /*std::cout << "Note: " << OutPut->FirstNote << ", Number: " << OutPut->NumberOfNotes
            << ", Chord: " << OutPut->Change << std::endl;*/
    }

    void StateMachine::SetState(IState *newSate) {
        if (newSate == _currentSate)
            return;

        if (_currentSate != nullptr)
            _currentSate->OnExit();

        _currentSate = newSate;

        _currentTransitions = _transitions[_currentSate];

        _currentSate->OnEnter();
    }

    void StateMachine::AddTransition(IState *from, IState *to, const std::function<bool()> &condition) {
        if (_transitions.find(from) == _transitions.end()) {
            const std::vector<Transition> transitions;
            _transitions[from] = transitions;
        }

        _transitions[from].emplace_back(to, condition);
    }

    Transition StateMachine::GetTransition() const {
        for (Transition currentTransition: _currentTransitions) {
            if (currentTransition.Condition())
                return currentTransition;
        }

        std::function<bool()> tt;
        return {nullptr, tt};
    }
}
