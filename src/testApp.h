#pragma once

#include "ofMain.h"
#include "StompBox.h"
#include "AudioBufferImpl.h"
#include "SimpleGui.h"
#include "PatchCompiler.h"
#include "ofxFft.h"

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
	
	ofxFft *fft;
	void loadDylib(string file);
	void drawFft();

	
	bool compile();
	

	AudioBufferImpl buff;
	xmlgui::SimpleGui gui;
	ofImage bgImage;
	

	
	PatchCompiler patchCompiler;
	
	void drawLevels();
	int bufferSize;
	int buffersPerFftBlock;
	int fftSize;
	int fftInputPos;
	float samplerate;
	float *fftInput;
	float *fftAvg;
	
	float inBuff[8192];
	int GUI_WIDTH;
	int LEVELS_WIDTH;
	int FFT_WIDTH;
	float getXForBin(float bin, float numBins);
};
