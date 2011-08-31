#pragma once

#include "ofxCv.h"

#define TRACKER_MAX_AGE (2)
#define TRACKER_MAX_DISTANCE (50.)

// if an object is > distance than a certain amount, add new id anyway

namespace ofxCv {
	
	using namespace cv;
	
	template <class T>
	class TrackedObject {
	protected:
		unsigned int age;
		unsigned int label;
	public:
		T object;
		
		TrackedObject(const T& object, unsigned int label)
		:object(object)
		,label(label)
		,age(0) {
		}
		TrackedObject(const T& object, const TrackedObject<T>& previous)
		:object(object)
		,label(previous.label)
		,age(0) {
		}
		void timeStep() {
			age++;
		}
		unsigned int getAge() const {
			return age;
		}
		unsigned int getLabel() const {
			return label;
		}
	};
	
	struct bySecond {
		template <class First, class Second>
		bool operator()(pair<First, Second> const &a, pair<First, Second> const &b) { 
			return a.second < b.second;
		}
	};
	
	struct isOld {
		template <class T>
		bool operator()(TrackedObject<T> const &object) { 
			return object.getAge() > TRACKER_MAX_AGE;
		}
	};
	
	template <class T>
	class Tracker {
	protected:
	
		typedef pair<int, int> MatchPair;
		typedef pair<MatchPair, float> MatchDistancePair;
		
		vector<TrackedObject<T> > previous;
		vector<unsigned int> labels;
		
		unsigned int curLabel;
		unsigned int getNewLabel() {
			return curLabel++;
		}
				
	public:
		Tracker<T>()
		:curLabel(0) {
		}
		vector<unsigned int>& track(const vector<T>& objects);
		vector<unsigned int>& getLabels() {
			return labels;
		}
	};
	
	template <class T>
	vector<unsigned int>& Tracker<T>::track(const vector<T>& objects) {
		ofRemove(previous, isOld());
		
		int n = objects.size();
		int m = previous.size();
		int nm = n * m;
		
		cout << previous.size() << " previous: ";
		for(int j = 0; j < m; j++) {
			cout << "(" << previous[j].object.x << "," << previous[j].object.y << "@" << previous[j].getAge() << ")";
		}
		cout << endl;
		cout << previous.size() << " objects: ";
		for(int i = 0; i < n; i++) {
			cout << "(" << objects[i].x << "," << objects[i].y << ")";
		}
		cout << endl;
		
		// build NxM distance matrix
		vector<MatchDistancePair> all(nm);
		int k = 0;
		for(int i = 0; i < n; i++) {
			for(int j = 0; j < m; j++) {
				all[k].first = MatchPair(i, j);
				all[k].second = trackingDistance(objects[i], previous[j].object);
				k++;
			}
		}
		
		// sort all possible matches by distance
		sort(all.begin(), all.end(), bySecond());
		
		cout << "sorted matches: ";
		for(k = 0; k < nm; k++) {
			MatchPair& match = all[k].first;
			float distance = all[k].second;
			int i = match.first;
			int j = match.second;
			cout << "(" << objects[i].x << "," << objects[i].y << ")~";
			cout << "(" << previous[j].object.x << "," << previous[j].object.y << "@" << previous[j].getAge() << "/" << previous[j].getLabel() << ")";
			cout << "=" << distance << " ";
		}
		cout << endl;
		
		labels.clear();
		labels.resize(n);
		vector<TrackedObject<T> > current;
		vector<bool> matchedObjects(n, false);
		vector<bool> matchedPrevious(m, false);
		// walk through matches in order
		for(k = 0; k < nm && all[k].second < TRACKER_MAX_DISTANCE; k++) {
			MatchPair& match = all[k].first;
			int i = match.first;
			int j = match.second;
			// only use match if both objects are unmatched (age is reset to 0)
			if(!matchedObjects[i] && !matchedPrevious[j]) {
				matchedObjects[i] = true;
				matchedPrevious[j] = true;
				TrackedObject<T> updatedTrackedObject(objects[i], previous[j]);
				current.push_back(updatedTrackedObject);
				labels[i] = current.back().getLabel();
				cout << "added obejct at " << i << "," << j << ": (" << updatedTrackedObject.object.x << "," << updatedTrackedObject.object.y << ")" << endl;
			}
		}
		cout << "quit after " << k << "/" << nm << " checks" << endl;
		
		// create new labels for new unmatched objects (age is 0)
		for(int i = 0; i < n; i++) {
			if(!matchedObjects[i]) {
				int curLabel = getNewLabel();
				current.push_back(TrackedObject<T>(objects[i], curLabel));
				labels[i] = curLabel;
			}
		}
		
		// copy old unmatched objects (age is increased)
		for(int j = 0; j < m; j++) {
			if(!matchedPrevious[j]) {
				current.push_back(previous[j]);
				current.back().timeStep();
			}
		}
		
		cout << objects.size() << " new: ";
		for(int i = 0; i < n; i++) {
			cout << labels[i] << "(" << objects[i].x << "," << objects[i].y << ")/";
			for(int j = 0; j < m; j++) {
				if(labels[i] == previous[j].getLabel()) {
				cout << "(" << previous[j].object.x << "," << previous[j].object.y << "@" << previous[j].getAge() << ")";
				}
			}
			cout << " ";
		}
		cout << endl;
		cout << endl;
		
		previous = current;
		return labels;
	}
	
	float trackingDistance(const cv::Rect& a, const cv::Rect& b);
	float trackingDistance(const cv::Point2f& a, const cv::Point2f& b);
	
	typedef Tracker<cv::Point2f> PointTracker;
	typedef Tracker<cv::Rect> RectTracker;
	
}