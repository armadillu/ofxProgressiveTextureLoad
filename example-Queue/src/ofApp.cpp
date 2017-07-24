#include "ofApp.h"

#include "ofxTimeMeasurements.h"


void ofApp::setup(){

	imageNames.push_back("huge_a.png"); //rgba , 4channels
	imageNames.push_back("huge.jpg");	//rgb, 3 channels
	imageNames.push_back("huge_bw.jpg");//luminance, 1 channel

	ofBackground(33);
	ofEnableAlphaBlending();
	ofSetVerticalSync(true);
	ofSetFrameRate(60);

	//setup time measurements
	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_DISABLE_AVERAGE();
	TIME_SAMPLE_GET_INSTANCE()->setIdleTimeColorFadePercent(0.3);
	TIME_SAMPLE_SET_REMOVE_EXPIRED_THREADS(false);
	ofxTimeMeasurements::instance()->setDeadThreadTimeDecay(0.99);
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);


	ProgressiveTextureLoadQueue * q = ProgressiveTextureLoadQueue::instance();

	q->setTexLodBias(-0.5); //negative gives you lower mipmaps >> sharper
	q->setScanlinesPerLoop(32); //how many scanlines to upload per batch; we will do as many batches we can fit each frame, given the user alloted time
	q->setTargetTimePerFrame(3); //set how many MS to spend at most each frame uploading texture data
	q->setNumberSimultaneousLoads(1); //how many textures can be prepared in the back at the same time. This should be <= your number of CPU cores.
	q->setVerbose(true);

	for(int i = 0; i < 3; i++){
		ofTexture* t = new ofTexture(); //create your own texture to got data loaded into; it will be cleared!
		textures.push_back(t);
		ready[t] = false;
		ofxProgressiveTextureLoad * loader = q->loadTexture(imageNames[i%imageNames.size()], //file to load
															t,					//ofTexture to load into - it will be cleared for you
															true,				//do you want mipmaps created for this texture?
															true,               //USE ARB? meaning GL_TEXTURE_RECTANGLE_ARB or GL_TEXTURE_2D - creating mimMaps requires GL_TEXTURE_2D!
															CV_INTER_AREA,		//opencv resize quality - to create mipmaps we need to downsample the original image; what resizing quality should we choose in doing so?
															false				//high priority - puts request at beginning of the queue instead of at the end of it
															);
		ofAddListener(loader->textureReady, this, &ofApp::textureReady);
		ofAddListener(loader->textureDrawable, this, &ofApp::textureDrawable);
	}
}


void ofApp::update(){

}


void ofApp::textureDrawable(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent& arg){
	ofLogNotice() << "texture Drawable!";
}


void ofApp::textureReady(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent& arg){
	if (arg.ok){
		//ofLogNotice() << "textureReady!";
		ready[arg.tex] = true;
	}else{
		ofLogError() << "texture load failed!" << arg.texturePath;
	}
}


void ofApp::draw(){

	//texture
	ofSetColor(255);
	textures[0]->draw(0, 0, ofGetWidth(), ofGetHeight());

	int x = 0;
	for(int i = 0; i < textures.size(); i++){
		if(ready[textures[i]]){
			float targetW = 200;
			float w = textures[i]->getWidth();
			float h = textures[i]->getHeight();
			float s = targetW / w;
			ofRectangle r = ofRectangle(x, 0, w * s, h * s);
			ofSetColor(255);
			textures[i]->draw(r);
			ofNoFill();
			ofSetColor(255,0,0);
			ofRect(r);
			ofFill();
			ofDrawBitmapString(ofToString(i), x + 30 , 52);
			x += w * s;
		}
	}

	//check for blocking main thread - visual clock
	float s = 50;
	ofPushMatrix();
	ofTranslate(ofGetWidth()/2, 70);
	ofSetColor(0);
	ofCircle(0, 0, s);
	ofRotate(ofGetFrameNum() * 5.0, 0, 0, 1);
	ofSetColor(255);
	ofRect(0, 0, s, 5);
	ofPopMatrix();

	ProgressiveTextureLoadQueue::instance()->draw(20, ofGetHeight() - 80);
}


void ofApp::keyPressed(int key){
}


void ofApp::keyReleased(int key){

}


void ofApp::mouseMoved(int x, int y){

}


void ofApp::mouseDragged(int x, int y, int button){

}


void ofApp::mousePressed(int x, int y, int button){
}


void ofApp::mouseReleased(int x, int y, int button){

}


void ofApp::windowResized(int w, int h){

}


void ofApp::gotMessage(ofMessage msg){

}


void ofApp::dragEvent(ofDragInfo dragInfo){ 

}