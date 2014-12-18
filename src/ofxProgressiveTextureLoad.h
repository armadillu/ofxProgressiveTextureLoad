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

//define OFX_PROG_TEX_LOADER_MEAURE_TIMINGS as FALSE to not measure timings with ofxTimeMeasurements
#define OFX_PROG_TEX_LOADER_MEAURE_TIMINGS	TRUE
#define DEBUG_TEX_LOADER_TIMES				FALSE /*requires timings and ofxTimeMeasurements*/
													/*very verbose timings if true*/

#if(OFX_PROG_TEX_LOADER_MEAURE_TIMINGS)
	#include "ofxTimeMeasurements.h"
#endif



class ofxProgressiveTextureLoad: public ofThread{

public:

	ofxProgressiveTextureLoad();
	~ofxProgressiveTextureLoad();

	void setup(ofTexture* tex, int resizeQuality = CV_INTER_CUBIC);
	void loadTexture(string path, bool withMipMaps);

	void update();

	//for each update() call (one frame), the addon will loop uploading texture regions
	//as scanlines, until we reach the target time per frame to be spent uploading texture data
	//how many scanlines per loop will determine the granularity of the time accuracy. Less scanlines
	//add more overhead, but should lead to more accurate stop times.
	void setScanlinesPerLoop(int numLines){numLinesPerLoop = numLines;}

	//how much time do you want ofxProgressiveTextureLoad to spend uploading data to the gpu per frame
	void setTargetTimePerFrame(float ms){maxTimeTakenPerFrame = ms;}

	//in mipmap levels. used to tweak which mipmap to use; helps make mipmaps sharper or blurrier.
	//0 is neutral; negative is lower mipmaps (sharper), positive is higher mipmaps (blurrier)
	void setTexLodBias(float bias){texLodBias = bias;}
	bool isReadyToDrawWhileLoading();

	float getTimeSpentLastFrame(){ return lastFrameTime;} //in ms!
	void setVerbose(bool v){verbose = v;}
	bool isBusy(){return state != IDLE;}
	bool isUploadingTextures(){return state == LOADING_TEX || state == LOADING_MIP_MAPS || state == ALLOC_TEXTURE; }
	
	string getStateString();

	void stopLoadingAsap(); //TODO! will get back to idle, tex remains as it is
							//(caller is responisble to clear the tex)

	void draw(int, int, bool debugImages = false); //for debug purposes!

	struct textureEvent{
		bool							ok; //used to be "loaded"
											//a load cancelled tex will return an "ok" in the event !

		bool							fullyLoaded;
		bool							readyToDraw;
		bool							canceledLoad;

		ofxProgressiveTextureLoad*		who;
		ofTexture*						tex;
		string							texturePath;
		float 							elapsedTime;

		textureEvent(){
			ok = true;
			canceledLoad = false;
			who = NULL;
		}
	};

	ofEvent<textureEvent>	textureReady; //will notify when texture is fully loaded, or failed to load
	ofEvent<textureEvent>	textureDrawable; //will notfy when texture is drawable,
											//it will   begind drawing in low res, and progressivelly improve!
	
	static int getNumInstancesCreated(){return numInstancesCreated;}
	static int getNumInstances(){return numInstances;}
	static float getNumMbLoaded(){return numMbLoaded;}

private:

	enum State{
				IDLE,
				LOADING_PIXELS,
				LOADING_FAILED,
				RESIZING_FOR_MIPMAPS,
				ALLOC_TEXTURE,
				LOADING_TEX,
				LOADING_MIP_MAPS
			};

	struct dataTypeConfig{
		GLuint glFormat;
		int numBytesPerPix;
		int opencvFormat;
	};


	State 				state;
	ofImage 			originalImage;
	dataTypeConfig		config;
	ofTexture			*texture;

	ofxMSATimer			timer;

	// speed params
	int 				numLinesPerLoop; //we can increase that to reduce overhead
	float 				maxTimeTakenPerFrame; //ms to spend loading tex data on a single frame
	float				texLodBias;

	int 				loadedScanLinesSoFar;

	bool 				verbose;

	string				imagePath;
	bool 				createMipMaps;
	bool 				pendingNotification;

	bool 				mipMapLevelLoaded;
	int					currentMipMapLevel;
	bool				mipMapLevelAllocPending;

	int 				resizeQuality;
	float 				startTime;
	float				lastFrameTime;

	bool				notifiedReadyToDraw;

	bool				cancelAsap;
	//float				cancelAsapDelay;

	bool 				isSetup;


	map<int, ofPixels*>	mipMapLevelPixels;

	void threadedFunction();
	void wrapUp();
	void resizeImageForMipMaps();
	bool progressiveTextureUpload(int mipmapLevel, uint64_t & currentTime);

	void setState(State newState){state = newState;}

	ofPoint getMipMap0ImageSize();

	int					ID;
	static int			numInstancesCreated;
	static int			numInstances;
	static float		numMbLoaded;


};


#endif /* defined(__BaseApp__ofxProgressiveTextureLoad__) */
