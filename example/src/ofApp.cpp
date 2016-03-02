#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowShape(WIDTH, HEIGHT);
	ofSetWindowPosition((ofGetScreenWidth() - ofGetWidth()) / 2, (ofGetScreenHeight() - ofGetHeight()) / 2);
	ofSetFrameRate(60);
	ofDisableArbTex();
	//ofSetVerticalSync(true);
    
    {
        ofFbo::Settings s;
        s.width = s.height = FBO_SIZE;
        s.useDepth = true;
        s.colorFormats.emplace_back(GL_RGBA);
        
        mFbo = shared_ptr<ofFbo>(new ofFbo);
        mFbo->allocate(s);
    }
    
    {
        mUniforms.setName("Uniforms");
        mUniforms.add(uElapsedTime.set("uElapsedTime", ofGetElapsedTimef()));
        
        mGui = shared_ptr<ofxGuiGroup>(new ofxGuiGroup);
        mGui->setup("GUI");
        mGui->add(mUniforms);
    }
		
	auto b = ofBufferFromFile("setting.json");
	auto d = ofJson::parse(b.getText());
	string key = d.value("BROWSER_KEY", "");

	mCloudVision = google::CloudVision::create(key);

	printf("");
}

//--------------------------------------------------------------
void ofApp::update(){
	ofSetWindowTitle("oF Application: " + ofToString(ofGetFrameRate(), 1));
	uElapsedTime = ofGetElapsedTimef();
	

	if (!inputString.empty())
	{
		auto& res = ofLoadURL(inputString);
		ofImage img;
		if (img.load(res.data))
		{
			printf("get image from %s\n", inputString.c_str());
			mCloudVision->pushPixels(img.getPixels());

			auto& pix = img.getPixels();
			tex.allocate(pix.getWidth(), pix.getHeight(), GL_RGB);
			tex.loadData(pix);
		}

		inputString = "";
	}


	mFbo->begin();
	auto rect = ofGetCurrentViewport();
	ofClear(0);

	auto result = mCloudVision->getResult();
	ofVec2f texurePosition;
	if (tex.isAllocated() && result)
	{
		ofPushMatrix();
		texurePosition = ofVec2f(rect.width - result->width, (rect.height - result->height) * 0.5f);
		tex.draw(texurePosition, result->width, result->height);
		ofPopMatrix();
	}

	auto drawBoundingPoly = [](ofVec2f pos, vector<ofVec2f>& vertices)
	{
		ofPushStyle();
		ofNoFill();
		ofPushMatrix();
		ofTranslate(pos);
		ofVbo vbo;
		vbo.setVertexData(&vertices[0].x, 2, vertices.size(), GL_STATIC_DRAW);
		vbo.draw(GL_LINE_LOOP, 0, vertices.size());
		ofPopMatrix();
		ofPopStyle();
	};

	auto drawFaceLandmarks = [](ofVec2f pos, vector<google::Landmark> landmarks)
	{
#if 0
		vector<ofVec3f> vertices;
		for (auto& lm : landmarks)
			vertices.emplace_back(lm.position);
		ofPushMatrix();
		ofTranslate(pos);
		ofVbo vbo;
		vbo.setVertexData(&vertices[0].x, 3, vertices.size(), GL_STATIC_DRAW);
		vbo.draw(GL_POINTS, 0, vertices.size());
		ofPopMatrix();
#else
		ofPushMatrix();
		ofTranslate(pos);
		for (auto& lm : landmarks)
			ofDrawCircle(lm.position, 1);
		ofPopMatrix();
#endif
	};
	
	if (result)
	{
		string text;
		ofVec2f textPosition(20, (rect.height - ofGetHeight()) / 2 + 20);
		ofVec2f textOffset(200, 0);
		if (result->labelAnnotations.size() > 0)
		{
			text = "[Label Annotations]\n";
			for (auto& annotation : result->labelAnnotations)
			{
				text += "-------------------\n";
				text += ofVAArgsToString("mid: %s\n", annotation.mid.c_str());
				text += ofVAArgsToString("description: %s\n", annotation.description.c_str());
				text += ofVAArgsToString("score: %f\n", annotation.score);
			}
			ofDrawBitmapString(text, textPosition);
			textPosition += textOffset;
		}
		if (result->textAnnotations.size() > 0)
		{
			text = "[Text Annotations]\n";
			for (auto& annotation : result->textAnnotations)
			{
				text += "-------------------\n";
				text += ofVAArgsToString("locale: %s\n", annotation.locale.c_str());
				text += ofVAArgsToString("description: %s\n", annotation.description.c_str());

				drawBoundingPoly(texurePosition, annotation.boundingPoly.vertices);
			}
			ofDrawBitmapString(text, textPosition);
			textPosition += textOffset;
		}
		if (result->logoAnnotations.size() > 0)
		{
			text = "[Logo Annotations]\n";
			for (auto& annotation : result->logoAnnotations)
			{
				text += "-------------------\n";
				text += ofVAArgsToString("mid: %s\n", annotation.mid.c_str());
				text += ofVAArgsToString("description: %s\n", annotation.description.c_str());
				text += ofVAArgsToString("score: %f\n", annotation.score);

				drawBoundingPoly(texurePosition, annotation.boundingPoly.vertices);
			}
			ofDrawBitmapString(text, textPosition);
			textPosition += textOffset;
		}
		if (result->landmarkAnnotations.size() > 0)
		{
			text = "[Landmark Annotations]\n";
			for (auto& annotation : result->landmarkAnnotations)
			{
				text += "-------------------\n";
				text += ofVAArgsToString("mid: %s\n", annotation.mid.c_str());
				text += ofVAArgsToString("description: %s\n", annotation.description.c_str());
				text += ofVAArgsToString("score: %f\n", annotation.score);
				for (auto& ll : annotation.locations)
					text += ofVAArgsToString("location: %3.3f, %3.3f\n", ll.latitude, ll.longitude);

				drawBoundingPoly(texurePosition, annotation.boundingPoly.vertices);
			}
			ofDrawBitmapString(text, textPosition);
			textPosition += textOffset;
		}
		if (result->faceAnnotations.size() > 0)
		{
			text = "[Face Annotations]\n";
			for (auto& annotation : result->faceAnnotations)
			{
				text += "-------------------\n";
				text += ofVAArgsToString("rollAngle: %f\n", annotation.rollAngle);
				text += ofVAArgsToString("panAngle: %f\n", annotation.panAngle);
				text += ofVAArgsToString("tiltAngle: %f\n", annotation.tiltAngle);
				text += ofVAArgsToString("detectionConfidence: %f\n", annotation.detectionConfidence);
				text += ofVAArgsToString("landmarkingConfidence: %f\n", annotation.landmarkingConfidence);
				text += ofVAArgsToString("joyLikelihood: %s\n", annotation.joyLikelihood.c_str());
				text += ofVAArgsToString("sorrowLikelihood: %s\n", annotation.sorrowLikelihood.c_str());
				text += ofVAArgsToString("angerLikelihood: %s\n", annotation.angerLikelihood.c_str());
				text += ofVAArgsToString("surpriseLikelihood: %s\n", annotation.surpriseLikelihood.c_str());
				text += ofVAArgsToString("underExposedLikelihood: %s\n", annotation.underExposedLikelihood.c_str());
				text += ofVAArgsToString("blurredLikelihood: %s\n", annotation.blurredLikelihood.c_str());
				text += ofVAArgsToString("headwearLikelihood: %s\n", annotation.headwearLikelihood.c_str());
				drawBoundingPoly(texurePosition, annotation.boundingPoly.vertices);
				drawBoundingPoly(texurePosition, annotation.fdBoundingPoly.vertices);
				drawFaceLandmarks(texurePosition, annotation.landmarks);
			}
			ofDrawBitmapString(text, textPosition);
			textPosition += textOffset;
		}
	}

	mFbo->end();
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofClear(0);

	mFbo->draw(0, (ofGetHeight() - ofGetWidth()) / 2, ofGetWidth(), ofGetWidth());
	

	if (bDebugVisible)
	{
		mGui->draw();
	}
		
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	auto toggleFullscreen = [&]()
	{
		ofToggleFullscreen();
		if (!(ofGetWindowMode() == OF_FULLSCREEN))
		{
			ofSetWindowShape(WIDTH, HEIGHT);
			auto pos = ofVec2f(ofGetScreenWidth() - WIDTH, ofGetScreenHeight() - HEIGHT) * 0.5f;
			ofSetWindowPosition(pos.x, pos.y);
		}
	};

	auto getClipboardText = []() ->string
	{
		// Try opening the clipboard
		if (!OpenClipboard(nullptr))
			return "";
		// Get handle of clipboard object for ANSI text
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (hData == nullptr)
			return "";
		// Lock the handle to get the actual text pointer
		char * pszText = static_cast<char*>(GlobalLock(hData));
		if (pszText == nullptr)
			return "";
		// Save text in a string class instance
		std::string text(pszText);
		// Release the lock
		GlobalUnlock(hData);
		// Release the clipboard
		CloseClipboard();
		return text;
	};
	
	switch (key)
	{
	case OF_KEY_F1:
		bDebugVisible = !bDebugVisible;
		break;
	case OF_KEY_F5:
		break;
	case OF_KEY_F11:
		toggleFullscreen();
		break;
	case 'v':
		inputString = getClipboardText();
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
	auto& file = dragInfo.files[0];

	ofImage img;
	if (!img.load(file))
		return;

	mCloudVision->pushPixels(img.getPixels());

	auto& pix = img.getPixels();
	tex.allocate(pix.getWidth(), pix.getHeight(), GL_RGB);
	tex.loadData(pix);
}

