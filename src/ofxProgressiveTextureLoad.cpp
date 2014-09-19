//
//  ofxProgressiveTextureLoad.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 16/09/14.
//
//

#include "ofxProgressiveTextureLoad.h"
#include "ofxTimeMeasurements.h"
#include "ofxRemoteUIServer.h"
#include <math.h>

ofxProgressiveTextureLoad::ofxProgressiveTextureLoad(){

	numLinesPerLoop = 16;
	maxTimeTakenPerFrame = 0.5; //ms
	state = IDLE;
	texture = NULL;
}

void ofxProgressiveTextureLoad::setup(ofTexture* tex, int resizeQuality_){

	resizeQuality = resizeQuality_;
	texture = tex;
	RUI_SHARE_PARAM(numLinesPerLoop, 1, 256);
	RUI_SHARE_PARAM(maxTimeTakenPerFrame, 0.1, 8); //ms
	RUI_WATCH_PARAM(numLinesPerLoop);

}

void ofxProgressiveTextureLoad::loadTexture(string path, bool withMipMaps){

	if (state == IDLE){
		createMipMaps = withMipMaps;
		pendingNotification = false;
		loadedScanLinesSoFar = 0;
		imagePath = path;
		setState(LOADING_PIXELS);
		startThread();
		ofAddListener(ofEvents().update, this, &ofxProgressiveTextureLoad::update);
	}else{
		ofLogError() << "cant load texture, busy!";
	}
}


void ofxProgressiveTextureLoad::threadedFunction(){

	while(isThreadRunning()){

		switch (state) {
			case LOADING_PIXELS:
				TS_START("loadPix");
				originalImage.setUseTexture(false);
				originalImage.loadImage(imagePath);
				//originalImage.setImageType(OF_IMAGE_COLOR_ALPHA); //testing rgba maybe faster upload?

				TS_STOP("loadPix");
				setState(RESIZING_FOR_MIPMAPS);
				break;

			case RESIZING_FOR_MIPMAPS:
				resizeImageForMipMaps();
				setState(ALLOC_TEXTURE);
				stopThread(); //at this point, we need to do the work from the main thread
				break;
		}
	}
}


void ofxProgressiveTextureLoad::resizeImageForMipMaps(){


	switch (originalImage.getPixelsRef().getImageType()) {

		case OF_IMAGE_COLOR:{ //todo rgba!

			int numC = 3;
			ofPoint targetSize = getMipMap0ImageSize();
			int newW = targetSize.x;
			int newH = targetSize.y;
			int mipMapLevel = floor(log( MAX(newW, newH) ) / log( 2 )) + 1; //THIS IS KEY! you need to do all mipmap levels or it will draw blank tex!

			TS_START_NIF("resize mipmap 0");
			//fill in an opencv image
			cv::Mat mipMap0(originalImage.height, originalImage.width, (numC == 3 ? CV_8UC3 : CV_8UC4));
//			int wstep = mipMap0.step1(0);
//			if( mipMap0.cols * mipMap0.channels() == wstep ){
				memcpy( mipMap0.data,  originalImage.getPixels(), originalImage.width * originalImage.height * numC);
//			}else{
//				for( int i=0; i < originalImage.height; i++ ) {
//					memcpy( mipMap0.data + (i * wstep), originalImage.getPixels() + (i * originalImage.width * numC), originalImage.width * numC );
//				}
//			}

			if (createMipMaps || (!createMipMaps && ofGetUsingArbTex() == false) ){
				//resize to next power of two
				cv::resize(mipMap0, mipMap0, cv::Size(newW, newH), 0, 0, resizeQuality);
			}

			ofPixels *pix = new ofPixels();
			pix->setFromPixels(mipMap0.data, newW, newH, numC/*RGB*/);
			mipMapLevelPixels[0] = pix;
			TS_STOP_NIF("resize mipmap 0");
			//ofSaveImage(*pix, "pix" + ofToString(0) + ".jpg" ); //debug!

			if(createMipMaps){

				for(int currentMipMapLevel = 1 ; currentMipMapLevel < mipMapLevel; currentMipMapLevel++){

					TS_START_NIF("resize mipmap " + ofToString(currentMipMapLevel));
					int www = mipMap0.cols/2;
					int hhh = mipMap0.rows/2;
					if (www < 1) www = 1; if (hhh < 1) hhh = 1;
					cv::resize(mipMap0, mipMap0, cv::Size(www, hhh), 0, 0, resizeQuality);
					ofPixels * tmpPix;
					tmpPix = new ofPixels();
					tmpPix->setFromPixels(mipMap0.data, mipMap0.cols, mipMap0.rows, numC/*RGB*/);
					mipMapLevelPixels[currentMipMapLevel] = tmpPix;
					ofLog() << "mipmaps for level " << currentMipMapLevel << " ready (" << tmpPix->getWidth() << ", " << tmpPix->getWidth()<< ")";
					TS_STOP_NIF("resize mipmap " + ofToString(currentMipMapLevel));
					//ofSaveImage(*tmpPix, "pix" + ofToString(currentMipMapLevel) + ".jpg" ); //debug!
				}
			}
			}break;

		default:
			ofLogError() << "img type not supported! " << originalImage.getPixelsRef().getImageType();
			break;
	}
}


ofPoint ofxProgressiveTextureLoad::getMipMap0ImageSize(){
	int newW;
	int newH;
	if (createMipMaps || (!createMipMaps && texture->getTextureData().textureTarget != GL_TEXTURE_RECTANGLE_ARB) ){
		newW = ofNextPow2(originalImage.width);
		newH = ofNextPow2(originalImage.height);
	}else{
		newW = originalImage.width;
		newH = originalImage.height;
	}
	return ofPoint(newW, newH);
}


void ofxProgressiveTextureLoad::update(ofEventArgs &d){

	switch (state) {

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
			originalImage.clear(); //dealloc original image, we have all the ofPixels in a map!
			mipMapLevelLoaded = false;
			if(createMipMaps){
				currentMipMapLevel = 0; //start by loading the smallest image (deepest mipmap)
				setState(LOADING_MIP_MAPS);
				cout << "start " << currentMipMapLevel << endl;
				TS_START_NIF("upload mipmap " + ofToString(currentMipMapLevel));
			}else{
				currentMipMapLevel = 0;
				setState(LOADING_TEX);
			}
			TS_START_NIF("upload mipmap 0");
			}break;

		case LOADING_TEX:
			progressiveTextureUpload(currentMipMapLevel);
			if (mipMapLevelLoaded){
				TS_STOP_NIF("upload mipmap 0");
				texture->setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
				wrapUp();
			}
			break;

		case LOADING_MIP_MAPS:

			if (mipMapLevelLoaded){

//				glTexParameteri(texture->getTextureData().textureTarget, GL_TEXTURE_BASE_LEVEL, 0);
//				glTexParameteri(texture->getTextureData().textureTarget, GL_TEXTURE_MAX_LEVEL, mipMapLevelPixels.size() - 1);

				mipMapLevelLoaded = false;
				mipMapLevelAllocPending = true;

				TS_STOP_NIF("upload mipmap " + ofToString(currentMipMapLevel));

				if (currentMipMapLevel == mipMapLevelPixels.size() - 1){ //all mipmaps loaded! done!
					texture->setTextureMinMagFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
					wrapUp();

				}else{
					currentMipMapLevel++;
					TS_START_NIF("upload mipmap " + ofToString(currentMipMapLevel));
				}
			}else{
				progressiveTextureUpload(currentMipMapLevel);
			}
			break;
	}

	if(pendingNotification){
		pendingNotification = false;
		textureEvent ev;
		ev.who = this;
		ev.tex = texture;
		ev.texturePath = imagePath;
		ofNotifyEvent(textureReady, ev, this);
	}
}

void ofxProgressiveTextureLoad::wrapUp(){

	ofRemoveListener(ofEvents().update, this, &ofxProgressiveTextureLoad::update);
	//delete all mipmap pixels
	for(int i = 0; i < mipMapLevelPixels.size(); i++){
		delete mipMapLevelPixels[i];
	}
	mipMapLevelPixels.clear();
	pendingNotification = true;
	setState(IDLE);
}


void ofxProgressiveTextureLoad::progressiveTextureUpload(int mipmapLevel){

	GLuint glFormat = GL_RGB;
	GLuint glPixelType = GL_UNSIGNED_BYTE;

	texture->bind();
	uint64_t currentTime = 0;
	int numC = ofGetNumChannelsFromGLFormat(glFormat);
	timer.getMicrosSinceLastCall();

	ofPixels * pix = mipMapLevelPixels[mipmapLevel];
	int scanlinesLoadedThisFrame = 0;
	int loops = 0;

	ofSetPixelStorei(pix->getWidth(), 1, ofGetNumChannelsFromGLFormat(glFormat));

	while (currentTime < maxTimeTakenPerFrame * 1000.0f && loadedScanLinesSoFar < pix->getHeight()) {

		unsigned char * data = pix->getPixels() + numC * (int)pix->getWidth() * loadedScanLinesSoFar;

		int numLinesToLoadThisLoop = pix->getHeight() - loadedScanLinesSoFar;
		if (numLinesToLoadThisLoop > numLinesPerLoop){
			numLinesToLoadThisLoop = numLinesPerLoop;
		}

		if( mipMapLevelAllocPending){ //level 0 mipmap is already allocated!
			TS_START_NIF("glTexImage2D mipmap " + ofToString(currentMipMapLevel));
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

			TS_STOP_NIF("glTexImage2D mipmap " + ofToString(currentMipMapLevel));
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
		//cout << "loop " << loops << " loaded " << numLinesToLoadThisLoop << " lines and took " << timeThisLoop / 1000.0f << " ms" << endl;
		loops++;
	}

	//cout << "mipmapLevel " << mipmapLevel << " spent " << currentTime / 1000.0f << " ms and loaded " << scanlinesLoadedThisFrame << " lines across "<< loops << " loops" << endl;

	if (loadedScanLinesSoFar >= pix->getHeight()){ //done!
		mipMapLevelLoaded = true;
		loadedScanLinesSoFar = 0;
	}

	texture->unbind();
	glDisable(texture->texData.textureTarget);
}


void ofxProgressiveTextureLoad::draw(int x, int y){

	if(texture){
		ofSetColor(255);
		texture->draw(0,0,
					  ofGetMouseX() > 50 ? ofGetMouseX() : 50,
					  ofGetMouseY() > 50 ? ofGetMouseY() : 50);
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
			msg += "currentMipMap: " + ofToString(currentMipMapLevel);
			float percent = 100.0f * loadedScanLinesSoFar / mipMapLevelPixels[0]->getHeight();
			msg += "\nloaded: " + ofToString(percent,1) + "%";
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