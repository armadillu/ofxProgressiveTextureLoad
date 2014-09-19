#include "testApp.h"
#include "ofxGLError.h"

#include "ofxTimeMeasurements.h"

void testApp::setup(){

	ofBackground(22);
	ofEnableAlphaBlending();
	ofDisableArbTex(); //POW2 textueres, GL_TEXTURE_2D!
	//ofEnableArbTex();

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_DISABLE_AVERAGE();
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);

	myTex = new ofTexture();
	progressiveTextureLoader.setup(myTex, CV_INTER_AREA);
	ofAddListener(progressiveTextureLoader.textureReady, this, &testApp::textureReady);

	//start loading the texture!
	TIME_SAMPLE_START_NOIF("Total Load Time");

	progressiveTextureLoader.loadTexture("8192px.jpg", true /*create mipmaps*/);

	plot = new ofxHistoryPlot( NULL, "frameTime", 400, false);
	plot->setRange(0, 16.66f * 2.0f);
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
	if(progressiveTextureLoader.isBusy()){
		plot->update(progressiveTextureLoader.getTimeSpentLastFrame());
	}
}


void testApp::textureReady(ofxProgressiveTextureLoad::textureEvent& arg){
	if (arg.loaded){
		ofLogNotice() << "textureReady!";
		TIME_SAMPLE_STOP_NOIF("Total Load Time");
	}else{
		ofLogError() << "texture load failed!" << arg.texturePath;
	}
}


void testApp::draw(){

	//texture
	ofSetColor(255);

	progressiveTextureLoader.draw(20, 50);

	//clock
	float s = 200;
	ofPushMatrix();
	ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
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
		progressiveTextureLoader.loadTexture("8192px.jpg", true /*create mipmaps*/);
	}
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