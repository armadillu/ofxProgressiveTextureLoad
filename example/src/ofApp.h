#pragma once

#include "ofMain.h"
#include "ofxHistoryPlot.h"
#include "ofxProgressiveTextureLoad.h"

//just in case we overrode it in ofxProgressiveTextureLoad.h by disabling measurements

class ofApp : public ofBaseApp{
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

		void textureReady(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent& arg);
		void textureDrawable(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent& arg);

	ofTexture * myTex;
	ofxProgressiveTextureLoad progressiveTextureLoader;
	bool textureReadyToDraw;
	ofxHistoryPlot * plot;

};
