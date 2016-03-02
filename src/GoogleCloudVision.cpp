#pragma once

#include "GoogleCloudVision.h"
#include "Poco/Base64Encoder.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/URI.h"

using namespace Poco;
using namespace Poco::Net;

namespace google
{
	CloudVision::CloudVision(string key)
		:GOOGLE_BROWSER_KEY(key)
	{
		SharedPtr<PrivateKeyPassphraseHandler> pConsoleHandler = new KeyConsoleHandler(false);
		SharedPtr<InvalidCertificateHandler> pInvalidCertHandler = new ConsoleCertificateHandler(true);
		Context::Ptr pContext = new Context(Context::CLIENT_USE, "", Context::VERIFY_NONE);
		SSLManager::instance().initializeClient(pConsoleHandler, pInvalidCertHandler, pContext);

		startThread();
	}

	CloudVision::~CloudVision()
	{
		stop();
		waitForThread(false);
	}

	void CloudVision::pushPixels(const ofPixels& pix)
	{
		std::unique_lock<std::mutex> lck(mutex);
		pixelQueue.emplace_back(pix);
	}

	std::shared_ptr<CloudVisionResponse> CloudVision::getResult()
	{
		return mResponse;
	}

	void CloudVision::stop()
	{
		std::unique_lock<std::mutex> lck(mutex);
		stopThread();
		condition.notify_all();
	}

	void CloudVision::threadedFunction() 
	{
		while (isThreadRunning())
		{
			if (pixelQueue.size() == 0)
				continue;

			// check img size
			size_t img_width, img_height;
			if (pixelQueue.front().getWidth() > 640 || pixelQueue.front().getHeight() > 480)
			{
				float w = 640;
				float h = (float)pixelQueue.front().getHeight() * w / pixelQueue.front().getWidth();
				if (h > 480)
				{
					h = 480;
					w = (float)pixelQueue.front().getWidth() * h / pixelQueue.front().getHeight();
				}
				ofPixels dst;
				dst.allocate(w, h, OF_IMAGE_COLOR);
				pixelQueue.front().resizeTo(dst, OF_INTERPOLATE_BICUBIC);
				std::swap(pixelQueue.front(), dst);
			}
			img_width = pixelQueue.front().getWidth();
			img_height = pixelQueue.front().getHeight();

			mResponse.reset();

			ofBuffer buffer;
			{
				std::unique_lock<std::mutex> lck(mutex);
				ofSaveImage(pixelQueue.front(), buffer);
				pixelQueue.pop_front();
			}
			string img_encoded = toBase64(buffer);


			string type = "LABEL_DETECTION"; // LABEL_DETECTION, TEXT_DETECTION, FACE_DETECTION, LANDMARK_DETECTION, LOGO_DETECTION, SAFE_SEARCH_DETECTION, IMAGE_PROPERTIES
			size_t maxResults = 3;

			map<string, size_t> types;
			types["LABEL_DETECTION"] = 3;
			types["TEXT_DETECTION"] = 3;
			types["FACE_DETECTION"] = 3;
			types["LANDMARK_DETECTION"] = 3;
			types["LOGO_DETECTION"] = 3;
#if 1
			string json_string = R"({"requests":[{"image":{"content":")" + img_encoded;
			json_string += ofVAArgsToString(R"("},"features":[)");
			for (auto& it = types.begin(); it != types.end();)
			{
				json_string += ofVAArgsToString(R"({"type":"%s","maxResults":%u})", it->first.c_str(), it->second);
				it++;
				if (it != types.end())
					json_string += ",";
			}
			json_string += ofVAArgsToString(R"(]}]})");
#else
			string json_string = 
R"({
	"requests":[
		{
			"image":{
				"content":")" + img_encoded + R"(")" + 
R"(
			},
			"features":[)";
			for (auto& it = types.begin(); it != types.end();)
			{
				json_string += ofVAArgsToString(
R"(
				{
					"type":"%s",
					"maxResults":%u
				})", it->first.c_str(), it->second);
				it++;
				if (it != types.end())
					json_string += ",";
			}
			json_string += ofVAArgsToString(
R"(
			]
		}
	]
})");
#endif
			ofBufferToFile("request.json", ofBuffer(json_string));


			string url = GOOGLE_VISION_API + "images:annotate";
			url += ofVAArgsToString("?key=%s", GOOGLE_BROWSER_KEY.c_str());
			auto& response = postData(url, ofBuffer(json_string), "application/json");
			printf("[ Google Cloud Vision ]\nstatus: %i\nerror: %s\n", response.status, response.error.c_str());
			ofBufferToFile("result.json", response.data);


			auto getBoundingPolyVertices = [](const ofJson& json, vector<ofVec2f>& container) 
			{
				if (json.find("boundingPoly") == json.end())
					return;
				for (auto& vt : json["boundingPoly"]["vertices"])
				{
					float x = vt.value("x", 0.0f);
					float y = vt.value("y", 0.0f);
					container.emplace_back(x, y);
				}
			};

			auto getLocations = [](const ofJson& json, vector<latLng>& container)
			{
				if (json.find("locations") == json.end())
					return;
				for (auto& vt : json["locations"])
				{
					latLng ll;
					ll.latitude = vt["latLng"].value("latitude", 0.0);
					ll.longitude = vt["latLng"].value("longitude", 0.0);
					container.emplace_back(ll);
				}
			};

			auto& documrnt = ofJson::parse(response.data.getText());
			for (auto& jsonResponse : documrnt["responses"])
			{
				CloudVisionResponse res;
				res.width = img_width;
				res.height = img_height;
				for (auto& jsonLabel : jsonResponse["labelAnnotations"])
				{
					LabelAnnotation label;
					label.mid = jsonLabel.value("mid", "");
					label.description = jsonLabel.value("description", "");
					label.score = jsonLabel.value("score", 0.0f);
					res.labelAnnotations.emplace_back(label);
				}
				for (auto& jsonText : jsonResponse["textAnnotations"])
				{
					TextAnnotation text;
					text.locale = jsonText.value("locale", "");
					text.description = jsonText.value("description", "");
					getBoundingPolyVertices(jsonText, text.boundingPoly.vertices);
					res.textAnnotations.emplace_back(text);
				}
				for (auto& jsonLogo : jsonResponse["logoAnnotations"])
				{
					LogoAnnotation logo;
					logo.mid = jsonLogo.value("mid", "");
					logo.description = jsonLogo.value("description", "");
					logo.score = jsonLogo.value("score", 0.0f);
					getBoundingPolyVertices(jsonLogo, logo.boundingPoly.vertices);
					res.logoAnnotations.emplace_back(logo);
				}
				for (auto& jsonLandmark : jsonResponse["landmarkAnnotations"])
				{
					LandmarkAnnotation landmark;
					landmark.mid = jsonLandmark.value("mid", "");
					landmark.description = jsonLandmark.value("description", "");
					landmark.score = jsonLandmark.value("score", 0.0f);
					getBoundingPolyVertices(jsonLandmark, landmark.boundingPoly.vertices);
					getLocations(jsonLandmark, landmark.locations);
					res.landmarkAnnotations.emplace_back(landmark);
				}
				mResponse = make_shared<CloudVisionResponse>(res);
			}
		}
	}

	std::string CloudVision::toBase64(const std::string &source)
	{
		std::istringstream in(source);
		std::ostringstream out;
		Poco::Base64Encoder b64out(out);

		std::copy(std::istreambuf_iterator<char>(in),
			std::istreambuf_iterator<char>(),
			std::ostreambuf_iterator<char>(b64out));
		b64out.close(); // always call this at the end!

		return out.str();
	}

	ofHttpResponse CloudVision::postData(string url, const ofBuffer& data, string contentType)
	{
		ofHttpResponse response;
		try {
			URI uri(url.c_str());
			std::string path(uri.getPathAndQuery());
			if (path.empty()) path = "/";

			HTTPRequest req(HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1);

			if (contentType != "") {
				req.setContentType(contentType);
			}

			req.setContentLength(data.size());

			HTTPResponse res;
			shared_ptr<HTTPSession> session;
			istream * rs;
			if (uri.getScheme() == "https") {
				HTTPSClientSession * httpsSession = new HTTPSClientSession(uri.getHost(), uri.getPort());//,context);
				httpsSession->setTimeout(Poco::Timespan(20, 0));
				httpsSession->sendRequest(req) << data;
				rs = &httpsSession->receiveResponse(res);
				session = shared_ptr<HTTPSession>(httpsSession);
			}
			else {
				HTTPClientSession * httpSession = new HTTPClientSession(uri.getHost(), uri.getPort());
				httpSession->setTimeout(Poco::Timespan(20, 0));
				httpSession->sendRequest(req) << data;
				rs = &httpSession->receiveResponse(res);
				session = shared_ptr<HTTPSession>(httpSession);
			}

			response.status = res.getStatus();
			response.data.set(*rs);
			response.error = res.getReason();

			if (response.status >= 300 && response.status<400) {
				Poco::URI uri(req.getURI());
				uri.resolve(res.get("Location"));
			}
		}
		catch (Exception& exc) {

			ofLogError("CloudVision") << "CloudVision error postData --";
			
			// for now print error, need to broadcast a response
			ofLogError("CloudVision") << exc.displayText();
			response.status = -1;
			response.error = exc.displayText();

		}
		return response;
	}

	
}