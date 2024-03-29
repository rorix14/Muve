/*
	This file contains all the helper functions that generate a sequence of notes
	at a specific number of required notes
*/

#pragma once

#include <iostream>
#include <algorithm>
#include <random>

namespace NGen {
    // Rules are made for the A minor scale
    std::map<char, char> RuleOfThirdsAMinor{
        {'A', 'C'},
        {'B', 'D'},
        {'C', 'E'},
        {'D', 'F'},
        {'E', 'G'},
        {'F', 'A'},
        {'G', 'B'}
    };

    std::map<char, char> RuleOfHarmonyAMinor{
        {'B', 'C'},
        {'D', 'C'},
        {'F', 'E'},
        {'G', 'A'}
    };

    std::string ActiveNotesAMinor = "BDFG";

    // Rules are made for the A major
    std::map<char, char> RuleOfThirdsAMajor{
        {'A', 'c'},
        {'B', 'D'},
        {'c', 'E'},
        {'D', 'f'},
        {'E', 'g'},
        {'f', 'A'},
        {'g', 'B'}
    };

    std::map<char, char> RuleOfHarmonyAMajor{
        {'B', 'c'},
        {'D', 'c'},
        {'f', 'E'},
        {'g', 'A'}
    };

    std::string activeNotesAMajor = "BDfg";

    // Array containing all rules |MIGHT NOT BE NECESSARY|
    std::map<char, char> Rules[] = {RuleOfThirdsAMinor, RuleOfHarmonyAMinor};

    // Full note not included because it can only be played if we have one note third
    std::vector<std::string> NoteTypes{"x.......", "x...", "x.", "x"};

    // Used to randomly shuffle the note types array
    std::default_random_engine rng = std::default_random_engine{};

    // variables used for testing
    enum ChordChange {
        NORMAL,
        INVERTED,
        DIMINISHED
    };

    struct AIOutput {
        char Note;
        int NumberOfNotes;
        ChordChange chordChange;

        AIOutput(const char &note, const int &numberOfNotes, const ChordChange &change)
            : Note(note), NumberOfNotes(numberOfNotes), chordChange(change) {
        }
    };

    // Find a musically correct sequence of beats given a number of notes
    inline std::string FindSequence(std::string phrase, const int &numberOfNotes, int &accumulatedNotes,
                                    const unsigned int currentNote) {
        if (phrase.size() >= 16 || accumulatedNotes >= numberOfNotes)
            return phrase;

        for (unsigned int i = currentNote; i < NoteTypes.size(); i++) {
            if (NoteTypes[i].size() + phrase.size() > 16)
                continue;

            phrase += NoteTypes[i];
            accumulatedNotes++;

            phrase = FindSequence(phrase, numberOfNotes, accumulatedNotes, i);

            if (phrase.size() == 16 && numberOfNotes == accumulatedNotes)
                return phrase;

            phrase = phrase.substr(0, phrase.size() - NoteTypes[i].size());
            accumulatedNotes--;
        }

        return phrase;
    }

    // generate a sequence of beats in a measure
    inline std::string GenerateBeatSequence(const int &numberOfNotes) {
        if (numberOfNotes == 1)
            return "x...............";
        if (numberOfNotes == 2)
            return "x.......x.......";
        if (numberOfNotes == 16)
            return "xxxxxxxxxxxxxxxx";

        const std::string phrase;
        int accumulatedNotes = 0;

        std::shuffle(NoteTypes.begin(), NoteTypes.end(), rng);

        return FindSequence(phrase, numberOfNotes, accumulatedNotes, 0);
    }

    // Generates the notes that will be played in a measure, based on starting note and the number of notes requested
    inline std::string GenerateNotes(const char &axium, const unsigned int &numberOfNotes) {
        std::string notes;
        notes += axium;

        for (unsigned int i = 0; i < numberOfNotes - 1; i++) {
            char lastSequenceNote = notes[notes.size() - 1];
            char note;
            const int randValue = rand() % 10;

            if (randValue < 1)
                note = lastSequenceNote;
            else if (ActiveNotesAMinor.find(lastSequenceNote) != std::string::npos && randValue < 6)
                note = RuleOfHarmonyAMinor[lastSequenceNote];
            else
                note = RuleOfThirdsAMinor[lastSequenceNote];

            notes += note;
        }

        return notes;
    }

    inline char GenerateNote(const char &axium) {
        const char lastSequenceNote = axium;
        char note;
        const int randValue = rand() % 10;

        if (randValue < 1)
            note = lastSequenceNote;
        else if (ActiveNotesAMinor.find(lastSequenceNote) != std::string::npos && randValue < 6)
            note = RuleOfHarmonyAMinor[lastSequenceNote];
        else
            note = RuleOfThirdsAMinor[lastSequenceNote];

        return note;
    }

    inline std::string NexGeneration(const std::string &current) {
        std::string next(1, current[0]);
        for (const char &note: current) {
            const char c = GenerateNote(note);
            next += c;
        }

        return next;
    }

    inline std::string LSystem(const std::string &axium, unsigned int numberOfNotes) {
        if (axium.size() >= numberOfNotes)
            return axium;

        return LSystem(NexGeneration(axium), numberOfNotes);
    }

    inline std::string GetNewPhrase(const int numberOfNotes, const char &firstNote) {
        const std::string notes = GenerateNotes(firstNote, numberOfNotes);
        //std::string notes = LSystem({ 65, firstNote }, numberOfNotes);
        const std::string beatSequence = GenerateBeatSequence(numberOfNotes);

        std::string finalSequence;
        int notesUsed = 0;
        for (const char beat: beatSequence) {
            if (beat == 'x') {
                finalSequence += notes[notesUsed];
                notesUsed++;
            } else
                finalSequence += beat;
        }

        std::cout << finalSequence << std::endl;
        return finalSequence;
    }
}
