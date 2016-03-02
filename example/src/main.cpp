#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
#if 1
	ofGLWindowSettings settings;
	settings.setGLVersion(4, 3);
	settings.windowMode = OF_WINDOW;
	ofCreateWindow(settings);
#else
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context
#endif
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
