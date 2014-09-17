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

ofxProgressiveTextureLoad::ofxProgressiveTextureLoad(){

	numLinesPerFrame = 16;
	maxTimeTakenPerFrame = 0.5; //ms
	state == IDLE;
}

void ofxProgressiveTextureLoad::setup(ofTexture* tex, int resizeQuality_){

	resizeQuality = resizeQuality_;
	texture = tex;
	RUI_SHARE_PARAM(numLinesPerFrame, 1, 256);
	RUI_SHARE_PARAM(maxTimeTakenPerFrame, 0.1, 8); //ms

}

void ofxProgressiveTextureLoad::loadTexture(string path, bool withMipMaps){

	if (state == IDLE){
		createMipMaps = withMipMaps;
		pendingNotification = false;
		loadedScanLinesSoFar = 0;
		imagePath = path;
		setState(LOADING_PIXELS);
		startThread();
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

		case OF_IMAGE_COLOR:{

			int numC = 3;
			int newW = ofNextPow2(originalImage.width);
			int newH = ofNextPow2(originalImage.height);
			int newS = MAX(newW, newH);
			int mipMapLevel = floor(log2(newS)) + 1; //THIS IS KEY! you need to do all mipmap levels or it will draw blank tex!

			TS_START_NIF("resize mipmap 0");
			//fill in an opencv image
			cv::Mat mipMap0(originalImage.width, originalImage.height, (numC == 3 ? CV_8UC3 : CV_8UC4));
			memcpy( mipMap0.data, originalImage.getPixels(), originalImage.width * originalImage.height * numC);
			//if(originalImage.width != newS && )
			//resize to next power of two
			cv::resize(mipMap0, mipMap0, cv::Size(newS, newS), 0, 0, resizeQuality);

			ofPixels *pix = new ofPixels();
			pix->setFromPixels(mipMap0.data, newS, newS, numC/*RGB*/);
			mipmapsPixels[0] = pix;
			TS_STOP_NIF("resize mipmap 0");
			ofSaveImage(*pix, "pix" + ofToString(0) + ".jpg" ); //debug!

			if(createMipMaps){
				for(int currentMipMapLevel = 1 ; currentMipMapLevel < mipMapLevel; currentMipMapLevel++){

					TS_START_NIF("resize mipmap " + ofToString(currentMipMapLevel));
					cv::resize(mipMap0, mipMap0, cv::Size(mipMap0.cols/2, mipMap0.rows/2), 0, 0, resizeQuality);
					ofPixels * tmpPix;
					tmpPix = new ofPixels();
					tmpPix->setFromPixels(mipMap0.data, mipMap0.cols, mipMap0.rows, numC/*RGB*/);
					mipmapsPixels[currentMipMapLevel] = tmpPix;
					ofLog() << "mipmaps for level " << currentMipMapLevel << " ready (" << tmpPix->getWidth() << ", " << tmpPix->getWidth()<< ")";
					TS_STOP_NIF("resize mipmap " + ofToString(currentMipMapLevel));
					ofSaveImage(*tmpPix, "pix" + ofToString(currentMipMapLevel) + ".jpg" ); //debug!
				}
			}
			}break;

		default:
			ofLogError() << "img type not supported! " << originalImage.getPixelsRef().getImageType();
			break;
	}
}


void ofxProgressiveTextureLoad::update(){

	switch (state) {

		case ALLOC_TEXTURE:{
			texture->clear();
			int newW = ofNextPow2(originalImage.width);
			int newH = ofNextPow2(originalImage.height);
			ofPixels & pix = originalImage.getPixelsRef();
			texture->allocate(newW, newH,
							  ofGetGlInternalFormat(pix),
							  createMipMaps? true : ofGetUsingArbTex(),
							  ofGetGlFormat(pix),
							  ofGetGlType(pix));
			originalImage.clear(); //dealloc original image, we have all the ofPixels in a map!
			mipMapLevelLoaded = false;
			currentMipMapLevel = 0;
			glGenerateMipmap(texture->getTextureData().textureTarget);
			setState(LOADING_TEX);
			TS_START_NIF("upload mipmap 0");
			}break;

		case LOADING_TEX:
			//if(ofGetFrameNum()%5 == 1){
				progressiveTextureUpload(currentMipMapLevel);
			//}

			if (mipMapLevelLoaded){ //mipmap0 is loaded! now for the smaller levels!
				TS_STOP_NIF("upload mipmap 0");
				if(createMipMaps){
					currentMipMapLevel++; //continue into mipmap 1
					mipMapLevelLoaded = false;
					mipMapLevelAllocPending = true;
					TS_START_NIF("upload mipmap " + ofToString(currentMipMapLevel));
					setState(LOADING_MIP_MAPS);
				}else{
					texture->setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
					pendingNotification = true;
					setState(IDLE);
				}
			}
			break;

		case LOADING_MIP_MAPS:
			//if(ofGetFrameNum()%5 == 1){
				progressiveTextureUpload(currentMipMapLevel);
			//}
			if (mipMapLevelLoaded){
				mipMapLevelLoaded = false;
				mipMapLevelAllocPending = true;
				TS_STOP_NIF("upload mipmap " + ofToString(currentMipMapLevel));
				currentMipMapLevel++;
				TS_START_NIF("upload mipmap " + ofToString(currentMipMapLevel));
				if (currentMipMapLevel == mipmapsPixels.size()){
					//done!
					glTexParameteri(texture->getTextureData().textureTarget, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(texture->getTextureData().textureTarget, GL_TEXTURE_MAX_LEVEL, mipmapsPixels.size());
					texture->setTextureMinMagFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
					setState(IDLE);
					TS_STOP_NIF("upload mipmap " + ofToString(currentMipMapLevel));
					pendingNotification = true;
				}
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


void ofxProgressiveTextureLoad::progressiveTextureUpload(int mipmapLevel){

	GLuint glFormat = GL_RGB;
	GLuint glPixelType = GL_UNSIGNED_BYTE;

	texture->bind();
	uint64_t currentTime = 0;
	int numC = ofGetNumChannelsFromGLFormat(glFormat);

	//doesnt seem to affect perfomance?
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, tex.texData.tex_w ); //in pixels, not bytes!
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, 0 ); //in pixels, not bytes!

	timer.getMicrosSinceLastCall();

	ofPixels* pix = mipmapsPixels[mipmapLevel];

	int c = 0;
	while (currentTime < maxTimeTakenPerFrame * 1000.0f && loadedScanLinesSoFar < pix->getHeight() ) {

		unsigned char * data = pix->getPixels() + numC * (int)pix->getWidth() * loadedScanLinesSoFar;

		int numToLoad = pix->getHeight() - loadedScanLinesSoFar;
		if (numToLoad > numLinesPerFrame){
			numToLoad = numLinesPerFrame;
		}

		if(mipmapLevel != 0 && mipMapLevelAllocPending){
			TS_START_NIF("glTexImage2D mipmap " + ofToString(currentMipMapLevel));
			mipMapLevelAllocPending = false;
			glTexImage2D(texture->getTextureData().textureTarget,	//target
						 mipmapLevel,								//mipmap level
						 texture->getTextureData().glTypeInternal,	//internal format
						 pix->getWidth(),							//w
						 pix->getHeight(),							//h
						 0,											//border
						 glFormat,									//format
						 glPixelType,								//type
						 0 );										//pixels
			cout << "!!! glTexImage2D " << mipmapLevel << endl;
			TS_STOP_NIF("glTexImage2D mipmap " + ofToString(currentMipMapLevel));
		}

		//if(ofGetFrameNum()%2 == 1){
			glTexSubImage2D(texture->texData.textureTarget,	//target
							mipmapLevel,					//mipmap level
							0,								//x offset
							loadedScanLinesSoFar,			//y offset
							pix->getWidth(),				//width
							numToLoad,						//height >> numLinesPerFrame line per iteration
							glFormat,						//format
							glPixelType,					//type
							data							//pixels
							);

			loadedScanLinesSoFar += numToLoad;
			c++;
		//}
		currentTime += timer.getMicrosSinceLastCall();
	}
	glFlush();
	cout << "mipmapLevel " << mipmapLevel << " spent " << currentTime / 1000.0f << " ms and loaded " << c << " lines" << endl;

	if (loadedScanLinesSoFar >= pix->getHeight()){ //done!
		mipMapLevelLoaded = true;
		loadedScanLinesSoFar = 0;
	}

	texture->unbind();
	glDisable(texture->texData.textureTarget);
}

void ofxProgressiveTextureLoad::draw(int x, int y){

	if(state==IDLE){
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
			float percent = 100.0f * loadedScanLinesSoFar / mipmapsPixels[0]->getHeight();
			msg += "\nloaded: " + ofToString(percent,1) + "%";
			ofDrawBitmapString(msg, x, y);
		}break;

		case LOADING_MIP_MAPS:{
			msg = "ofxProgressiveTextureLoad LOADING_MIP_MAPS\n";
			msg += "currentMipMap: " + ofToString(currentMipMapLevel);
			float percent = 100.0f * loadedScanLinesSoFar / mipmapsPixels[0]->getHeight();
			msg += "\nloaded: " + ofToString(percent,1) + "%";
			ofDrawBitmapString(msg, x, y);
			}break;

		default:
			break;
	}

}