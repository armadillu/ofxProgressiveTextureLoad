#pragma once

#include "ofMain.h"
#include "ofxHistoryPlot.h"
#include "ProgressiveTextureLoadQueue.h"

//just in case we overrode it in ofxProgressiveTextureLoad.h by disabling measurements

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

		void textureReady(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent& arg);
		void textureDrawable(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent& arg);

		vector<ofTexture *>				textures;
		vector<ofxProgressiveTextureLoad*>	loaders;
		map<ofTexture *, bool>				ready;
		vector<string>					imageNames;

	void threadedFunction();
};
