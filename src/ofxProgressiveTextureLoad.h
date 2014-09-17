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
#include "ofxOpenCv.h"


class ofxProgressiveTextureLoad: public ofThread{

public:

	ofxProgressiveTextureLoad();
	void setup(ofTexture* tex, int resizeQuality = CV_INTER_CUBIC);

	void loadTexture(string path, bool withMipMaps);
	void update();

	void draw(int, int);

	struct textureEvent{
		bool							loaded;
		ofxProgressiveTextureLoad*		who;
		ofTexture*						tex;
		string							texturePath;

		textureEvent(){
			loaded = true;
			who = NULL;
		}
	};

	bool isBusy(){return state != IDLE;}

	ofEvent<textureEvent>	textureLoadFailed;
	ofEvent<textureEvent>	textureReady;

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
	int 				numLinesPerFrame; //we can increase that to reduce overhead
	float 				maxTimeTakenPerFrame; //ms to spend loading tex data on a single frame
	int 				loadedScanLinesSoFar;


	string				imagePath;
	bool 				createMipMaps;
	bool 				pendingNotification;

	bool 				mipMapLevelLoaded;
	int					currentMipMapLevel;
	bool				mipMapLevelAllocPending;

	int 				resizeQuality;

	map<int, ofPixels*>	mipMapLevelPixels;

	void threadedFunction();
	void resizeImageForMipMaps();
	void progressiveTextureUpload(int mipmapLevel);

	void setState(State newState){state = newState;}

	ofPoint getMipMap0ImageSize();

};


#endif /* defined(__BaseApp__ofxProgressiveTextureLoad__) */
