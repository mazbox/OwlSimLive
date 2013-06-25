#include "testApp.h"

#include <dlfcn.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "SimpleGui.h"
#include "Parameters.h"
xmlgui::SimpleGui gui;

ofMutex *mutex;
Patch *eff = NULL;
Patch *silent = NULL;
ofFile hppFile;
long updateTime;
class SilentEffect: public Patch {
public:
	
	virtual void processAudio(AudioInputBuffer &input, AudioOutputBuffer &output) {
		float* y = output.getSamples();
		int size = input.getSize();
		memset(y, 0, size*sizeof(float));
	}
};

	


void *livecodeLib = NULL;

void testApp::loadDylib(string file) {
	mutex->lock();
	if(livecodeLib!=NULL) {
		delete eff;
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
			vector<string> ctrlIds;
			ctrlIds.push_back("A");
			ctrlIds.push_back("B");
			ctrlIds.push_back("C");
			ctrlIds.push_back("D");
			ctrlIds.push_back("E");
			
			for(int i = 0; i < ctrlIds.size(); i++) {
				gui.getControlById(ctrlIds[i])->pointToValue(((float *(*)(int))paramPtrFunc)(i));
				gui.getControlById(ctrlIds[i])->setValue(i);
			}


					
			
			eff = ((Patch *(*)())ptrFunc)();
			
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
void testApp::doGui(Patch *e) {
	gui.clear();
	/*
	for(int i = 0; i < params.params.size(); i++) {
		Parameter *p = params.params[i];
		if(p->type=="slider") {
			SliderParameter *s = (SliderParameter*)p;
			gui.addSlider(s->name, *s->value, s->min, s->max);
		} else if(p->type=="intslider") {
			IntSliderParameter *s = (IntSliderParameter*)p;
			gui.addSlider(s->name, *s->value, s->min, s->max);
		} else if(p->type=="meter") {
			MeterParameter *s = (MeterParameter*)p;
			gui.addMeter(s->name, *s->value);
			
		} else if(p->type=="toggle") {
			ToggleParameter *t = (ToggleParameter*)p;
			gui.addToggle(t->name, *t->value);
		} else if(p->type=="switch") {
			SwitchParameter *s = (SwitchParameter*)p;
			gui.addSegmented(s->name, *s->value, s->options);
		}
	}*/
}




float dummyParams[5];
//--------------------------------------------------------------
void testApp::setup(){
	string r = ofToDataPath("", true);
	ofSetDataPathRoot(r.substr(0, r.size()-20)+"Resources/");
	printf("%s\n", ofToDataPath("", true).c_str());
	mutex = new ofMutex();
	silent = new SilentEffect();
	eff = silent;
	ofBackground(0);
	ofSetFrameRate(60);
	//loadDylib(ofToDataPath("../../Osc.dylib", true));
	
	gui.setEnabled(true);
	for(int i = 0; i < 5; i++) dummyParams[i] = 0;
	gui.addSlider("Parameter A", dummyParams[PARAMETER_A], 0, 1)->id = "A";
	gui.addSlider("Parameter B", dummyParams[PARAMETER_B], 0, 1)->id = "B";
	gui.addSlider("Parameter C", dummyParams[PARAMETER_C], 0, 1)->id = "C";
	gui.addSlider("Parameter D", dummyParams[PARAMETER_D], 0, 1)->id = "D";
	gui.addSlider("Parameter E", dummyParams[PARAMETER_E], 0, 1)->id = "E";
	//	ofSetWindowShape(195+50, 400);
	ofSetWindowShape(1024, 400);
	
	ofSoundStreamSetup(2, 2, this, 44100, 256
					   , 1);
}
float inBuff[8192];

float inLevelL = 0;
float inLevelR = 0;
int numInputChannels = 1;
int numOutputChannels = 1;
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

class InBuffer: public AudioInputBuffer {
public:
	float *samples;
	int size;
	InBuffer() {
		size = 0;
	}
	void getSamples(int from, int length, float* data) {
		memcpy(data, &samples[from], length * sizeof(float));
	}
	
	float* getSamples() {
		return samples;
	}
	int getSize() {
		return size;
	}
};

class OutBuffer: public AudioOutputBuffer {
public:
	OutBuffer() {
		size = 0;
	}
	
	int size;
	float *samples;
	
	void setSamples(int from, int length, float* data) {
		memcpy(&samples[from], data, length*sizeof(float));
	}
	
	void setSamples(float* data) {
		memcpy(samples, data, size*sizeof(float));
	}
	
	float* getSamples() {
		return samples;
	}
	
	int getSize() {
		return size;
	}
};

InBuffer ins;
OutBuffer outs;

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
	
	eff->processAudio(ins, outs);
	
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
	/*AEH h;
	ofstream myfile;
	myfile.open ("/tmp/livecode/AudioEffect.h");
	myfile << 	h.getHeader().c_str();
	myfile.close();*/
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
	string includes = "-I" + hppFile.getEnclosingDirectory() + " -I" + ofToDataPath("");
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
	ofRect(0,0, 195, ofGetHeight());
	
	
	// draw volumes
	ofRectangle r(195, ofGetHeight()-1, 25, -(ofGetHeight()-1));
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
	} else if(lastError=="" && ofGetWidth()>195+100) {
		ofSetWindowShape(195+100, ofGetHeight());
	}
	if(lastError!="") {
		ofDrawBitmapString(wrap(lastError, ofGetWidth() - 20 - (195+100)), rr.x+rr.width, 20);
	}
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	
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