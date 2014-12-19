#include "testApp.h"
#include "ofxGLError.h"

#include "ofxTimeMeasurements.h"


void testApp::setup(){

	imageNames.push_back("huge_a.png"); //rgba , 4channels
	imageNames.push_back("huge.jpg");	//rgb, 3 channels
	imageNames.push_back("huge_bw.jpg");//luminance, 1 channel

	ofBackground(33);
	ofEnableAlphaBlending();
	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	ofDisableArbTex(); //POW2 textueres, GL_TEXTURE_2D!
	//ofEnableArbTex();

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_DISABLE_AVERAGE();
	TIME_SAMPLE_GET_INSTANCE()->setIdleTimeColorFadePercent(0.3);
	TIME_SAMPLE_SET_REMOVE_EXPIRED_THREADS(false);
	ofxTimeMeasurements::instance()->setDeadThreadTimeDecay(0.99);
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);

	ProgressiveTextureLoadQueue * q = ProgressiveTextureLoadQueue::instance();

	q->setTexLodBias(-0.5); //negative gives you lower mipmaps >> sharper
	q->setScanlinesPerLoop(32);
	q->setTargetTimePerFrame(3);
	q->setNumberSimultaneousLoads(1);
	q->setVerbose(true);

	for(int i = 0; i < 3; i++){
		ofTexture* t = new ofTexture(); //create your own texture to got data loaded into; it will be cleared!
		textures.push_back(t);
		ready[t] = false;
		ofxProgressiveTextureLoad * loader = q->loadTexture(imageNames[i%imageNames.size()],
															t,					/*tex to load into*/
															true,				/*MIP-MAPS!*/
															CV_INTER_AREA);		/*Resize Quality*/
		ofAddListener(loader->textureReady, this, &testApp::textureReady);
		ofAddListener(loader->textureDrawable, this, &testApp::textureDrawable);
	}
}


void testApp::update(){

	ProgressiveTextureLoadQueue::instance()->update();

}


void testApp::textureDrawable(ofxProgressiveTextureLoad::textureEvent& arg){
	ofLogNotice() << "texture Drawable!";
}


void testApp::textureReady(ofxProgressiveTextureLoad::textureEvent& arg){
	if (arg.ok){
		//ofLogNotice() << "textureReady!";
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

	//see if GL is happy
	ofxGLError::draw(20,ofGetHeight() - 104);

	ProgressiveTextureLoadQueue::instance()->draw(20, ofGetHeight() - 80);
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