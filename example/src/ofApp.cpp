#include "ofApp.h"

#include "Poco/Base64Encoder.h"

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
	
	mFbo->begin();
	auto rect = ofGetCurrentViewport();
	ofClear(0);

	auto result = mCloudVision->getResult();
	ofVec2f texurePosition;
	if (tex.isAllocated() && result)
	{
		ofPushMatrix();
		texurePosition = ofVec2f(rect.width - result->width, rect.height - result->height) * 0.5f;
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
		vbo.draw(GL_TRIANGLE_FAN, 0, vertices.size());
		ofPopMatrix();
		ofPopStyle();
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

