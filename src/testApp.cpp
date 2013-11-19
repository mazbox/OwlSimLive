#include "testApp.h"



#include "Parameters.h"
#include "MutePatch.h"






bool active = true;


float inLevelL = 0;
float inLevelR = 0;
int numInputChannels = 1;
int numOutputChannels = 1;













void styleKnob(Knob *k, int pos) {
	k->width = 100;
	k->height = k->width;
	k->needleImage = xmlgui::Resources::getImage(ofToDataPath("knobNeedle.png"));
	k->needleImage->setAnchorPercent(0.5, 0.5);

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
	

	// set data path to app/Contents/Resources
	string r = ofToDataPath("", true);
	string dataPathRoot = r;//r.substr(0, r.size()-20)+"Resources/data/";
	ofSetDataPathRoot(dataPathRoot);
	ofLogNotice() << "Setting data path root to " << dataPathRoot;
	
	bgImage.loadImage("bg.png");
	GUI_WIDTH = bgImage.getWidth();
	
	patchCompiler.setup(&gui);
	
	
	ofBackground(0);
	ofSetFrameRate(60);
	
	gui.setEnabled(true);
	gui.setAutoLayout(false);
	
	for(int i = 0; i < patchCompiler.ctrlIds.size(); i++) {
		patchCompiler.dummyParams[i] = 0;
		Knob *knob = gui.addKnob("Parameter "+patchCompiler.ctrlIds[i], patchCompiler.dummyParams[i]);
		knob->id = patchCompiler.ctrlIds[i];
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
			if(inBuff[i]>1) inBuff[i] = 1;
			if(inBuff[i]<-1) inBuff[i] = -1;
		}
	}
	
	for(int i = 0; i < bufferSize*nChannels; i++) {
		if(inBuff[i]>1) inBuff[i] = 1;
		if(inBuff[i]<-1) inBuff[i] = -1;
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
	patchCompiler.lock();
	
	buff.size = bufferSize;
	buff.samples = inBuff;
	
	buff.size = bufferSize;
	
	
	if(active) {
		patchCompiler.patch->processAudio(buff);
	} 
	
	
	
	if(numOutputChannels==1) {
		for(int i = 0; i < bufferSize; i++) {
			output[i*2] = output[i*2+1] = buff.samples[i];
		}
	}
	
	
	
	
	
	patchCompiler.unlock();
	
	for(int i = 0; i < nChannels*bufferSize; i++) {
		if(output[i]>1) output[i] = 1;
		else if(output[i] <-1) output[i] = -1;

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




//--------------------------------------------------------------
void testApp::update(){
	//	if(ofGetFrameNum()%60==0) {
	patchCompiler.checkSourceForUpdates();
	//	}
}

string wrapLine(string inp, int w) {
	int charWidth = 6;
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
	ofSetColor(255);
	bgImage.draw(0, 0);
	
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
	
	// TODO: don't ask for the entire error string to test if there's an error
	// just ask for something like patchCompiler.hasError()
	if(ofGetWidth()<1024 && patchCompiler.getLastError()!="") {
		ofSetWindowShape(1024, ofGetHeight());
	} else if(patchCompiler.getLastError()=="" && ofGetWidth()>GUI_WIDTH+100) {
		ofSetWindowShape(GUI_WIDTH+100, ofGetHeight());
	}
	if(patchCompiler.getLastError()!="") {
		xmlgui::Resources::drawString(wrap(patchCompiler.getLastError(), ofGetWidth() - 20 - (GUI_WIDTH+100)), rr.x+rr.width, 20);
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
			patchCompiler.setFile(f.path());
		}
	}
}