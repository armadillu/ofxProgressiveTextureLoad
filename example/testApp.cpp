#include "testApp.h"
#include "ofxGLError.h"

void testApp::setup(){

	ofBackground(22);
	ofEnableAlphaBlending();
	ofDisableArbTex(); //POW2 textueres, GL_TEXTURE_2D!

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_DISABLE_AVERAGE();
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);

	OFX_REMOTEUI_SERVER_SETUP(); 	//start server
	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("DEBUG");

	myTex = new ofTexture();
	progressiveTextureLoader.setup(myTex);
	TS_START_NIF("Total Load Time");
	progressiveTextureLoader.loadTexture("crap8192.jpg", true);
	ofAddListener(progressiveTextureLoader.textureReady, this, &testApp::textureReady);

	OFX_REMOTEUI_SERVER_LOAD_FROM_XML();

	plot = new ofxHistoryPlot( NULL, "frameTime", 400, false);
	plot->setRange(0, 16.6f * 2.0f);
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
		float up = TIME_SAMPLE_GET_LAST_DURATION("update()");
		float draw = TIME_SAMPLE_GET_LAST_DURATION("update()");
		plot->update(up+draw);
	}
}

void testApp::textureReady(ofxProgressiveTextureLoad::textureEvent& arg){
	cout << "textureReady!" << endl;
	TS_STOP_NIF("Total Load Time");
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