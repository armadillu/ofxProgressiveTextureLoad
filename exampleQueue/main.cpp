#include "ofMain.h"
#include "testApp.h"
#include "ofAppGLFWWindow.h"
#include "ofxTimeMeasurements.h"


//========================================================================
int main( ){
//	ofAppGLFWWindow win;
//	win.setNumSamples(8);
	//win.setOrientation(OF_ORIENTATION_90_LEFT);
	//win.setMultiDisplayFullscreen(true);
	//win.set

	ofSetupOpenGL(1800,400, OF_WINDOW);	// <-------- setup the GL context

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_DISABLE_AVERAGE();
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new testApp());

}
