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
	if(numSimlutaneousLoads < 1) numSimlutaneousLoads = 1; //safe dumb developers from themselves
}

void ProgressiveTextureLoadQueue::setScanlinesPerLoop(int numLines){
	numLinesPerLoop = numLines;
	if(numLinesPerLoop < 1) numLinesPerLoop = 1; //safe dumb developers from themselves
}

void ProgressiveTextureLoadQueue::update(ofEventArgs & args){

	vector<int> toDelete;

	//see who is finished
	for(int i = 0; i < current.size(); i++){
		if(!current[i].loader->isBusy()){ //must have finished loading! time to start next one!
			toDelete.push_back(i);
		}
	}

	//dealloc and remove from current all the finished ones
	for(int i = toDelete.size()-1; i >= 0 ; i--){
		delete current[toDelete[i]].loader;
		//ofLogNotice() << "delete loader " << current[toDelete[i]].ID;
		current.erase(current.begin() + toDelete[i]);
	}

	//is there stuff to do?
	//stagger a bit jobs across frames too with %3
	if(current.size() < numSimlutaneousLoads && (ofGetFrameNum()%3 == 1)){
		if(pending.size()){
			current.push_back(pending[0]);
			pending.erase(pending.begin());
			int indx = current.size()-1;
			current[indx].loader->loadTexture(current[indx].path, current[indx].withMipMaps);
			//ofLogNotice() << "load texture " << current[indx].ID;
		}
	}
}


void ProgressiveTextureLoadQueue::draw(int x, int y){

	ofDrawBitmapStringHighlight("ProgressiveTextureLoadQueue\nbusy: " + string(current.size() ? "YES" : "NO" )+
								"\npending: " + ofToString(pending.size()),
								x, y);
}

