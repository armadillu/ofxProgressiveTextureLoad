#include "testApp.h"
#include "ofxGLError.h"

void testApp::setup(){

	ofDisableArbTex(); //POW2 textueres, GL_TEXTURE_2D!

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
	progressiveTextureLoader.loadTexture("crap8192.jpg", true);

	OFX_REMOTEUI_SERVER_LOAD_FROM_XML();

	plot = new ofxHistoryPlot( NULL, "frameTime", 600, false);
	plot->setRange(0, 16.6);
	plot->setColor( ofColor(255,0,0) );
	plot->setBackgroundColor(ofColor(0,220));
	plot->setShowNumericalInfo(true);
	plot->setRespectBorders(true);
	plot->setLineWidth(1);
	plot->setShowSmoothedCurve(true);
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



void testApp::draw(){


	//texture
	ofSetColor(255);


	progressiveTextureLoader.draw();


	//clock
	ofPushMatrix();
	ofTranslate(50, 200);
	ofSetColor(0);
	ofCircle(0, 0, 50);
	ofRotate(ofGetFrameNum() * 3.0, 0, 0, 1);
	ofSetColor(255);
	ofRect(0, 0, 50, 4);
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