//
//  ofxProgressiveTextureLoad.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 16/09/14.
//
//

#include "ofxProgressiveTextureLoad.h"
#include <math.h>

#if(!DEBUG_TEX_LOADER_TIMES) //override TS_* if not debugging tex loader times
#undef TS_START_NIF
#undef TS_STOP_NIF
#define TS_START_NIF(x) ;
#define TS_STOP_NIF(x)	;
#endif

int ofxProgressiveTextureLoad::numInstancesCreated = 0;

ofxProgressiveTextureLoad::ofxProgressiveTextureLoad(){

	numLinesPerLoop = 16;
	maxTimeTakenPerFrame = 1.0; //ms
	currentMipMapLevel = 0;
	state = IDLE;
	verbose = false;
	lastFrameTime = 0.0f;
	texLodBias = -0.5;
	texture = NULL;
	ID = numInstancesCreated;
	numInstancesCreated++;
}

ofxProgressiveTextureLoad::~ofxProgressiveTextureLoad(){
	ofRemoveListener(ofEvents().update, this, &ofxProgressiveTextureLoad::update); //just in case
	//delete all mipmap pixels
	if(mipMapLevelPixels.size()){
		for(int i = 0; i < mipMapLevelPixels.size(); i++){
			delete mipMapLevelPixels[i];
		}
		mipMapLevelPixels.clear();
	}
}

void ofxProgressiveTextureLoad::setup(ofTexture* tex, int resizeQuality_){

	resizeQuality = resizeQuality_;
	texture = tex;
}

void ofxProgressiveTextureLoad::loadTexture(string path, bool withMipMaps){

	if (state == IDLE && texture){
		startTime = ofGetElapsedTimef();
		createMipMaps = withMipMaps;
		pendingNotification = cancelAsap = false;
		loadedScanLinesSoFar = 0;
		imagePath = path;
		setState(LOADING_PIXELS);
		startThread();
		ofAddListener(ofEvents().update, this, &ofxProgressiveTextureLoad::update);
		if(OFX_PROG_TEX_LOADER_MEAURE_TIMINGS) TS_START_NIF("total tex load time " + ofToString(ID));
	}else{
		ofLogError() << "cant load texture, busy!";
	}
}


void ofxProgressiveTextureLoad::threadedFunction(){

	getPocoThread().setName("ofxProgressiveTextureLoad " + ofToString(ID));

	while(isThreadRunning()){
		switch (state) {

			case LOADING_PIXELS:{
				
				TS_START_NIF("loadPix " + ofToString(ID));
				try{
					originalImage.setUseTexture(false);
					bool ok = originalImage.loadImage(imagePath);
					if (!ok){
						setState(LOADING_FAILED);
						ofLogError() << "ofxProgressiveTextureLoad: img loading failed! " << imagePath;
						stopThread();
					}else{
						TS_STOP_NIF("loadPix " + ofToString(ID));
						switch (originalImage.getPixelsRef().getImageType()) {
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
						setState(RESIZING_FOR_MIPMAPS);
					}
				}catch(...){
					TS_STOP_NIF("loadPix " + ofToString(ID));
					setState(LOADING_FAILED);
					ofLogError() << "exception in ofxProgressiveTextureLoad::threadedFunction()";
					stopThread();
				}
				}break;

			case RESIZING_FOR_MIPMAPS:
				TS_START_NIF("resizeImageForMipMaps " + ofToString(ID));
				try{
					resizeImageForMipMaps();
				}catch(...){
					setState(LOADING_FAILED); //mm TODO!
					stopThread();
					break;
				}
				setState(ALLOC_TEXTURE);
				TS_STOP_NIF("resizeImageForMipMaps " + ofToString(ID));
				return;
				break;
		}
	}

}


void ofxProgressiveTextureLoad::resizeImageForMipMaps(){

	int numC = config.numBytesPerPix;
	ofPoint targetSize = getMipMap0ImageSize();
	int newW = targetSize.x;
	int newH = targetSize.y;
	int mipMapLevel = floor(log( MAX(newW, newH) ) / log( 2 )) + 1; //THIS IS KEY! you need to do all mipmap levels or it will draw blank tex!

	//fill in an opencv image
	cv::Mat mipMap0(originalImage.height, originalImage.width, config.opencvFormat);
//		int wstep = mipMap0.step1(0);
//		if( mipMap0.cols * mipMap0.channels() == wstep ){
	memcpy( mipMap0.data,  originalImage.getPixels(), originalImage.width * originalImage.height * numC);
//		}else{
//			for( int i=0; i < originalImage.height; i++ ) {
//				memcpy( mipMap0.data + (i * wstep), originalImage.getPixels() + (i * originalImage.width * numC), originalImage.width * numC );
//			}
//		}

	if (createMipMaps || (!createMipMaps && ofGetUsingArbTex() == false) ){
		//resize to next power of two
		cv::resize(mipMap0, mipMap0, cv::Size(newW, newH), 0, 0, resizeQuality);
	}

	ofPixels *pix = new ofPixels();
	pix->setFromPixels(mipMap0.data, newW, newH, numC);
	mipMapLevelPixels[0] = pix;
	//ofSaveImage(*pix, "pix" + ofToString(0) + ".jpg" ); //debug!

	if(createMipMaps){

		for(int currentMipMapLevel = 1 ; currentMipMapLevel < mipMapLevel; currentMipMapLevel++){

			//TS_START_NIF("resize mipmap " + ofToString(currentMipMapLevel));
			int www = mipMap0.cols/2;
			int hhh = mipMap0.rows/2;
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
	}
}


ofPoint ofxProgressiveTextureLoad::getMipMap0ImageSize(){
	int newW;
	int newH;
	if (createMipMaps || (!createMipMaps && !ofGetUsingArbTex()) ){
		newW = ofNextPow2(originalImage.width);
		newH = ofNextPow2(originalImage.height);
	}else{
		newW = originalImage.width;
		newH = originalImage.height;
	}
	return ofPoint(newW, newH);
}


bool ofxProgressiveTextureLoad::isReadyToDrawWhileLoading(){
	if(isBusy() && createMipMaps){
		return (state == LOADING_MIP_MAPS && currentMipMapLevel < mipMapLevelPixels.size() - 3 );
	}
	return false;
}

void ofxProgressiveTextureLoad::update(ofEventArgs &d){

	TS_START_ACC("ProgTexLoad u");
	switch (state) {

		case LOADING_FAILED:{
			pendingNotification = true;
		}break;

		case ALLOC_TEXTURE:{
			texture->clear();
			ofPoint targetSize = getMipMap0ImageSize();
			int newW = targetSize.x;
			int newH = targetSize.y;

			ofPixels & pix = originalImage.getPixelsRef();
			texture->allocate(newW, newH,
							  ofGetGlInternalFormat(pix),
							  createMipMaps ? false : ofGetUsingArbTex(), //arb! no arb when creating mipmaps
							  ofGetGlFormat(pix),
							  ofGetGlType(pix)
							  );

			notifiedReadyToDraw = false;
			//setup OF texture sizes!
			if(texture->texData.textureTarget == GL_TEXTURE_RECTANGLE_ARB){
				texture->texData.tex_t = pix.getWidth();
				texture->texData.tex_u = pix.getHeight();
			}else{
				texture->texData.tex_t = 1;
				texture->texData.tex_u = 1;
			}
			texture->texData.tex_w = pix.getWidth();
			texture->texData.tex_h = pix.getHeight();
			texture->texData.width = pix.getWidth();
			texture->texData.height = pix.getHeight();

			originalImage.clear(); //dealloc original image, we have all the ofPixels in a map!
			mipMapLevelLoaded = false;
			if(createMipMaps){
				currentMipMapLevel = mipMapLevelPixels.size() - 1; //start by loading the smallest image (deepest mipmap)
				setState(LOADING_MIP_MAPS);
				mipMapLevelAllocPending = true; //otherwise we dont alloc space for the 1st mipmap level, and we get GL_INVALID_OPERATION
				TS_START_NIF("upload mipmaps " + ofToString(ID));
			}else{
				currentMipMapLevel = 0;
				setState(LOADING_TEX);
			}
			//TS_START_NIF("upload mipmap 0");
			}break;

		case LOADING_TEX:{
			if (cancelAsap){
				setState(IDLE);
				wrapUp();
			}else{
				uint64_t currentTime = 0;
				progressiveTextureUpload(currentMipMapLevel, currentTime);
				if (mipMapLevelLoaded){
					//TS_STOP_NIF("upload mipmap 0");
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
						texture->bind();
						glTexParameteri(texture->texData.textureTarget, GL_TEXTURE_BASE_LEVEL, currentMipMapLevel);
						glTexParameteri(texture->texData.textureTarget, GL_TEXTURE_MAX_LEVEL, mipMapLevelPixels.size() - 1  );
						int mipmapsLoaded = mipMapLevelPixels.size() - currentMipMapLevel;
						if (mipmapsLoaded >= 1 && !notifiedReadyToDraw){ //notify the user that the texture is drawable right now, and will progressively draw
							notifiedReadyToDraw = true;
							textureEvent ev;
							ev.loaded = true;
							ev.who = this;
							ev.tex = texture;
							ev.elapsedTime = ofGetElapsedTimef() - startTime;
							ev.texturePath = imagePath;
							ofNotifyEvent(textureDrawable, ev, this);
							glTexParameteri(texture->texData.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameteri(texture->texData.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
						}
						if (mipmapsLoaded >= 4){
							glTexParameterf(texture->texData.textureTarget, GL_TEXTURE_LOD_BIAS, texLodBias);
						}
						texture->unbind();

						mipMapLevelLoaded = false;
						mipMapLevelAllocPending = true;

						if (currentMipMapLevel == 0){ //all mipmaps loaded! done!
							wrapUp();
							wrappingUp = true;
							TS_STOP_NIF("upload mipmaps " + ofToString(ID));
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
		textureEvent ev;
		if (state == LOADING_FAILED || cancelAsap == true){
			ev.loaded = false;
		}else{
			ev.loaded = true;
		}
		ev.canceledLoad = cancelAsap;
		ev.who = this;
		ev.tex = texture;
		ev.elapsedTime = ofGetElapsedTimef() - startTime;
		ev.texturePath = imagePath;
		ofNotifyEvent(textureReady, ev, this);
		setState(IDLE);
		if(OFX_PROG_TEX_LOADER_MEAURE_TIMINGS) TS_STOP_NIF("total tex load time " + ofToString(ID));
	}
	TS_STOP_ACC("ProgTexLoad u");
}

void ofxProgressiveTextureLoad::wrapUp(){

	ofRemoveListener(ofEvents().update, this, &ofxProgressiveTextureLoad::update);
	//delete all mipmap pixels
	for(int i = 0; i < mipMapLevelPixels.size(); i++){
		delete mipMapLevelPixels[i];
	}
	mipMapLevelPixels.clear();
	pendingNotification = true;
	//setState(IDLE);
}


bool ofxProgressiveTextureLoad::progressiveTextureUpload(int mipmapLevel, uint64_t & currentTime){

	GLuint glFormat = config.glFormat;
	GLuint glPixelType = GL_UNSIGNED_BYTE;

	texture->bind();
	int numC = ofGetNumChannelsFromGLFormat(glFormat);
	timer.getMicrosSinceLastCall();

	ofPixels * pix = mipMapLevelPixels[mipmapLevel];
	int scanlinesLoadedThisFrame = 0;
	int loops = 0;

	#if (OF_VERSION_PATCH <= 3)
		ofSetPixelStorei(pix->getWidth(), 1, ofGetNumChannelsFromGLFormat(glFormat));
	#else
		ofSetPixelStoreiAlignment(GL_PACK_ALIGNMENT, pix->getWidth(), 1, ofGetNumChannelsFromGLFormat(glFormat));
	#endif

	while (currentTime < maxTimeTakenPerFrame * 1000.0f && loadedScanLinesSoFar < pix->getHeight()) {

		unsigned char * data = pix->getPixels() + numC * (int)pix->getWidth() * loadedScanLinesSoFar;

		int numLinesToLoadThisLoop = pix->getHeight() - loadedScanLinesSoFar;
		if (numLinesToLoadThisLoop > numLinesPerLoop){
			numLinesToLoadThisLoop = numLinesPerLoop;
		}

		if(mipMapLevelAllocPending){ //level 0 mipmap is already allocated!
			//TS_START_NIF("glTexImage2D mipmap " + ofToString(currentMipMapLevel));
			mipMapLevelAllocPending = false;
			glTexImage2D(texture->texData.textureTarget,	//target
						 mipmapLevel,						//mipmap level
						 texture->texData.glTypeInternal,	//internal format
						 pix->getWidth(),					//w
						 pix->getHeight(),					//h
						 0,									//border
						 glFormat,							//format
						 glPixelType,						//type
						 0 );								//pixels
			//TS_STOP_NIF("glTexImage2D mipmap " + ofToString(currentMipMapLevel));
		}

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

		loadedScanLinesSoFar += numLinesToLoadThisLoop;
		scanlinesLoadedThisFrame += numLinesToLoadThisLoop;
		int timeThisLoop = timer.getMicrosSinceLastCall();
		currentTime += timeThisLoop;
		if(verbose) cout << "loop " << loops << " loaded " << numLinesToLoadThisLoop << " lines and took " << timeThisLoop / 1000.0f << " ms" << endl;
		loops++;
	}
	//if we finshed this mipmap but there's time left, we could go on...
	bool canGoOn = false;
	if(currentTime < maxTimeTakenPerFrame * 1000.0f){
		canGoOn = true;
	}

	if(verbose) cout << "mipmapLevel " << mipmapLevel << " spent " << currentTime / 1000.0f << " ms and loaded " << scanlinesLoadedThisFrame << " lines across "<< loops << " loops" << endl;

	lastFrameTime = currentTime / 1000.0f; //in ms!

	if (loadedScanLinesSoFar >= pix->getHeight()){ //done!
		mipMapLevelLoaded = true;
		loadedScanLinesSoFar = 0;
	}

	texture->unbind();
	glDisable(texture->texData.textureTarget);
	return canGoOn;
}


void ofxProgressiveTextureLoad::draw(int x, int y, bool debugImages){

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
			msg += ofToString(percent,1) + "% loaded) " + (cancelAsap  ? "pending cancel!" : "");
			return msg;
		}
		case LOADING_MIP_MAPS:{
			msg += "LOADING_MIP_MAPS (currentMipMap: " + ofToString(currentMipMapLevel);
			float percent = 100.0f * loadedScanLinesSoFar / mipMapLevelPixels[currentMipMapLevel]->getHeight();
			msg += " " +ofToString(percent,1) + "% loaded)" + (cancelAsap  ? "pending cancel!" : "");
			return msg;
		}
	}
}

#if(DEBUG_TEX_LOADER_TIMES) //override TS_* if not debugging tex loader times
#define TS_START_NIF(x) TIME_SAMPLE_START_NOIF(x, ##__VA_ARGS__);
#define TS_STOP_NIF(x)	TIME_SAMPLE_STOP_NOIF(x, ##__VA_ARGS__);
#endif
