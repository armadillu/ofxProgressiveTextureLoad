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

ProgressiveTextureLoadQueue* ProgressiveTextureLoadQueue::instance(){
	if (!singleton){   // Only allow one instance of class to be generated.
		singleton = new ProgressiveTextureLoadQueue();
	}
	return singleton;
}



ProgressiveTextureLoadQueue::ProgressiveTextureLoadQueue(){

	numLinesPerLoop = 16;
	maxTimeTakenPerFrame = 1.0; //ms
	texLodBias = -0.5;
	numSimlutaneousLoads = 2;
	ids = 0;
	verbose = false;

	ofAddListener(ofEvents().update, this, &ProgressiveTextureLoadQueue::update);
}


ofxProgressiveTextureLoad* ProgressiveTextureLoadQueue::loadTexture(string path, ofTexture* tex, bool createMipMaps,
											  int resizeQuality, bool highPriority){

	LoadRequest r;
	r.path = path;
	r.withMipMaps = createMipMaps;
	r.loader = new ofxProgressiveTextureLoad();
	r.loader->setVerbose(verbose);
	r.loader->setScanlinesPerLoop(numLinesPerLoop);
	r.loader->setTargetTimePerFrame(maxTimeTakenPerFrame);
	r.loader->setTexLodBias(texLodBias);
	r.loader->setup(tex, resizeQuality);
	r.ID = ids;
	ids++;

	if(highPriority)
		pending.insert(pending.begin(), r);
	else
		pending.push_back(r);

	return r.loader;
}

void ProgressiveTextureLoadQueue::setNumberSimultaneousLoads(int numThreads){
	numSimlutaneousLoads = numThreads;
	if(numSimlutaneousLoads < 1) numSimlutaneousLoads = 1; //save dumb developers from themselves
}

void ProgressiveTextureLoadQueue::setScanlinesPerLoop(int numLines){
	numLinesPerLoop = numLines;
	if(numLinesPerLoop < 1) numLinesPerLoop = 1; //save dumb developers from themselves
}

void ProgressiveTextureLoadQueue::setTargetTimePerFrame(float ms){
	maxTimeTakenPerFrame = ms;
	if(maxTimeTakenPerFrame < 0.01) maxTimeTakenPerFrame = 0.01; //save dumb developers from themselves
}

void ProgressiveTextureLoadQueue::update(ofEventArgs & args){

	vector<int> toDelete;

	//see who is finished
	int numUploadingTextures = 0;
	for(int i = 0; i < current.size(); i++){
		if(!current[i].loader->isBusy()){ //must have finished loading! time to start next one!
			toDelete.push_back(i);
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

	//dealloc and remove from current all the finished ones
	for(int i = toDelete.size()-1; i >= 0 ; i--){
		delete current[toDelete[i]].loader;
		//ofLogNotice() << "delete loader " << current[toDelete[i]].ID;
		current.erase(current.begin() + toDelete[i]);
	}

	//is there stuff to do?
	//stagger a bit jobs across frames too with %3
	if(current.size() < numSimlutaneousLoads){
		if(pending.size()){
			current.push_back(pending[0]);
			pending.erase(pending.begin());
			int indx = current.size()-1;
			current[indx].loader->loadTexture(current[indx].path, current[indx].withMipMaps);
			//ofLogNotice() << "load texture " << current[indx].ID;
		}
	}
}


int ProgressiveTextureLoadQueue::getNumBusy(){
	int n = 0;
	for(int i = 0; i < current.size(); i++){
		if(current[i].loader->isBusy()){ //must have finished loading! time to start next one!
			n++;
		}
	}
	return n;
}


void ProgressiveTextureLoadQueue::draw(int x, int y){

	string msg = "ProgressiveTextureLoadQueue max(" + ofToString(numSimlutaneousLoads) + 
	")\nbusy: " + string(current.size() ? "YES" : "NO" ) +
	"\npending: " + ofToString(pending.size());

	for(int i = 0 ; i < current.size(); i++){
		msg += "\n  Loader " + ofToString(i) + ": " + current[i].loader->getStateString();
	}
	ofDrawBitmapStringHighlight(msg, x, y);
}

