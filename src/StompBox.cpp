/**
 *  StompBox.cpp
 *
 *  Created by Marek Bereza on 24/06/2013.
 */

#include "StompBox.h"
#include "Parameters.h"

std::string patchParameterNames[5];
float patchParameterValues[5];

int Patch::getBlockSize() { return 256; }
double Patch::getSampleRate() { return 44100; }

void Patch::registerParameter(PatchParameterId pid, const std::string& name, const std::string& description) {
	patchParameterNames[pid] = name;
	
}

float Patch::getParameterValue(PatchParameterId pid) {
	return patchParameterValues[pid];
}
