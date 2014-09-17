#pragma once

#include "ofMain.h"
#include "ofxTimeMeasurements.h"
#include "ofxHistoryPlot.h"
#include "ofxRemoteUIServer.h"
#include "ofxProgressiveTextureLoad.h"

#include "ofxMSATimer.h"

class testApp : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);


	ofTexture * myTex;
	ofxProgressiveTextureLoad progressiveTextureLoader;

	ofxHistoryPlot * plot;

	void threadedFunction();
};
