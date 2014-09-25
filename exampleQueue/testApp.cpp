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

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_DISABLE_AVERAGE();
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);

	ProgressiveTextureLoadQueue * q = ProgressiveTextureLoadQueue::instance();

	q->setTexLodBias(-0.5);
	q->setScanlinesPerLoop(256);
	q->setTargetTimePerFrame(1.0);
	q->setNumberSimultaneousLoads(3);

	for(int i = 0; i < 10; i++){
		ofTexture* t = new ofTexture(); //create your own texture, it will be cleared so be sure its empty
		textures.push_back(t);
		ready[t] = false;

		ofxProgressiveTextureLoad * loader = q->loadTexture(imgName,
															t,
															true/*MIP-MAPS!*/,
															CV_INTER_AREA);
		ofAddListener(loader->textureReady, this, &testApp::textureReady);
		ofAddListener(loader->textureDrawable, this, &testApp::textureDrawable);
	}

}


void testApp::update(){

}

void testApp::textureDrawable(ofxProgressiveTextureLoad::textureEvent& arg){
	//ofLogNotice() << "texture Drawable!";
}

void testApp::textureReady(ofxProgressiveTextureLoad::textureEvent& arg){
	if (arg.loaded){
		//ofLogNotice() << "textureReady!";
		//TIME_SAMPLE_STOP_NOIF("Total Load Time");
		ready[arg.tex] = true;
	}else{
		ofLogError() << "texture load failed!" << arg.texturePath;
	}
}


void testApp::draw(){

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

			textures[i]->draw(x, 0, w * s, h * s);
			x += w * s;
			ofDrawBitmapString(ofToString(i), x + 10, 22);
		}
	}

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