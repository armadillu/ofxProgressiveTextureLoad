//
//  ofxProgressiveTextureLoad.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 16/09/14.
//
//

#ifndef __BaseApp__ofxProgressiveTextureLoad__
#define __BaseApp__ofxProgressiveTextureLoad__

#include "ofMain.h"
#include "ofxMSATimer.h"


class ofxProgressiveTextureLoad: public ofThread{

public:

	ofxProgressiveTextureLoad();
	void setup(ofTexture* tex);

	void loadTexture(string path, bool withMipMaps);
	void update();

	void draw();

	struct ofxProgressiveTextureLoadEvent{
		bool							loaded;
		ofxProgressiveTextureLoadEvent *	who;
		ofTexture						tex;
		string							texturePath;

		ofxProgressiveTextureLoadEvent(){
			loaded = true;
			who = NULL;
		}
	};

	bool isBusy(){return state != IDLE;}


	ofEvent<ofxProgressiveTextureLoadEvent>	textureLoadFailed;
	ofEvent<ofxProgressiveTextureLoadEvent>	textureReady;

private:

	enum State{
				IDLE,
				LOADING_PIXELS,
				RESIZING_FOR_MIPMAPS,
				ALLOC_TEXTURE,
				LOADING_TEX,
				LOADING_MIP_MAPS
			};

	State 				state;
	ofImage 			originalImage;
	ofTexture			*texture;

	ofxMSATimer			timer;

	// speed params
	int numLinesPerFrame; //we can increase that to reduce overhead
	float maxTimeTakenPerFrame; //ms to spend loading tex data on a single frame
	int loadedScanLinesSoFar;


	string				imagePath;
	bool 				createMipMaps;
	bool 				pendingNotification;

	bool 				mipMapLevelLoaded;
	int					currentMipMapLevel;
	bool				mipMapLevelAllocPending;

	map<int, ofPixels*>	mipmapsPixels;

	void threadedFunction();
	void resizeImageForMipMaps();
	void progressiveTextureUpload(int mipmapLevel);

	void setState(State newState){state = newState;}


};


#endif /* defined(__BaseApp__ofxProgressiveTextureLoad__) */
