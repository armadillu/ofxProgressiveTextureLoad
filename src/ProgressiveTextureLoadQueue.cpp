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

	ofAddListener(ofEvents().update, this, &ProgressiveTextureLoadQueue::update, OF_EVENT_ORDER_BEFORE_APP);
}


ofxProgressiveTextureLoad* ProgressiveTextureLoadQueue::loadTexture(string path, ofTexture* tex, bool createMipMaps,
											  int resizeQuality, bool highPriority){

	LoadRequest r;
	r.path = path;
	r.withMipMaps = createMipMaps;
	r.loader = new ofxProgressiveTextureLoad();
	r.loader->setVerbose(false);
	r.loader->setScanlinesPerLoop(numLinesPerLoop);
	r.loader->setTargetTimePerFrame(maxTimeTakenPerFrame);
	r.loader->setTexLodBias(texLodBias);
	r.loader->setup(tex, resizeQuality);

	if(highPriority)
		pending.insert(pending.begin(), r);
	else
		pending.push_back(r);

	return r.loader;

}


void ProgressiveTextureLoadQueue::update(ofEventArgs & args){

	if (current.size()){
		if(!current[0].loader->isBusy()){ //must have finished loading! time to start next one!
			delete current[0].loader;
			current.clear();
		}
	}
	if(current.size() == 0){ //not doing anything, is there stuff to do?
		if(pending.size()){
			current.push_back(pending[0]);
			pending.erase(pending.begin());
			current[0].loader->loadTexture(current[0].path, current[0].withMipMaps);
		}
	}

}
void ProgressiveTextureLoadQueue::draw(int x, int y){

	ofDrawBitmapStringHighlight("ProgressiveTextureLoadQueue\nbusy: " + string(current.size() ? "YES" : "NO" )+
								"\npending: " + ofToString(pending.size()),
								x, y);
}

