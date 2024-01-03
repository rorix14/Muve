/*
This file contains the functionality that saves the stat history of a user session
*/

#pragma once
#include "StateMachine.h"
#include <string>

namespace Evalautor
{
	struct EvaLautionMetrics
	{
		int SensorMood;
		AI::ChordChange Change;
	};

	// saves user session history
	std::vector<EvaLautionMetrics> OutPutHistory;

	//float round(float var)
	//{
	//	float value = (int)(var * 100 + .5);
	//	return (float)value / 100;
	//}

	// evaluates a user session
	void EvalauteSession()
	{
		int numberOfSongLoops = OutPutHistory.size();
		float numberOfInversions = 0;
		float numberOfDimished = 0;
		int sensorAvarage = 0;

		float hightN = 0;
		float midHighN = 0;
		float midN = 0;
		float midLowN = 0;
		float lowN = 0;

		for (EvaLautionMetrics output : OutPutHistory)
		{
			switch (output.Change)
			{
			case AI::INVERTED:
				numberOfInversions++;
			case AI::DIMINISHED:
				numberOfDimished++;
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
				hightN++;

			sensorAvarage += output.SensorMood;
		}

		sensorAvarage /= numberOfSongLoops;

		numberOfInversions = (numberOfInversions / numberOfSongLoops) * 100;
		numberOfDimished = (numberOfDimished / numberOfSongLoops) * 100;

		hightN = (hightN / numberOfSongLoops) * 100;
		midHighN = (midHighN / numberOfSongLoops) * 100;
		midN = (midN / numberOfSongLoops) * 100;
		midLowN = (midLowN / numberOfSongLoops) * 100;
		lowN = (lowN / numberOfSongLoops) * 100;

		std::string seesionStatesmessage;
		if (sensorAvarage < 20)
			seesionStatesmessage.append("Very low movement over all, ");
		else if (sensorAvarage < 40)
			seesionStatesmessage.append("Low movement over all, ");
		else if (sensorAvarage < 60)
			seesionStatesmessage.append("Balanced movement over all, ");
		else if (sensorAvarage < 80)
			seesionStatesmessage.append("High movement over all, ");
		else
			seesionStatesmessage.append("Very high movement over all, ");

		if(hightN > midHighN && hightN > midN && hightN > midLowN && hightN > lowN)
			seesionStatesmessage.append("spent the majority of time with very high movement, ");
		else if(midHighN > hightN && midHighN > midN && midHighN > midLowN && midHighN > lowN)
			seesionStatesmessage.append("spent the majority of time with high movement, ");
		else if (midN > hightN && midN > midHighN && midN > midLowN && midN > lowN)
			seesionStatesmessage.append("spent the majority of time with balanced movement, ");
		else if (midLowN > hightN && midLowN > midHighN && midLowN > midN && midLowN > lowN)
			seesionStatesmessage.append("spent the majority of time with low movement, ");
		else 
			seesionStatesmessage.append("spent the majority of time with very low movement, ");

		std::string inversionPercentage = "you drastically went from low to high movement "
			+ std::to_string(numberOfInversions) + "% of time, ";
	
		std::string dimishedPercentage = "you drastically went from high to low movement " 
		+ std::to_string(numberOfDimished) + "% of time";

		seesionStatesmessage.append(inversionPercentage);
		seesionStatesmessage.append(dimishedPercentage);

		std::cout << "Your play style was: " << seesionStatesmessage << std::endl;
	}
}