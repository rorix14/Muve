/*
This file contains the functionality that saves the stat history of a user session
*/

#pragma once

#include "StateMachine.h"
#include <string>

namespace Evalautor {
    struct EvaluationMetrics {
        int SensorMood;
        AI::ChordChange Change;

        EvaluationMetrics(int sensorMood, AI::ChordChange change) : SensorMood(sensorMood), Change(change) {}
    };

    // saves user session history
    std::vector<EvaluationMetrics> OutPutHistory;

    //float round(float var)
    //{
    //	float value = (int)(var * 100 + .5);
    //	return (float)value / 100;
    //}

    // evaluates a user session
    void EvalauteSession() {
        unsigned int numberOfSongLoops = OutPutHistory.empty() ? 1 : OutPutHistory.size();
        float numberOfInversions = 0;
        float numberOfDiminished = 0;
        unsigned int sensorAverage = 0;

        float highN = 0;
        float midHighN = 0;
        float midN = 0;
        float midLowN = 0;
        float lowN = 0;

        for (EvaluationMetrics output: OutPutHistory) {
            switch (output.Change) {
                case AI::INVERTED:
                    numberOfInversions++;
                case AI::DIMINISHED:
                    numberOfDiminished++;
                default:
                    break;
            }

            if (output.SensorMood < 20)
                lowN++;
            else if (output.SensorMood < 40)
                midLowN++;
            else if (output.SensorMood < 60)
                midN++;
            else if (output.SensorMood < 80)
                midHighN++;
            else
                highN++;

            sensorAverage += output.SensorMood;
        }

        sensorAverage /= numberOfSongLoops;

        const auto numberOfSongLoopsFloat = static_cast<float>(numberOfSongLoops);
        numberOfInversions = (numberOfInversions / numberOfSongLoopsFloat) * 100;
        numberOfDiminished = (numberOfDiminished / numberOfSongLoopsFloat) * 100;

        highN = (highN / numberOfSongLoopsFloat) * 100;
        midHighN = (midHighN / numberOfSongLoopsFloat) * 100;
        midN = (midN / numberOfSongLoopsFloat) * 100;
        midLowN = (midLowN / numberOfSongLoopsFloat) * 100;
        lowN = (lowN / numberOfSongLoopsFloat) * 100;

        std::string sessionStatesMessage;
        if (sensorAverage < 20)
            sessionStatesMessage.append("Very low movement over all, ");
        else if (sensorAverage < 40)
            sessionStatesMessage.append("Low movement over all, ");
        else if (sensorAverage < 60)
            sessionStatesMessage.append("Balanced movement over all, ");
        else if (sensorAverage < 80)
            sessionStatesMessage.append("High movement over all, ");
        else
            sessionStatesMessage.append("Very high movement over all, ");

        if (highN > midHighN && highN > midN && highN > midLowN && highN > lowN)
            sessionStatesMessage.append("spent the majority of time with very high movement, ");
        else if (midHighN > highN && midHighN > midN && midHighN > midLowN && midHighN > lowN)
            sessionStatesMessage.append("spent the majority of time with high movement, ");
        else if (midN > highN && midN > midHighN && midN > midLowN && midN > lowN)
            sessionStatesMessage.append("spent the majority of time with balanced movement, ");
        else if (midLowN > highN && midLowN > midHighN && midLowN > midN && midLowN > lowN)
            sessionStatesMessage.append("spent the majority of time with low movement, ");
        else
            sessionStatesMessage.append("spent the majority of time with very low movement, ");

        std::string inversionPercentage = "you drastically went from low to high movement "
                                          + std::to_string(numberOfInversions) + "% of time, ";

        std::string diminishedPercentage = "you drastically went from high to low movement "
                                           + std::to_string(numberOfDiminished) + "% of time";

        sessionStatesMessage.append(inversionPercentage);
        sessionStatesMessage.append(diminishedPercentage);

        std::cout << "Your play style was: " << sessionStatesMessage << std::endl;
    }
}