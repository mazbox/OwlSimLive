#pragma once

#include "ofMain.h"
#include "StompBox.h"
#include "AudioBufferImpl.h"
#include "SimpleGui.h"
#include "PatchCompiler.h"

class testApp : public ofBaseApp {
	
public:
	void setup();
	void update();
	void draw();
	
	void keyPressed  (int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	void audioIn( float * input, int bufferSize, int nChannels );
	void audioOut( float * output, int bufferSize, int nChannels );
	
private:

	void loadDylib(string file);

	
	bool compile();
	

	AudioBufferImpl buff;
	xmlgui::SimpleGui gui;
	ofImage bgImage;
	

	
	PatchCompiler patchCompiler;
	

	
	float inBuff[8192];
	int GUI_WIDTH;
};
