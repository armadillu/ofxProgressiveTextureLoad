#include "testApp.h"
#include "ofxGLError.h"

#include "ofxTimeMeasurements.h"

string imgName = "huge.jpg";

void testApp::setup(){

	ofBackground(33);
	ofEnableAlphaBlending();
	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	ofDisableArbTex(); //POW2 textueres, GL_TEXTURE_2D!
	//ofEnableArbTex();

	textureReadyToDraw = false;

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_DISABLE_AVERAGE();
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);

	myTex = new ofTexture(); //create your own texture, it will be cleared so be sure its empty

	//setup the loader by giving it a texture to load into, and a resizing quality
	//CV_INTER_LINEAR, CV_INTER_NN, CV_INTER_CUBIC, CV_INTER_AREA
	progressiveTextureLoader.setup(myTex, CV_INTER_CUBIC);
	progressiveTextureLoader.setVerbose(true);

	//these 2 settings control how long it takes for the tex to load
	progressiveTextureLoader.setScanlinesPerLoop(512);//see header for explanation
	progressiveTextureLoader.setTargetTimePerFrame(10.0f); //how long to spend uploading data per frame, in ms

	//add a listener to get notified when tex is fully loaded
	//and one to let you know when the texture is drawable
	ofAddListener(progressiveTextureLoader.textureReady, this, &testApp::textureReady);
	ofAddListener(progressiveTextureLoader.textureDrawable, this, &testApp::textureDrawable);

	//start loading the texture!
	TIME_SAMPLE_START_NOIF("Total Load Time");
	progressiveTextureLoader.loadTexture(imgName, true /*create mipmaps*/);

	plot = new ofxHistoryPlot( NULL, "frameTime", 400, false);
	plot->setRange(0, 18.0f);
	//plot->setLowerRange(0);
	plot->addHorizontalGuide(16.66f, ofColor(0,255,0));
	plot->setColor( ofColor(255,0,0) );
	plot->setBackgroundColor(ofColor(0,220));
	plot->setShowNumericalInfo(true);
	plot->setRespectBorders(true);
	plot->setLineWidth(1);
	plot->setShowSmoothedCurve(false);
	plot->setSmoothFilter(0.04);
}


void testApp::update(){
	
	progressiveTextureLoader.update();

	if(progressiveTextureLoader.isBusy()){
		plot->update(progressiveTextureLoader.getTimeSpentLastFrame());
	}

}

void testApp::textureDrawable(ofxProgressiveTextureLoad::textureEvent& arg){
	ofLogNotice() << "texture Drawable!";
	textureReadyToDraw = true;
}

void testApp::textureReady(ofxProgressiveTextureLoad::textureEvent& arg){
	if (arg.ok){
		ofLogNotice() << "textureReady!";
		TIME_SAMPLE_STOP_NOIF("Total Load Time");
		textureReadyToDraw = true;
	}else{
		ofLogError() << "texture load failed!" << arg.texturePath;
	}
}


void testApp::draw(){

	//texture
	ofSetColor(255);
	
	if(textureReadyToDraw){
		myTex->draw(0,0, ofGetWidth(), ofGetHeight());
	}
	progressiveTextureLoader.draw(20, 50); //debug

	//clock
	float s = 50;
	ofPushMatrix();
	ofTranslate(ofGetWidth()/2, 70);
	ofSetColor(0);
	ofCircle(0, 0, s);
	ofRotate(ofGetFrameNum() * 3.0, 0, 0, 1);
	ofSetColor(255);
	ofRect(0, 0, s, 5);
	ofPopMatrix();

	ofxGLError::draw(20,20);
	float H = 200;
	plot->draw(0, ofGetHeight() - H, ofGetWidth(), H);
}


void testApp::keyPressed(int key){
	if(key == ' ' && !progressiveTextureLoader.isBusy()){
		TIME_SAMPLE_START_NOIF("Total Load Time");
		myTex->clear();
		textureReadyToDraw = false;
		progressiveTextureLoader.loadTexture(imgName, true /*create mipmaps*/);
	}
	if(key=='c') myTex->clear();
}


void testApp::keyReleased(int key){

}


void testApp::mouseMoved(int x, int y){

}


void testApp::mouseDragged(int x, int y, int button){

}


void testApp::mousePressed(int x, int y, int button){
}


void testApp::mouseReleased(int x, int y, int button){

}


void testApp::windowResized(int w, int h){

}


void testApp::gotMessage(ofMessage msg){

}


void testApp::dragEvent(ofDragInfo dragInfo){ 

}