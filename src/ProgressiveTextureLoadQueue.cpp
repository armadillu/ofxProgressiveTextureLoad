//
//  ProgressiveTextureLoadQueue.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 23/09/14.
//
//

#include "ofMain.h"

#include "ProgressiveTextureLoadQueue.h"


ProgressiveTextureLoadQueue* ProgressiveTextureLoadQueue::singleton = NULL;



ProgressiveTextureLoadQueue::ProgressiveTextureLoadQueue(){

	numLinesPerLoop = 16;
	maxTimeTakenPerFrame = 1.0; //ms
	texLodBias = -0.5;
	maxRequestsPerFrame = 10;
	maxSimlutaneousThreads = 2;
	ids = 0;
	verbose = false;
	ofAddListener(ofEvents().update, this, &ProgressiveTextureLoadQueue::update, OF_EVENT_ORDER_BEFORE_APP);
}

ofxProgressiveTextureLoad*
ProgressiveTextureLoadQueue::loadTexture(string path,
										 ofTexture* tex,
										 bool createMipMaps,
										 bool ARB,
										 int resizeQuality,
										 bool highPriority){

	LoadRequest r;
	r.path = path;
	r.withMipMaps = createMipMaps;
	r.loader = new ofxProgressiveTextureLoad();
	r.loader->setWillBeUsedOnlyOnce(true);
	r.loader->setVerbose(verbose);
	r.loader->setScanlinesPerLoop(numLinesPerLoop);
	r.loader->setTargetTimePerFrame(maxTimeTakenPerFrame);
	r.loader->setTexLodBias(texLodBias);
	r.loader->setup(tex, resizeQuality, ARB);
	r.ID = ids;
	ids++;

	if(highPriority)
		pending.insert(pending.begin(), r);
	else
		pending.push_back(r);

	return r.loader;
}

void ProgressiveTextureLoadQueue::setMaxThreads(int numThreads){
	maxSimlutaneousThreads = numThreads;
	if(maxSimlutaneousThreads < 1) maxSimlutaneousThreads = 1; //save dumb developers from themselves
}

void ProgressiveTextureLoadQueue::setNumberSimultaneousLoads(int numThreads){
	setMaxThreads(numThreads);
}

void ProgressiveTextureLoadQueue::setScanlinesPerLoop(int numLines){
	numLinesPerLoop = numLines;
	if(numLinesPerLoop < 1) numLinesPerLoop = 1; //save dumb developers from themselves
}

void ProgressiveTextureLoadQueue::setTargetTimePerFrame(float ms){
	maxTimeTakenPerFrame = ms;
	if(maxTimeTakenPerFrame < 0.01) maxTimeTakenPerFrame = 0.01; //save dumb developers from themselves
}

void ProgressiveTextureLoadQueue::update( ofEventArgs & args ){

	vector<int> indicesToDelete;

	//see who is finished
	int numUploadingTextures = 0;
	for(int i = 0; i < current.size(); i++){

		current[i].loader->update();

		if(current[i].loader->canBeDeleted()){ //must have finished loading! time to start next one!
			indicesToDelete.push_back(i);
		}
		if(current[i].loader->isUploadingTextures()){
			numUploadingTextures++;
		}
	}

	float timePerLoader = maxTimeTakenPerFrame;
	if (numUploadingTextures > 0){
		timePerLoader /= numUploadingTextures;
	}

	for(int i = 0; i < current.size(); i++){
		current[i].loader->setTargetTimePerFrame(timePerLoader);
	}

	//go though expired loaders and put them in a future delete list
	float t = ofGetElapsedTimef();
	for(int i = indicesToDelete.size()-1; i >= 0 ; i--){
		toDeleteSoon.push_back(make_pair(current[indicesToDelete[i]], t + 0.5)); //will delete in 0.1 seconds
		current.erase(current.begin() + indicesToDelete[i]);
	}

	//dealloc and remove stuff older loaders that have finished a while ago
	for(int i = toDeleteSoon.size()-1; i >= 0 ; i--){
		if(toDeleteSoon[i].second < t){
			try{
				delete toDeleteSoon[i].first.loader;
				toDeleteSoon[i].first.loader = NULL;
			}catch(...){}
			toDeleteSoon.erase(toDeleteSoon.begin() + i);
		}
	}
	
	//is there stuff to do?
	int c = 0;

	while(pending.size() && current.size() < maxSimlutaneousThreads && c < maxRequestsPerFrame ){
		if(pending[0].loader->hasBeenAskedToCancelLoad()){
			pending[0].loader->update(); //send the notification that we got canceled ok
			delete pending[0].loader;
			pending.erase(pending.begin()); //TODO this doesnt seem safe
			continue;
		}
		current.push_back(pending[0]);
		pending.erase(pending.begin());
		int indx = current.size()-1;
		current[indx].loader->loadTexture(current[indx].path, current[indx].withMipMaps);
		c++;
	}
}


int ProgressiveTextureLoadQueue::getNumBusy(){
	int n = 0;
	for(int i = 0; i < current.size(); i++){
		if(current[i].loader->isBusy()){ //must have finished loading! time to start next one!
			n++;
		}
	}
	for(int i = 0; i < pending.size(); i++){
		if(pending[i].loader->isBusy()){ //must have finished loading! time to start next one!
			n++;
		}
	}
	return n;
}

string ProgressiveTextureLoadQueue::getStatsAsText(){

	string msg = "ProgressiveTextureLoadQueue (" + ofToString(ofxProgressiveTextureLoad::getNumInstances()) +
		"/" + ofToString(maxSimlutaneousThreads) +
		")\nTotal Loaded: " + ofToString(ofxProgressiveTextureLoad::getNumMbLoaded(), 1) + "MB" +
		"\nPending: " + ofToString(pending.size());

	for (int i = 0; i < current.size(); i++) {
		msg += "\n   " + ofToString(i) + ": " + current[i].loader->getStateString();
	}
	return msg;
}

void ProgressiveTextureLoadQueue::draw(int x, int y){
	ofDrawBitmapStringHighlight(getStatsAsText(), x, y, ofColor::black, ofColor::limeGreen);
}

