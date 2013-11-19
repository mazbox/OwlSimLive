/**     ___           ___           ___                         ___           ___     
 *     /__/\         /  /\         /  /\         _____         /  /\         /__/|    
 *    |  |::\       /  /::\       /  /::|       /  /::\       /  /::\       |  |:|    
 *    |  |:|:\     /  /:/\:\     /  /:/:|      /  /:/\:\     /  /:/\:\      |  |:|    
 *  __|__|:|\:\   /  /:/~/::\   /  /:/|:|__   /  /:/~/::\   /  /:/  \:\   __|__|:|    
 * /__/::::| \:\ /__/:/ /:/\:\ /__/:/ |:| /\ /__/:/ /:/\:| /__/:/ \__\:\ /__/::::\____
 * \  \:\~~\__\/ \  \:\/:/__\/ \__\/  |:|/:/ \  \:\/:/~/:/ \  \:\ /  /:/    ~\~~\::::/
 *  \  \:\        \  \::/          |  |:/:/   \  \::/ /:/   \  \:\  /:/      |~~|:|~~ 
 *   \  \:\        \  \:\          |  |::/     \  \:\/:/     \  \:\/:/       |  |:|   
 *    \  \:\        \  \:\         |  |:/       \  \::/       \  \::/        |  |:|   
 *     \__\/         \__\/         |__|/         \__\/         \__\/         |__|/   
 *
 *  Description: 
 *				 
 *  PatchCompiler.h, created by Marek Bereza on 19/11/2013.
 */

#pragma once

#include "ofMain.h"
#include "SimpleGui.h"
#include "StompBox.h"
#define NUM_PARAMS 5





class PatchCompiler {
public:
	
	void setup(xmlgui::SimpleGui *gui);
	void setFile(string file);
	

	
	
	void checkSourceForUpdates();
	string getLastError();
	
	void lock();
	void unlock();
	
	vector<string> ctrlIds;
	
	float dummyParams[NUM_PARAMS];
	

	Patch *patch;
	
private:
	

	
	
	long getUpdateTime(ofFile &file);
	
	
	void *livecodeLib;
	void loadDylib(string file);
	void clean();
	bool compile();
	string linkObjects();
	ofFile hppFile;
	long updateTime;
	string lastError;
	string className;
	ofMutex *mutex;
	xmlgui::SimpleGui *gui;
	
};

