//
//  ofxProgressiveTextureLoad.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 16/09/14.
//
//

#include "ofxProgressiveTextureLoad.h"
#include "ofxOpenCv.h"
#include "ofxTimeMeasurements.h"
#include "ofxRemoteUIServer.h"

ofxProgressiveTextureLoad::ofxProgressiveTextureLoad(){

	numLinesPerFrame = 16;
	maxTimeTakenPerFrame = 0.5; //ms
	state == IDLE;
}

void ofxProgressiveTextureLoad::setup(ofTexture* tex){

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

				TS_STOP("loadPix");
				setState(RESIZING_FOR_MIPMAPS);
				break;

			case RESIZING_FOR_MIPMAPS:
				resizeImageForMipMaps();
				setState(ALLOC_TEXTURE);
				stopThread(); //at this point, we need the main thread to work
				break;
		}
	}
}


void ofxProgressiveTextureLoad::resizeImageForMipMaps(){

	int resizeMethod = CV_INTER_AREA; /// TODO! param!

	switch (originalImage.getPixelsRef().getImageType()) {
		case OF_IMAGE_COLOR:{

				int numC = 3;

				int newS = ofNextPow2(MAX(originalImage.width, originalImage.height));
				int mipMapLevel = floor(log2(newS)) + 1; //THIS IS KEY! you need to do all mipmap levels or it will draw blank tex!


				TS_START_NIF("mipmap 0");
				cv::Mat mipMap0(originalImage.width, originalImage.height, CV_8UC3);
				memcpy( mipMap0.data, originalImage.getPixels(), originalImage.width * originalImage.height * numC);
				cv::resize(mipMap0, mipMap0, cv::Size(newS, newS), 0, 0, resizeMethod);
				TS_STOP_NIF("mipmap 0");

				TS_START_NIF("mipmap 0 cp pix");
				ofPixels *pix = new ofPixels();
				pix->setFromPixels(mipMap0.data, newS, newS, numC/*RGB*/);
				mipmapsPixels[0] = pix;
				TS_STOP_NIF("mipmap 0 cp pix");

				if(createMipMaps){
					for(int i = 1 ; i < mipMapLevel; i++){

						TS_START_NIF("mipmap " + ofToString(i));
						cv::resize(mipMap0, mipMap0, cv::Size(mipMap0.cols/2, mipMap0.rows/2), 0, 0, resizeMethod);
						ofPixels * tmpPix;
						tmpPix = new ofPixels();
						tmpPix->setFromPixels(mipMap0.data, mipMap0.cols, mipMap0.rows, numC/*RGB*/);
						mipmapsPixels[i] = tmpPix;
						ofLog() << "mipmaps for level " << i << " ready (" << tmpPix->getWidth() << ", " << tmpPix->getWidth()<< ")";
						TS_STOP_NIF("mipmap " + ofToString(i));
						//ofSaveImage(*tmpPix, "pix" + ofToString(i) + ".jpg" ); //debug!

					}

				}else{
					ofLogError() << "wtf !!? ";
				}
			}break;

		default:
			ofLogError() << "img type not supported! " << originalImage.getPixelsRef().getImageType();
			break;
	}
}



void ofxProgressiveTextureLoad::update(){

	switch (state) {

		case ALLOC_TEXTURE:
			texture->clear();
			texture->allocate(originalImage.getPixelsRef());
			originalImage.clear(); //dealloc original image, we have all the ofPixels in a map!
			mipMapLevelLoaded = false;
			currentMipMapLevel = 0;
			glGenerateMipmap(texture->getTextureData().textureTarget);
			setState(LOADING_TEX);
			TS_START_NIF("upload 0");
			break;

		case LOADING_TEX:
			if(ofGetFrameNum()%5 == 1){
				progressiveTextureUpload(currentMipMapLevel);
			}

			if (mipMapLevelLoaded){ //mipmap0 is loaded! now for the smaller levels!
				TS_STOP_NIF("upload 0");
				if(createMipMaps){
					currentMipMapLevel++; //continue into mipmap 1
					mipMapLevelLoaded = false;
					mipMapLevelAllocPending = true;
					TS_START_NIF("upload " + ofToString(currentMipMapLevel));
					setState(LOADING_MIP_MAPS);
				}else{
					texture->setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
					pendingNotification = true;
					setState(IDLE);
				}
			}
			break;

		case LOADING_MIP_MAPS:
			if(ofGetFrameNum()%5 == 1){
				progressiveTextureUpload(currentMipMapLevel);
			}
			if (mipMapLevelLoaded){
				mipMapLevelLoaded = false;
				mipMapLevelAllocPending = true;
				TS_STOP_NIF("upload " + ofToString(currentMipMapLevel));
				currentMipMapLevel++;
				TS_START_NIF("upload " + ofToString(currentMipMapLevel));
				if (currentMipMapLevel == mipmapsPixels.size()){
					//done!
					glTexParameteri(texture->getTextureData().textureTarget, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(texture->getTextureData().textureTarget, GL_TEXTURE_MAX_LEVEL, mipmapsPixels.size());
					texture->setTextureMinMagFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
					setState(IDLE);
					TS_STOP_NIF("upload " + ofToString(currentMipMapLevel));
					pendingNotification = true;
				}
			}
			break;
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
	cout << "progressiveTextureUpload"<<"(" << mipmapLevel <<") " << pix->getWidth() << ", " << pix->getHeight() << endl;

	int c = 0;
	while (currentTime < maxTimeTakenPerFrame * 1000.0f && loadedScanLinesSoFar < pix->getHeight() ) {

		unsigned char * data = pix->getPixels() + numC * (int)pix->getWidth() * loadedScanLinesSoFar;

		int numToLoad = pix->getHeight() - loadedScanLinesSoFar;
		if (numToLoad > numLinesPerFrame){
			numToLoad = numLinesPerFrame;
		}

		if(mipmapLevel != 0 && mipMapLevelAllocPending){
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
		}
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
		currentTime += timer.getMicrosSinceLastCall();
		c++;
	}
	//cout << "timer " << currentTime / 1000 << "  loadedLines: " << c << endl;

	if (loadedScanLinesSoFar >= pix->getHeight()){ //done!
		mipMapLevelLoaded = true;
		loadedScanLinesSoFar = 0;
	}

	texture->unbind();
	glDisable(texture->texData.textureTarget);
}

void ofxProgressiveTextureLoad::draw(){

	if(state==IDLE){
		ofSetColor(255);
		texture->draw(0,0, ofGetMouseX(), ofGetMouseY());
	}
}