/**
 *  PatchCompiler.cpp
 *
 *  Created by Marek Bereza on 19/11/2013.
 */



#include "PatchCompiler.h"

#include <dlfcn.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "MutePatch.h"



string execute(string cmd) {
	printf("Executing %s\n", cmd.c_str());
	cmd += " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
	printf("%s\n", result.c_str());
    return result;
}





string PatchCompiler::linkObjects() {
	string dylibName = ofGetTimestampString()+"-livecode.dylib";
	execute("g++ -arch i386 -dynamiclib -o /tmp/livecode/"+dylibName+" /tmp/livecode/*.o ");
	return dylibName;
}



void PatchCompiler::checkSourceForUpdates() {
	if(hppFile.path()=="") {
		return;
	}
	bool compiled = false;
	
	if(getUpdateTime(hppFile)!=updateTime) {
		compile();
		compiled = true;
	}
	
	if(compiled) {
		string dylibName = linkObjects();
		loadDylib("/tmp/livecode/"+dylibName);
	}
}






void PatchCompiler::setFile(string file) {
	
	hppFile = ofFile(file);
	updateTime = getUpdateTime(hppFile);
	
	
	clean();
	compile();
	string dylibName = linkObjects();
	loadDylib("/tmp/livecode/"+dylibName);
}



long PatchCompiler::getUpdateTime(ofFile &file) {
	
	struct stat fileStat;
    if(stat(file.path().c_str(), &fileStat) < 0) {
		printf("Couldn't stat file\n");
		return;
	}
	
	return fileStat.st_mtime;
}


void PatchCompiler::clean() {
	execute("rm -Rf /tmp/livecode");
	execute("mkdir -p /tmp/livecode");
}





string PatchCompiler::getLastError() {
	return lastError;
}
bool PatchCompiler::compile() {
	string includes = "-I\"" + hppFile.getEnclosingDirectory() + "\" -I\"" + ofToDataPath("..") + "\"";
	updateTime = getUpdateTime(hppFile);
	string basename = hppFile.getBaseName();
	string hpp = hppFile.path();
	string cpp = "/tmp/livecode/" + hppFile.getBaseName() + ".cpp";
	
	printf("Creating %s\n", cpp.c_str());
	className = hppFile.getBaseName();
	ofstream myfile;
	myfile.open (cpp.c_str());
	//	myfile << "#include <math.h>";
	//	myfile << "#define assert_param(A) if(A){}";
	
	myfile << 	"#include \""+hppFile.getFileName()+"\"\n";
	myfile << "extern \"C\" { Patch *getPatch() { return new " + hppFile.getBaseName() + "(); }};\n";
	myfile << "std::string patchParameterNames[5];\n";
	myfile << "float patchParameterValues[5];\n";
	
	myfile << 	"int Patch::getBlockSize() { return 256; }\n";
	myfile << 	"double Patch::getSampleRate() { return 44100; }\n";
	myfile <<   "AudioBuffer::~AudioBuffer() {";
	myfile << 	"}";
	myfile << 	"Patch::~Patch() {";
	myfile << 	"}";
	myfile << 	"Patch::Patch() {";
	myfile << 	"}";
	myfile << 	"void Patch::registerParameter(PatchParameterId pid, const std::string& name, const std::string& description) {patchParameterNames[pid] = name;}\n";
	myfile << 	"float Patch::getParameterValue(PatchParameterId pid) {return patchParameterValues[pid];}\n";
	myfile << "extern \"C\" { float *getPtrForParameter(PatchParameterId pid) { return &patchParameterValues[pid]; };\n";
	myfile << "std::string getNameForParameter(PatchParameterId pid) { return patchParameterNames[pid]; };\n};\n";
	myfile.close();
	
	execute("cp " + ofToDataPath("StompBox.h") + " /tmp/livecode/");
	execute("cp " + ofToDataPath("StompBox.h") + " /tmp/livecode/");
	
	string result = execute("g++ -arch i386 -c \""+cpp+"\" " +includes+" -I/tmp/livecode -o /tmp/livecode/"+basename+".o");
	//	printf(">>>> %ld '%s'\n",result.size(), result.c_str());
	lastError = result;
	if(result.size()!=0) ofBackground(100, 0,0);
	else ofBackground(0);
	return result.size()==0;
}



void PatchCompiler::setup(xmlgui::SimpleGui *gui) {
	this->gui = gui;
	patch = new MutePatch();
	mutex = new ofMutex();
	
	livecodeLib = NULL;
	for(int i = 0; i < NUM_PARAMS; i++) {
		// fill an array with the letters, A, B, C, D...
		char c[2];
		c[0] = 'A' + i;
		c[1] = '\0';
		string letter(c);
		ctrlIds.push_back(letter);
	}
}


void PatchCompiler::loadDylib(string file) {
	mutex->lock();
	if(livecodeLib!=NULL) {
		// remember previous slider positions
		for(int i = 0; i < ctrlIds.size(); i++) {
			dummyParams[i] = gui->getControlById(ctrlIds[i])->getFloat();
		}
		delete patch;
		dlclose(livecodeLib);
		
		livecodeLib = NULL;
	}
	
	
	livecodeLib = dlopen(file.c_str(), RTLD_LAZY);
	
	if (livecodeLib == NULL) {
		// report error ...
		printf("Error: No dice loading %s\n", file.c_str());
	} else {
		
		// use the result in a call to dlsym
		printf("Success loading\n");
		void *paramPtrFunc = dlsym(livecodeLib, "getPtrForParameter");
		void *paramNameFunc = dlsym(livecodeLib, "getNameForParameter");
		void *ptrFunc = dlsym(livecodeLib, "getPatch");
		
		if(ptrFunc!=NULL && paramPtrFunc!=NULL && paramNameFunc != NULL) {
			
			ofSetWindowTitle(className);
			
			for(int i = 0; i < ctrlIds.size(); i++) {
				gui->getControlById(ctrlIds[i])->pointToValue(((float *(*)(int))paramPtrFunc)(i));
				gui->getControlById(ctrlIds[i])->setValue(dummyParams[i]);
			}
			
			
			
			
			patch = ((Patch *(*)())ptrFunc)();
			
			for(int i = 0; i < ctrlIds.size(); i++) {
				gui->getControlById(ctrlIds[i])->name =((string(*)(int))paramNameFunc)(i);
				printf("Control Name: %s\n", gui->getControlById(ctrlIds[i])->name.c_str());
			}
			
			
		} else {
			if(ptrFunc==NULL) printf("Couldn't find the getPatch() function\n");
			if(paramPtrFunc==NULL) printf("Couldn't find the getPtrForParameter() function\n");
			if(paramNameFunc==NULL) printf("Couldn't find the getNameForParameter() function\n");
		}
	}
	mutex->unlock();
	
}







void PatchCompiler::lock() {
	mutex->lock();
}

void PatchCompiler::unlock() {
	mutex->unlock();
}
