#include "testApp.h"

#include <dlfcn.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "SimpleGui.h"
#include "Parameters.h"
#include "MutePatch.h"
#include "InBuffer.h"
#include "OutBuffer.h"

#define NUM_PARAMS 5
#define GUI_WIDTH 500

InBuffer ins;
OutBuffer outs;

bool active = true;


xmlgui::SimpleGui gui;

ofMutex *mutex;
Patch *patch = NULL;

ofFile hppFile;
long updateTime;
vector<string> ctrlIds;

float inBuff[8192];

float inLevelL = 0;
float inLevelR = 0;
int numInputChannels = 1;
int numOutputChannels = 1;



void *livecodeLib = NULL;



float dummyParams[NUM_PARAMS];



std::string execute(string cmd) {
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
	printf("Got %s\n", result.c_str());
    return result;
}




string linkObjects() {
	string dylibName = ofGetTimestampString()+"-livecode.dylib";
	execute("g++ -arch i386 -dynamiclib -o /tmp/livecode/"+dylibName+" /tmp/livecode/*.o ");
	return dylibName;
}


void clean() {
	execute("rm -Rf /tmp/livecode");
	execute("mkdir -p /tmp/livecode");
}





void testApp::loadDylib(string file) {
	mutex->lock();
	if(livecodeLib!=NULL) {
		// remember previous slider positions
		for(int i = 0; i < ctrlIds.size(); i++) {
			dummyParams[i] = gui.getControlById(ctrlIds[i])->getFloat();
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
			
			
			
			for(int i = 0; i < ctrlIds.size(); i++) {
				gui.getControlById(ctrlIds[i])->pointToValue(((float *(*)(int))paramPtrFunc)(i));
				gui.getControlById(ctrlIds[i])->setValue(dummyParams[i]);
			}


					
			
			patch = ((Patch *(*)())ptrFunc)();
			
			for(int i = 0; i < ctrlIds.size(); i++) {
				gui.getControlById(ctrlIds[i])->name =((string(*)(int))paramNameFunc)(i);
				printf("Control Name: %s\n", gui.getControlById(ctrlIds[i])->name.c_str());
			}
			
			
		} else {
			if(ptrFunc==NULL) printf("Couldn't find the getPatch() function\n");
			if(paramPtrFunc==NULL) printf("Couldn't find the getPtrForParameter() function\n");
			if(paramNameFunc==NULL) printf("Couldn't find the getNameForParameter() function\n");
		}
	}
	mutex->unlock();
	
}

void styleKnob(Knob *k, int pos) {
	k->width = 100;
	k->height = k->width;
	k->y = 10;
	if(pos==4) { // put the 5th knob below the fourth
		pos = 3;
		k->y += k->height + 20;
	}
	k->x = 10 + (k->width + 20)*pos;
	
}

//--------------------------------------------------------------
void testApp::setup(){
	ofSetCircleResolution(100);
	for(int i = 0; i < NUM_PARAMS; i++) {
		// fill an array with the letters, A, B, C, D...
		char c[2];
		c[0] = 'A' + i;
		c[1] = '\0';
		string letter(c);
		ctrlIds.push_back(letter);
	}

	// set data path to app/Contents/Resources
	string r = ofToDataPath("", true);
	ofSetDataPathRoot(r.substr(0, r.size()-20)+"Resources/data/");

	
	mutex = new ofMutex();
	patch = new MutePatch();
	
	ofBackground(0);
	ofSetFrameRate(60);
	
	gui.setEnabled(true);
	gui.setAutoLayout(false);
	
	for(int i = 0; i < ctrlIds.size(); i++) {
		dummyParams[i] = 0;
		Knob *knob = gui.addKnob("Parameter "+ctrlIds[i], dummyParams[i]);
		knob->id = ctrlIds[i];
		styleKnob(knob, i);

	}
	Toggle *bypass = gui.addToggle("", active);
	bypass->width = bypass->height = 30;
	bypass->x = GUI_WIDTH/2 - bypass->width/2;
	bypass->y = 300;

	
	ofSetWindowShape(1024, 400);
	
	ofSoundStreamSetup(2, 2, this, 44100, 256, 1);
}



void testApp::audioIn( float * input, int bufferSize, int nChannels ) {
	assert(numInputChannels<3);
	if(numInputChannels==2) {
		memcpy(inBuff, input, bufferSize*nChannels*sizeof(float));
	} else {
		for(int i = 0; i < bufferSize; i++) {
			inBuff[i] = (input[i*2] + input[i*2+1])*0.5f;
		}
	}
	
	if(numInputChannels==2) {
		for(int i = 0; i < bufferSize; i++) {
			if(ABS(input[i*2])>inLevelL) {
				inLevelL = ABS(input[i*2]);
			} else {
				inLevelL *= 0.999;
			}
			if(ABS(input[i*2+1])>inLevelR) {
				
				inLevelR = ABS(input[i*2+1]);
			} else {
				inLevelR *= 0.999;
			}
		}
	} else {
		for(int i = 0; i < bufferSize; i++) {
			if(ABS(input[i])>inLevelL) {
				inLevelL = ABS(input[i]);
			} else {
				inLevelL *= 0.999;
			}
		}
	}
}

float outLevelL = 0;
float outLevelR = 0;




void testApp::audioOut( float * output, int bufferSize, int nChannels ) {
	assert(numOutputChannels<3);
	mutex->lock();
	
	ins.size = bufferSize;
	ins.samples = inBuff;
	
	outs.size = bufferSize;
	
	if(numOutputChannels==1) {
		// yes, I know.
		outs.samples = new float[bufferSize];
	} else {
		outs.samples = output;
	}
	if(active) {
		patch->processAudio(ins, outs);
	} else {
		assert(numInputChannels==numOutputChannels);
		memcpy(outs.samples, ins.samples, bufferSize*numInputChannels*sizeof(float));
	}
	
	if(numOutputChannels==1) {
		for(int i = 0; i < bufferSize; i++) {
			output[i*2] = output[i*2+1] = outs.samples[i];
		}
		delete [] outs.samples;
	}
	mutex->unlock();
	
	for(int i = 0; i < nChannels*bufferSize; i++) {
		if(output[i]>1) output[i] = 1;
		else if(output[i] <-1) output[i] = -1;
		//output[i] *= 0.5;
	}
	
	for(int i = 0; i < bufferSize; i++) {
		if(ABS(output[i*2])>outLevelL) {
			outLevelL = ABS(output[i*2]);
		} else {
			outLevelL *= 0.999;
		}
		if(ABS(output[i*2+1])>outLevelR) {
			
			outLevelR = ABS(output[i*2+1]);
		} else {
			outLevelR *= 0.999;
		}
	}
}


long getUpdateTime(ofFile &file) {
	
	struct stat fileStat;
    if(stat(file.path().c_str(), &fileStat) < 0) {
		printf("Couldn't stat file\n");
		return;
	}
	
	return fileStat.st_mtime;
}

string lastError = "";

bool compile() {
	string includes = "-I" + hppFile.getEnclosingDirectory() + " -I" + ofToDataPath("..");
	updateTime = getUpdateTime(hppFile);
	string basename = hppFile.getBaseName();
	string hpp = hppFile.path();
	string cpp = "/tmp/livecode/" + hppFile.getBaseName() + ".cpp";
	
	printf("Creating %s\n", cpp.c_str());
	
	ofstream myfile;
	myfile.open (cpp.c_str());
	myfile << 	"#include \""+hppFile.getFileName()+"\"\n";
	myfile << "extern \"C\" { Patch *getPatch() { return new " + hppFile.getBaseName() + "(); }};\n";
	myfile << "std::string patchParameterNames[5];\n";
	myfile << "float patchParameterValues[5];\n";
	
	myfile << 	"int Patch::getBlockSize() { return 256; }\n";
	myfile << 	"double Patch::getSampleRate() { return 44100; }\n";
	myfile << 	"void Patch::registerParameter(PatchParameterId pid, const std::string& name, const std::string& description) {patchParameterNames[pid] = name;}\n";
	myfile << 	"float Patch::getParameterValue(PatchParameterId pid) {return patchParameterValues[pid];}\n";
	myfile << "extern \"C\" { float *getPtrForParameter(PatchParameterId pid) { return &patchParameterValues[pid]; };\n";
	myfile << "std::string getNameForParameter(PatchParameterId pid) { return patchParameterNames[pid]; };\n};\n";
	myfile.close();
	
	
	
	string result = execute("g++ -arch i386 -c "+cpp+" " +includes+" -I/tmp/livecode -o /tmp/livecode/"+basename+".o");
	//	printf(">>>> %ld '%s'\n",result.size(), result.c_str());
	lastError = result;
	if(result.size()!=0) ofBackground(100, 0,0);
	else ofBackground(0);
	return result.size()==0;
}



void testApp::setFile(string file) {

	hppFile = ofFile(file);
	updateTime = getUpdateTime(hppFile);


	clean();
	compile();
	string dylibName = linkObjects();
	loadDylib("/tmp/livecode/"+dylibName);
}

void testApp::checkSourceForUpdates() {
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


//--------------------------------------------------------------
void testApp::update(){
	//	if(ofGetFrameNum()%60==0) {
	checkSourceForUpdates();
	//	}
}

string wrapLine(string inp, int w) {
	int charWidth = 8;
	string o = "";
	int numCharsPerLine = w/charWidth;
	
	for(int i = 0; i < inp.size(); i++) {
		o += inp[i];
		if(i>0 && i % numCharsPerLine==0) {
			o += "\n";
		}
	}
	return o;
}
string wrap(string inp, int w) {
	
	vector<string> lines = ofSplitString(inp, "\n");
	string out = "";
	for(int i = 0; i <lines.size(); i++) {
		out += wrapLine(lines[i], w) + "\n";
	}
	return out;
}
//--------------------------------------------------------------
void testApp::draw(){
	ofEnableAlphaBlending();
	glColor4f(1, 1, 1, 0.05);
	ofRect(0,0, GUI_WIDTH, ofGetHeight());
	
	
	// draw volumes
	ofRectangle r(GUI_WIDTH, ofGetHeight()-1, 25, -(ofGetHeight()-1));
	ofSetHexColor(0xFFFFFF);
	ofNoFill();
	ofRect(r);
	ofFill();
	ofSetHexColor(0xFF9944);
	ofRectangle rr = r;
	rr.height *= inLevelL;
	ofRect(rr);
	
	r.x += r.width;
	ofSetHexColor(0xFFFFFF);
	ofNoFill();
	ofRect(r);
	ofFill();
	ofSetHexColor(0xFF9944);
	rr = r;
	rr.height *= inLevelR;
	ofRect(rr);
	ofSetHexColor(0xFFFFFF);
	
	
	r.x += r.width;
	ofSetHexColor(0xFFFFFF);
	ofNoFill();
	ofRect(r);
	ofFill();
	ofSetHexColor(0x4499FF);
	rr = r;
	rr.height *= outLevelL;
	ofRect(rr);
	ofSetHexColor(0xFFFFFF);
	
	r.x += r.width;
	ofSetHexColor(0xFFFFFF);
	ofNoFill();
	ofRect(r);
	ofFill();
	ofSetHexColor(0x4499FF);
	rr = r;
	rr.height *= outLevelR;
	ofRect(rr);
	ofSetHexColor(0xFFFFFF);
	
	
	if(ofGetWidth()<1024 && lastError!="") {
		ofSetWindowShape(1024, ofGetHeight());
	} else if(lastError=="" && ofGetWidth()>GUI_WIDTH+100) {
		ofSetWindowShape(GUI_WIDTH+100, ofGetHeight());
	}
	if(lastError!="") {
		ofDrawBitmapString(wrap(lastError, ofGetWidth() - 20 - (GUI_WIDTH+100)), rr.x+rr.width, 20);
	}
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	if(key==' ') {
		active ^= true;
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
	
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
	
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
	
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
	
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
	if(dragInfo.files.size()>0) {
		ofFile f(dragInfo.files[0]);
		if(f.isDirectory()) {
			printf("Error: Must be a single file\n");
		} else {
			setFile(f.path());
		}
	}
}