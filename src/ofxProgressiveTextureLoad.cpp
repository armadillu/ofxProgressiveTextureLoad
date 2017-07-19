//
//  ofxProgressiveTextureLoad.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesiˆ on 16/09/14.
//
//

#include "ofxProgressiveTextureLoad.h"
#include <math.h>
#include "opencv2/imgproc/imgproc.hpp"


int ofxProgressiveTextureLoad::numInstancesCreated = 0;
int ofxProgressiveTextureLoad::numInstances = 0;
float ofxProgressiveTextureLoad::numMbLoaded = 0;

ofxProgressiveTextureLoad::ofxProgressiveTextureLoad(){

	numLinesPerLoop = 64;
	maxTimeTakenPerFrame = 6.0; //ms
	currentMipMapLevel = 0;
	state = IDLE;
	verbose = false;
	lastFrameTime = 0.0f;
	texLodBias = -0.5;
	texture = NULL;
	ID = numInstancesCreated;
	numInstancesCreated++;
	numInstances++;
	isSetup = false;
	cancelAsap = false;
	readyForDeletion = false;
	useOnlyOnce = false;
}

ofxProgressiveTextureLoad::~ofxProgressiveTextureLoad(){
	//delete all mipmap pixels
	//cout << "begin delete ofxProgressiveTextureLoad " << this << endl;
	for(size_t i = 0; i < mipMapLevelPixels.size(); i++){
		if(mipMapLevelPixels[i] != &imagePixels){ //in some cases where no resize needed we dont actually allocate pix, we only reference local pixels
			delete mipMapLevelPixels[i];
		}
	}
	mipMapLevelPixels.clear();
	numInstances--;
	//cout << "end delete ofxProgressiveTextureLoad " << this << endl;
}

void ofxProgressiveTextureLoad::setup(ofTexture* tex, int resizeQuality_, bool useARB_){

	resizeQuality = resizeQuality_;
	texture = tex;
	isSetup = true;
	useARB = useARB_;
}

void ofxProgressiveTextureLoad::loadTexture(string path, bool withMipMaps){

	if (state == IDLE && texture){

		if (useARB && withMipMaps){
			ofLogError("ofxProgressiveTextureLoad") << "You can't create mipmaps on ARB textures. If you want mipmaps, call setup() the object with \"useARB = false\"";
			ofLogError("ofxProgressiveTextureLoad") << "Will load the texture without mipmaps";
			withMipMaps = false;
		}

		startTime = ofGetElapsedTimef();
		createMipMaps = withMipMaps;
		#ifdef TARGET_OPENGLES
		createMipMaps = false;
		#endif
		pendingNotification = false;
		loadedScanLinesSoFar = 0;
		imagePath = path;
		setState(LOADING_PIXELS);
		startThread(true);
		//ofAddListener(ofEvents().update, this, &ofxProgressiveTextureLoad::update);
		#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
		TS_START_NIF("total tex load time " + ofToString(ID));
		#endif
	}else{
		ofLogError("ofxProgressiveTextureLoad") << "cant load texture, busy!";
	}
}


void ofxProgressiveTextureLoad::threadedFunction(){


	#ifndef TARGET_WIN32
	pthread_setname_np("ofxProgressiveTextureLoad");
	#endif

	float startTime = ofGetElapsedTimef();

	while(true){
		if(cancelAsap){
			setState(IDLE);
			pendingNotification = true;
			return;
		}
		switch (state) {

			case LOADING_PIXELS:{
				#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
				TS_START_NIF("loadPix " + ofToString(ID));
				#endif
				try{
					bool ok = ofLoadImage(imagePixels, imagePath);
					if (!ok){
						ofLogError("ofxProgressiveTextureLoad") << "img loading failed! " << imagePath;
						setState(LOADING_FAILED);
						return;
					}else{
						if(cancelAsap){
							setState(IDLE);
							pendingNotification = true;
							return;
						}
						#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
						TS_STOP_NIF("loadPix " + ofToString(ID));
						#endif
						switch (imagePixels.getImageType()) {
							case OF_IMAGE_COLOR:
								config.glFormat = GL_RGB;
								config.numBytesPerPix = 3;
								config.opencvFormat = CV_8UC3;
								break;
							case OF_IMAGE_COLOR_ALPHA:
								config.glFormat = GL_RGBA;
								config.numBytesPerPix = 4;
								config.opencvFormat = CV_8UC4;
								break;
							case OF_IMAGE_GRAYSCALE:
								config.glFormat = GL_LUMINANCE;
								config.numBytesPerPix = 1;
								config.opencvFormat = CV_8UC1;
								break;
						}
						if(useARB){ //no mipmaps either way
							mipMapLevelPixels[0] = &imagePixels;
							setState(ALLOC_TEXTURE);
							return;
						}else{
							if (createMipMaps){
								setState(RESIZING_FOR_MIPMAPS);
							}else{
								mipMapLevelPixels[0] = &imagePixels;
								setState(ALLOC_TEXTURE);
								return;
							}
						}
					}
				}catch(...){
					#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
					TS_STOP_NIF("loadPix " + ofToString(ID));
					#endif
					ofLogError("ofxProgressiveTextureLoad") << "exception in threadedFunction()";
					setState(LOADING_FAILED);
					return;
				}
			}break;

			case RESIZING_FOR_MIPMAPS:
				#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
				TS_START_NIF("resizeImageForMipMaps " + ofToString(ID));
				#endif
				try{
					bool ok = resizeImageForMipMaps();
					if (!ok){
						setState(IDLE);
						pendingNotification = true;
						return;
					}
				}catch(...){
					ofLogError("ofxProgressiveTextureLoad") << "img resizing failed! " << imagePath;
					setState(LOADING_FAILED); //mm TODO!
					return;
				}
				#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
				TS_STOP_NIF("resizeImageForMipMaps " + ofToString(ID));
				#endif
				 //working around the of thread issue where very short lived threads cause expcetions sometimes
				setState(ALLOC_TEXTURE);
				return;
			default:
				ofLogError("ofxProgressiveTextureLoad") << "how are we here?";
				break;
		}
	}
}

bool ofxProgressiveTextureLoad::isDoingWorkInThread(){
	return state == LOADING_PIXELS || state == RESIZING_FOR_MIPMAPS;

}

bool ofxProgressiveTextureLoad::resizeImageForMipMaps(){

	int numC = config.numBytesPerPix;
	ofPoint targetSize = ofVec2f(imagePixels.getWidth(), imagePixels.getHeight());
	int newW = targetSize.x;
	int newH = targetSize.y;
	int largestSide = MAX((newW), (newH));
	int mipMapLevel = floor(log( largestSide ) / log( 2 )) + 1; //THIS IS KEY! you need to do all mipmap levels or it will draw blank tex!

	if(cancelAsap){
		return false;
	}

	//fill in an opencv image
	cv::Mat mipMap0(imagePixels.getHeight(), imagePixels.getWidth(), config.opencvFormat);
	memcpy( mipMap0.data,  imagePixels.getData(), imagePixels.getWidth() * imagePixels.getHeight() * numC);

	if(cancelAsap){
		return false;
	}

	ofPixels *pix = new ofPixels();
	pix->setFromPixels(mipMap0.data, newW, newH, numC);
	mipMapLevelPixels[0] = pix;
	//ofSaveImage(*pix, "pix" + ofToString(0) + ".jpg" ); //debug!

	for(int currentMipMapLevel = 1 ; currentMipMapLevel < mipMapLevel; currentMipMapLevel++){

		if(cancelAsap){
			return false;
		}

		ofPoint newSize = getMipMapImageSize(currentMipMapLevel);

		//TS_START_NIF("resize mipmap " + ofToString(currentMipMapLevel));
		int www = newSize.x; //mipMap0.cols/2;
		int hhh = newSize.y; //mipMap0.rows/2;
		if (www < 1) www = 1; if (hhh < 1) hhh = 1;
		cv::resize(mipMap0, mipMap0, cv::Size(www, hhh), 0, 0, resizeQuality);
		ofPixels * tmpPix;
		tmpPix = new ofPixels();
		tmpPix->setFromPixels(mipMap0.data, mipMap0.cols, mipMap0.rows, numC);
		mipMapLevelPixels[currentMipMapLevel] = tmpPix;
		//ofLog() << "mipmaps for level " << currentMipMapLevel << " ready (" << tmpPix->getWidth() << ", " << tmpPix->getWidth()<< ")";
		//TS_STOP_NIF("resize mipmap " + ofToString(currentMipMapLevel));
		//ofSaveImage(*tmpPix, "pix" + ofToString(currentMipMapLevel) + ".jpg" ); //debug!
	}
	return true;
}


ofPoint ofxProgressiveTextureLoad::getMipMapImageSize(int mipmaplevel){
	int divisionFactor = (mipmaplevel == 0) ? 1 : (pow(2, mipmaplevel));
	int mipmapTexSizeX = (mipMapLevelPixels[0]->getWidth()) / divisionFactor;
	int mipmapTexSizeY = (mipMapLevelPixels[0]->getHeight()) / divisionFactor;
	if(mipmapTexSizeX < 1) mipmapTexSizeX = 1;
	if(mipmapTexSizeY < 1) mipmapTexSizeY = 1;
	return ofPoint(mipmapTexSizeX, mipmapTexSizeY);
}


bool ofxProgressiveTextureLoad::isReadyToDrawWhileLoading(){
	if(isBusy() && createMipMaps){
		return (state == LOADING_MIP_MAPS && currentMipMapLevel < mipMapLevelPixels.size() - 3 );
	}
	return false;
}

void ofxProgressiveTextureLoad::setState(State newState){
	state = newState;
	if(state == LOADING_FAILED){
		pendingNotification = true;
	}
}

void ofxProgressiveTextureLoad::update(){

	if(readyForDeletion && useOnlyOnce) return;
	bool willBeReadyForDeletion = false;

	#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
	TS_START_ACC("ProgTexLoad u");
	#endif

	switch (state) {

		case IDLE:{
			if (cancelAsap){
				wrapUp();
			}
		}break;

		case ALLOC_TEXTURE:{
			texture->clear();
			int newW = imagePixels.getWidth();
			int newH = imagePixels.getHeight();
			texture->allocate(newW, newH,
							  ofGetGlInternalFormat(imagePixels),
							  useARB, //arb! no arb when creating mipmaps
							  ofGetGlFormat(imagePixels),
							  ofGetGlType(imagePixels)
							  );

			notifiedReadyToDraw = false;
			//setup OF texture sizes!
			texture->texData.width = imagePixels.getWidth();
			texture->texData.height = imagePixels.getHeight();
			#ifndef TARGET_OPENGLES
			if(texture->texData.textureTarget == GL_TEXTURE_RECTANGLE_ARB){
				texture->texData.tex_t = imagePixels.getWidth();
				texture->texData.tex_u = imagePixels.getHeight();
			}else{
				if(createMipMaps){
					texture->texData.tex_t = 1;
					texture->texData.tex_u = 1;
				}else{
					texture->texData.tex_t = imagePixels.getWidth() / (float)ofNextPow2(imagePixels.getWidth());
					texture->texData.tex_u = imagePixels.getHeight() / (float)ofNextPow2(imagePixels.getHeight());
				}
			}
			#else
			texture->texData.tex_t = imagePixels.getWidth() / (float)ofNextPow2(imagePixels.getWidth());
			texture->texData.tex_u = imagePixels.getHeight() / (float)ofNextPow2(imagePixels.getHeight());
			#endif
			texture->texData.tex_w = imagePixels.getWidth();
			texture->texData.tex_h = imagePixels.getHeight();

			if(createMipMaps){
				imagePixels.clear(); //dealloc original image, we have all the ofPixels in a map!
			}
			mipMapLevelLoaded = false;
			if(createMipMaps){
				currentMipMapLevel = mipMapLevelPixels.size() - 1; //start by loading the smallest image (deepest mipmap)
				setState(LOADING_MIP_MAPS);
				mipMapLevelAllocPending = true; //otherwise we dont alloc space for the 1st mipmap level, and we get GL_INVALID_OPERATION
				#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
				TS_START_NIF("upload mipmaps " + ofToString(ID));
				#endif
			}else{
				currentMipMapLevel = 0;
				setState(LOADING_TEX);
			}
		}break;

		case LOADING_TEX:{
			if (cancelAsap){
				setState(IDLE);
				wrapUp();
			}else{
				uint64_t currentTime = 0;
				progressiveTextureUpload(currentMipMapLevel, currentTime);
				if (mipMapLevelLoaded){
					imagePixels.clear(); //dealloc original image pixels, we finished loading!
					mipMapLevelPixels.clear();
					texture->setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
					wrapUp();
				}
			}
		}break;

		case LOADING_MIP_MAPS:{
			if (cancelAsap){
				setState(IDLE);
				wrapUp();
			}else{
				bool wrappingUp = false;
				bool canGoOn = true;
				uint64_t timeSpentSoFar = 0;
				while(canGoOn && !wrappingUp){
					if (mipMapLevelLoaded){
						#ifndef TARGET_OPENGLES
						texture->bind();
						glTexParameteri(texture->texData.textureTarget, GL_TEXTURE_BASE_LEVEL, currentMipMapLevel);
						glTexParameteri(texture->texData.textureTarget, GL_TEXTURE_MAX_LEVEL, mipMapLevelPixels.size() - 1  );
						#endif
						int mipmapsLoaded = mipMapLevelPixels.size() - currentMipMapLevel;
						if (!notifiedReadyToDraw && mipmapsLoaded >= 3 ){ //notify the user that the texture is drawable right now, and will progressively draw
							notifiedReadyToDraw = true;
							ProgressiveTextureLoadEvent ev;
							ev.ok = true;
							ev.fullyLoaded = false;
							ev.readyToDraw = true;
							ev.who = this;
							ev.tex = texture;
							ev.elapsedTime = ofGetElapsedTimef() - startTime;
							ev.texturePath = imagePath;
							ofNotifyEvent(textureDrawable, ev, this);
							#ifndef TARGET_OPENGLES
							glTexParameteri(texture->texData.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameteri(texture->texData.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
							#endif
							if(verbose) ofLogNotice("ofxProgressiveTextureLoad") << "Texture Ready To Draw! Time So far: " << ofGetElapsedTimef() - startTime << "sec";
						}
						#ifndef TARGET_OPENGLES
						if (mipmapsLoaded >= 4){
							glTexParameterf(texture->texData.textureTarget, GL_TEXTURE_LOD_BIAS, texLodBias);
						}

						texture->unbind();
						#endif

						mipMapLevelLoaded = false;
						mipMapLevelAllocPending = true;
						if(verbose) ofLogNotice("ofxProgressiveTextureLoad") << "Mipmap Level " << currentMipMapLevel << " fully Loaded!!";

						if (currentMipMapLevel == 0){ //all mipmaps loaded! done!
							wrappingUp = true;
							wrapUp();
							#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
							TS_STOP_NIF("upload mipmaps " + ofToString(ID));
							#endif
							setState(IDLE);
						}else{
							currentMipMapLevel--;
						}

					}
					if(!wrappingUp){
						canGoOn = progressiveTextureUpload(currentMipMapLevel, timeSpentSoFar);
					}
				}
			}
		}break;
	}

	if(pendingNotification){

		pendingNotification = false;
		ProgressiveTextureLoadEvent ev;
		if (state == LOADING_FAILED || cancelAsap){
			if(state == LOADING_FAILED)
				ev.ok = false;
			else
				ev.ok = cancelAsap;
			ev.fullyLoaded = false;
			ev.readyToDraw = false;
		}else{
			ev.ok = true;
			ev.fullyLoaded = true;
			ev.readyToDraw = true;
		}
		ev.canceledLoad = cancelAsap;
		ev.who = this;
		ev.tex = texture;
		if(ev.fullyLoaded && ev.ok){
			float thisTex = ev.tex->getWidth() * ev.tex->getHeight() * config.numBytesPerPix / float(1024 * 1024);
			if(createMipMaps) thisTex *= 1.33f;
			numMbLoaded += thisTex;
		}
		ev.elapsedTime = ofGetElapsedTimef() - startTime;
		ev.texturePath = imagePath;
		ofNotifyEvent(textureReady, ev, this);
		#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
		TS_STOP_NIF("total tex load time " + ofToString(ID));
		#endif
		setState(IDLE);
		willBeReadyForDeletion = true;
	}
	#ifdef OFX_PROG_TEX_LOADER_MEAURE_TIMINGS
	TS_STOP_ACC("ProgTexLoad u");
	#endif
	if(willBeReadyForDeletion) readyForDeletion = true;
}

void ofxProgressiveTextureLoad::wrapUp(){
	pendingNotification = true;
	totalLoadTime = ofGetElapsedTimef() - startTime;
	if(verbose) ofLogNotice("ofxProgressiveTextureLoad") << "Total Time: " << totalLoadTime << "sec";
}


bool ofxProgressiveTextureLoad::progressiveTextureUpload(int mipmapLevel, uint64_t & currentTime){

	GLuint glFormat = config.glFormat;
	GLuint glPixelType = GL_UNSIGNED_BYTE;

	glEnable(texture->texData.textureTarget);
	glBindTexture(texture->texData.textureTarget, texture->texData.textureID);
	//texture->bind();
	int numC = ofGetNumChannelsFromGLFormat(glFormat);

	ofPixels * pix = mipMapLevelPixels[mipmapLevel];
	int scanlinesLoadedThisFrame = 0;
	int loops = 0;

	ofSetPixelStoreiAlignment(GL_UNPACK_ALIGNMENT, pix->getBytesStride());

	while (currentTime < maxTimeTakenPerFrame * 1000.0f && loadedScanLinesSoFar < pix->getHeight()) {


		uint64_t time = ofGetElapsedTimeMicros();
		unsigned char * data = pix->getData() + numC * (int)pix->getWidth() * loadedScanLinesSoFar;

		int numLinesToLoadThisLoop = pix->getHeight() - loadedScanLinesSoFar;
		if (numLinesToLoadThisLoop > numLinesPerLoop){
			numLinesToLoadThisLoop = numLinesPerLoop;
		}

		if (mipMapLevelAllocPending && mipmapLevel != 0) { //level 0 mipmap is already allocated!

			//if (verbose) ofLogNotice() << "loop  " << loops;
			mipMapLevelAllocPending = false;
			ofPoint mipmapSize = getMipMapImageSize(mipmapLevel);

			if (verbose) ofLogError("ofxProgressiveTextureLoad") << "tex " << texture->texData.textureTarget << " allocating mipmap level " << mipmapLevel << " [" << pix->getWidth() << "x" << numLinesToLoadThisLoop << "]";
			glTexImage2D(texture->texData.textureTarget,	//target
							mipmapLevel,						//mipmap level
							#if OF_VERSION_MINOR < 9
							texture->texData.glTypeInternal,	//internal format
							#else
							texture->texData.glInternalFormat,	//internal format
							#endif
							mipmapSize.x,					//w
							mipmapSize.y,					//h
							0,									//border
							glFormat,							//format
							glPixelType,						//type
							0 );								//pixels


		}

		#ifdef TARGET_WIN32
		if (numLinesToLoadThisLoop > 1) { //This is a workaround for WINDOWS ATI DRIVERS crashing when uploading 1x1 tex data!?
		#else
		{
		#endif
			if (verbose) ofLogWarning("ofxProgressiveTextureLoad") << "tex " << texture->texData.textureTarget << " loading mipmap level " << mipmapLevel << " [" << pix->getWidth() << "x" << numLinesToLoadThisLoop << "]";
				glTexSubImage2D(texture->texData.textureTarget,	//target
								mipmapLevel,					//mipmap level
								0,								//x offset
								loadedScanLinesSoFar,			//y offset
								pix->getWidth(),				//width
								numLinesToLoadThisLoop,			//height >> numLinesPerLoop line per iteration
								glFormat,						//format
								glPixelType,					//type
								data							//pixels
								);
			}
			loadedScanLinesSoFar += numLinesToLoadThisLoop;
			scanlinesLoadedThisFrame += numLinesToLoadThisLoop;
			uint64_t thisTime = ofGetElapsedTimeMicros() - time;
			currentTime += thisTime;
			if(verbose) ofLogNotice("ofxProgressiveTextureLoad") << "loop " << loops << " loaded " << numLinesToLoadThisLoop << " lines and took " << thisTime / 1000.0f << " ms";
			loops++;
		}
		//if we finished this mipmap but there's time left, we could go on...
		bool couldGoOn = false;
		if(currentTime < maxTimeTakenPerFrame * 1000.0f){
			couldGoOn = true;
		}

		if(verbose) ofLogNotice("ofxProgressiveTextureLoad") << "mipmapLevel " << mipmapLevel << " spent " << currentTime / 1000.0f << " ms and loaded " << scanlinesLoadedThisFrame << " lines across "<< loops << " loops";

		lastFrameTime = currentTime / 1000.0f; //in ms!

		if (loadedScanLinesSoFar >= pix->getHeight()){ //done!
			mipMapLevelLoaded = true;
			loadedScanLinesSoFar = 0;
		}

		glDisable(texture->texData.textureTarget);
		glBindTexture(texture->texData.textureTarget, 0);
		return couldGoOn;
	}


	void ofxProgressiveTextureLoad::draw(int x, int y, bool debugImages){

		if(readyForDeletion && useOnlyOnce) return;

		if(texture && debugImages){
			if(isReadyToDrawWhileLoading() || state == IDLE ){
				ofSetColor(255);
				float ar = texture->getWidth() / float(texture->getHeight());
				float xx = (texture->getWidth() - ofGetWidth()) * (ofGetMouseX() / float(ofGetWidth()));
				float yy = (texture->getHeight() - ofGetHeight()) * (ofGetMouseY() / float(ofGetHeight()));
				texture->draw(-xx, -yy);
				texture->draw(0,
							  0,
							  ofClamp(ofGetMouseX(), 10, 300),
							  ofClamp(ofGetMouseY(), 10, 300 / ar)
							  );
			}
		}

		string msg;
		switch (state) {
			case IDLE:
				ofDrawBitmapString("ofxProgressiveTextureLoad IDLE", x, y);
				break;

			case LOADING_PIXELS:
				ofDrawBitmapString("ofxProgressiveTextureLoad LOADING_PIXELS", x, y);
				break;

			case RESIZING_FOR_MIPMAPS:
				msg = "ofxProgressiveTextureLoad RESIZING_FOR_MIPMAPS\n";
				msg += "currentMipMap: " + ofToString(currentMipMapLevel);
				ofDrawBitmapString(msg, x, y);
				break;

			case ALLOC_TEXTURE:
				ofDrawBitmapString("ofxProgressiveTextureLoad ALLOC_TEXTURE", x, y);
				break;

			case LOADING_TEX:{
				msg = "ofxProgressiveTextureLoad LOADING_TEX\n";
				float percent = 100.0f * loadedScanLinesSoFar / mipMapLevelPixels[0]->getHeight();
				msg += "loaded: " + ofToString(percent,1) + "%";
				ofDrawBitmapString(msg, x, y);
			}break;

			case LOADING_MIP_MAPS:{
				msg = "ofxProgressiveTextureLoad LOADING_MIP_MAPS\n";
				msg += "currentMipMap: " + ofToString(currentMipMapLevel);
				float percent = 100.0f * loadedScanLinesSoFar / mipMapLevelPixels[0]->getHeight();
				msg += "\nloaded: " + ofToString(percent,1) + "%";
				ofDrawBitmapString(msg, x, y);
			}break;

			default:
				break;
		}
	}

	void ofxProgressiveTextureLoad::stopLoadingAsap(){
		cancelAsap = true;
	}

	string ofxProgressiveTextureLoad::getStateString(){
		string msg;
		switch (state) {
			case IDLE: return "IDLE";
			case LOADING_PIXELS: return "LOADING_PIXELS";
			case LOADING_FAILED: return "LOADING_FAILED";
			case RESIZING_FOR_MIPMAPS: return "RESIZING_FOR_MIPMAPS";
			case ALLOC_TEXTURE: return "ALLOC_TEXTURE";
			case LOADING_TEX:{
				msg += "LOADING_TEX (";
				float percent = 100.0f * loadedScanLinesSoFar / mipMapLevelPixels[0]->getHeight();
				msg += ofToString(percent,1) + "% loaded) " + (cancelAsap  ? " canceling!" : "");
				return msg;
			}
			case LOADING_MIP_MAPS:{
				msg += "LOADING_MIP_MAPS (mipMap: " + ofToString(currentMipMapLevel);
				float percent = 100.0f * loadedScanLinesSoFar / mipMapLevelPixels[currentMipMapLevel]->getHeight();
				msg += " " +ofToString(percent,1) + "%)" + (cancelAsap  ? " canceling! " : "");
				return msg;
			}
		}
		return "";
	}
	
