#include "ofMain.h"

namespace google
{
	struct latLng
	{
		double latitude;
		double longitude;
	};

	struct Landmark
	{
		string type;
		ofVec3f position;
	};

	struct LabelAnnotation
	{
		string mid;
		string description;
		float score;
	};

	struct TextAnnotation
	{
		string locale;
		string description;
		struct {
			std::vector<ofVec2f> vertices;
		} boundingPoly;
	};

	struct LogoAnnotation
	{
		string mid;
		string description;
		float score;
		struct {
			std::vector<ofVec2f> vertices;
		} boundingPoly;
	};

	struct LandmarkAnnotation
	{
		string mid;
		string description;
		float score;
		struct {
			std::vector<ofVec2f> vertices;
		} boundingPoly;
		std::vector<latLng> locations;
	};

	struct FaceAnnotation
	{
		struct {
			std::vector<ofVec2f> vertices;
		} boundingPoly;
		struct {
			std::vector<ofVec2f> vertices;
		} fdBoundingPoly;
		std::vector<Landmark> landmarks;
		float rollAngle;
		float panAngle;
		float tiltAngle;
		float detectionConfidence;
		float landmarkingConfidence;
		string joyLikelihood;
		string sorrowLikelihood;
		string angerLikelihood;
		string surpriseLikelihood;
		string underExposedLikelihood;
		string blurredLikelihood;
		string headwearLikelihood;
	};

	struct CloudVisionResponse
	{
		size_t width = 0;
		size_t height = 0;
		std::vector<LabelAnnotation> labelAnnotations;
		std::vector<TextAnnotation> textAnnotations;
		std::vector<LogoAnnotation> logoAnnotations;
		std::vector<LandmarkAnnotation> landmarkAnnotations;
		std::vector<FaceAnnotation> faceAnnotations;
	};


	typedef std::shared_ptr<class CloudVision> CloudVisionRef;

	class CloudVision : private ofThread
	{
	public:
		static CloudVisionRef create(string key)
		{
			return CloudVisionRef(new CloudVision(key));
		}
		~CloudVision();
		void pushPixels(const ofPixels& pix);
		std::shared_ptr<CloudVisionResponse> getResult();
		void stop();

	protected:
		CloudVision(string key);
		void threadedFunction();
		ofHttpResponse postData(string url, const ofBuffer& data, string contentType);
		std::string toBase64(const std::string &source);

	private:
		const string GOOGLE_VISION_API = "https://vision.googleapis.com/v1/";
		string GOOGLE_BROWSER_KEY = "";
		
		std::condition_variable condition;
		std::deque<ofPixels> pixelQueue;
		std::shared_ptr<CloudVisionResponse> mResponse;
	};
}