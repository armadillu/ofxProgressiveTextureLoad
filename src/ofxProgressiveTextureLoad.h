//
//  ofxProgressiveTextureLoad.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesiˆ on 16/09/14.
//
//

#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"

//define OFX_PROG_TEX_LOADER_MEAURE_TIMINGS in your preProcessor macros to measure timings with ofxTimeMeasurements

#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
	#include "ofxTimeMeasurements.h"
#endif

class SimpleThread{
public:

	SimpleThread(): state(IDLE){}

	bool startThread(){
		if(state == IDLE || state == DONE){
			state = RUNNING;
			thread = std::thread(&SimpleThread::runThread, this);
			return true;
		}else{
			ofLogError("SimpleThread") << "can't start thread as its already running!";
			return false;
		}
	}

	void waitForThread( long milliseconds = -1) {
		std::unique_lock<std::mutex> lck(mutex);
		if (state == RUNNING) {
			if (-1 == milliseconds) { //infinite wait
				condition.wait(lck);
			} else {
				if (condition.wait_for(lck, std::chrono::milliseconds(milliseconds)) == std::cv_status::timeout) {
					ofLogWarning("SimpleThread") << "Timed out waiting for thread!";
				}
			}
		}
	}

	virtual void threadedFunction() = 0; //you want your subclass to implement this method

	bool isRunning(){return state == RUNNING;}
	bool isDone(){return state == DONE;}
	std::thread& getThread(){return thread;}

protected:

	enum State{ IDLE, RUNNING, DONE };

	void runThread(){
		try{
			threadedFunction();
		}catch(...){}
		state = DONE;
		condition.notify_all();
	}

	std::thread thread;
	std::atomic<State> state;

	std::condition_variable condition;
	std::mutex mutex;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////


class ofxProgressiveTextureLoad : public SimpleThread{

	friend class ProgressiveTextureLoadQueue;
	
public:

	ofxProgressiveTextureLoad();
	~ofxProgressiveTextureLoad();

	//if using ARB Textures, no resize is necessary!
	//you cant create mipmaps if using ARB!
	void setup(ofTexture* tex, int resizeQuality = CV_INTER_CUBIC, bool useARB = false);
	void loadTexture(std::string path, bool withMipMaps);
	int getID(){return ID;}

	void update();

	//for each update() call (one frame), the addon will loop uploading texture regions
	//as scanlines, until we reach the target time per frame to be spent uploading texture data
	//how many scanlines per loop will determine the granularity of the time accuracy. Less scanlines
	//add more overhead, but should lead to more accurate stop times.
	void setScanlinesPerLoop(int numLines){numLinesPerLoop = numLines;}

	//how much time do you want ofxProgressiveTextureLoad to spend uploading data to the GPU per frame
	void setTargetTimePerFrame(float ms){maxTimeTakenPerFrame = ms;}

	//in mipmap levels. used to tweak which mipmap to use; helps make mipmaps sharper or blurrier.
	//0 is neutral; negative is lower mipmaps (sharper), positive is higher mipmaps (blurrier)
	void setTexLodBias(float bias){texLodBias = bias;}
	bool isReadyToDrawWhileLoading();

	bool hasBeenAskedToCancelLoad(){ return cancelAsap; }

	float getTimeSpentLastFrame(){ return lastFrameTime && isDone();} //in ms!
	void setVerbose(bool v){verbose = v;}
	bool canBeDeleted(){return readyForDeletion;}
	bool isBusy(){return !readyForDeletion || state == LOADING_PIXELS;}
	bool isUploadingTextures(){return state == LOADING_TEX || state == LOADING_MIP_MAPS || state == ALLOC_TEXTURE; }
	
	std::string getStateString();
	bool isDoingWorkInThread();

	void stopLoadingAsap(); //TODO! will get back to idle, tex remains as it is
							//(caller is responsible to clear the tex)

	void draw(int, int, bool debugImages = false); //for debug purposes!

	struct ProgressiveTextureLoadEvent{
		bool							ok; //used to be "loaded"
											//a load canceled tex will return an "ok" in the event !

		bool							fullyLoaded;
		bool							readyToDraw;
		bool							canceledLoad;

		ofxProgressiveTextureLoad*		who;
		ofTexture*						tex;
		std::string							texturePath;
		float 							elapsedTime;

		ProgressiveTextureLoadEvent(){
			ok = true;
			canceledLoad = false;
			who = NULL;
		}
	};

	ofEvent<ProgressiveTextureLoadEvent>	textureReady; //will notify when texture is fully loaded, or failed to load
	ofEvent<ProgressiveTextureLoadEvent>	textureDrawable; //will notfy when texture is drawable,
											//it will begin drawing in low res, and progressivelly improve!
	
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
	ofPixels 			imagePixels;
	dataTypeConfig		config;
	ofTexture			*texture;

	uint64_t			timer; //TODO drop this in OF 0.9, timings will be more accurate in windows

	// speed params
	int 				numLinesPerLoop; //we can increase that to reduce overhead
	float 				maxTimeTakenPerFrame; //ms to spend loading tex data on a single frame
	float				texLodBias;

	int 				loadedScanLinesSoFar;

	bool 				verbose;

	std::string				imagePath;
	bool 				createMipMaps;
	bool				useARB;
	bool 				pendingNotification;

	bool 				mipMapLevelLoaded;
	int					currentMipMapLevel;
	bool				mipMapLevelAllocPending;

	int 				resizeQuality;
	float 				startTime;
	float 				totalLoadTime;
	float				lastFrameTime;

	bool				notifiedReadyToDraw;

	bool				cancelAsap;
	//float				cancelAsapDelay;

	bool 				isSetup;


	std::map<int, ofPixels*>	mipMapLevelPixels;

	void threadedFunction();
	void wrapUp();
	bool resizeImageForMipMaps();
	bool progressiveTextureUpload(int mipmapLevel, uint64_t & currentTime);

	void setState(State newState);

	ofPoint getMipMapImageSize(int mipmapLevel);

	int					ID;
	static int			numInstancesCreated;
	static int			numInstances;
	static float		numMbLoaded;
	bool				readyForDeletion;

	bool				useOnlyOnce;

	//to be called only by ProgressiveTextureLoadQueue
	void setWillBeUsedOnlyOnce(bool b){ useOnlyOnce = b; };


};

