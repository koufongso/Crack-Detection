#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <set>
#include <stack>

using namespace std;

#define A_MIN 10  // [pixel]

struct Pixel {
	Pixel(int i, int j) {
		this->i = i;
		this->j = j;
	}
	int i, j;
};


void Salembier(const cv::Mat, cv::Mat &,int);


class PatternSegmentation {
	public:
		PatternSegmentation(cv::Mat img,int kernel_size) {
			this->img_c = img.clone();
			this->img_mark = img.clone();
			cv::cvtColor(img, this->img, CV_BGR2GRAY);
			this->h = img.rows;
			this->w = img.cols;
			this->haveVisit = vector<bool>(h*w);

			for (int i = 0; i < h;i++){
				for(int j=0; j<w;j++)
					haveVisit[i * w + j] = false;
			}

			this->kernel_size = kernel_size;
			segmentation(kernel_size);
		}

		~PatternSegmentation() {
			clearPatterns();
		}

		int size() {
			return patternSet.size();
		}

		void segmentation(const int kernel_size) {
			clearPatterns();

			Salembier(this->img, this->img_s, kernel_size);
			cv::threshold(this->img_s, this->img_otsu, 0, 255, cv::THRESH_OTSU);
			
			for (int i = 0; i < h; i++) {
				for (int j = 0; j < w; j++) {
					hold.clear();
					cv::Scalar c = img_otsu.at<uchar>(i, j);
					if (c.val[0] == 255 && !haveVisit[i * w + j]) getPattern(i, j); // hit a new white pixel
					if (hold.size() > A_MIN)
						patternSet.insert(hold);
					else
						for (Pixel* p : hold) delete p;
				}
			}
			markPatterns();
		}

		void clearPatterns() {
			for (set<Pixel*> pattern : patternSet) {
				for (Pixel* p : pattern) {
					delete p;
				}
			}
			patternSet.clear();
		}


		void drawOrigin() {
			cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.
			imshow("Display window", img);                   // Show our image inside it.
			cv::waitKey();
		}

		void drawSalembier(){
			cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.
			imshow("Display window", img_s);                   // Show our image inside it.
			cv::waitKey();
		}

		void drawOtsu() {
			cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.
			imshow("Display window", img_otsu);                   // Show our image inside it.
			cv::waitKey();
		}

		void drawPatterns() {
			cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.
			imshow("Display window", img_mark);                   // Show our image inside it.
			cv::waitKey();
		}

		void savePatterns(string path) {
			cv::imwrite(path, img_mark);
		}



	private:
		cv::Mat img,img_c,img_mark,img_s, img_otsu;
		int h,w;
		int kernel_size;
		vector<bool> haveVisit;
		set<set<Pixel*>> patternSet;
		set<Pixel*> hold;
		stack<Pixel*> patternStack;

		/* recursion version , not working properlly (stack overflow)
		* 
		void segmentation() {
			for (int i = 0; i < h; i++) {
				for (int j = 0; j < w; j++) {
				cv::Scalar c = img_otsu.at<uchar>(i, j);
				if(c.val[0]==255 && !haveVisit[i*w+j]) visit(i, j);
				if (hold.size() > A_MIN) patternSet.insert(hold);
				for (Pixel* p : hold) {
					delete p;
				}
				hold.clear();
				}
			}
		}

		void visit(int i, int j) {
			if (i < 0 || i >= h || j < 0 || j >= w) return;
			if (haveVisit[i * w + j]) return;
			cv::Scalar c = img_otsu.at<uchar>(i, j);
			if (c.val[0] != 255) return;
			haveVisit[i * w + j] = true;
			hold.insert(new Pixel(i, j));
			visit(i-1, j); // top
			visit(i-1, j+1); // top-right
			visit(i, j+1); // right
			visit(i+1, j+1); // bot-right
			visit(i+1, j); // bot
			visit(i+1, j-1); // bot-left
			visit(i, j-1); // left
			visit(i-1, j-1); // top-lef
		}
		*/

		/* stack data structure version, working properlly
		*/


		void getPattern(int i, int j) {
			Pixel* current = new Pixel(i, j);
			haveVisit[i * w + j] = true;
			patternStack.push(current);
			hold.insert(current);
			while (!patternStack.empty()) {
				Pixel* next = patternStack.top();
				patternStack.pop();
				int ni = next->i;
				int nj = next->j;
				visit(ni-1, nj);		// top
				visit(ni-1, nj+1);	// top-right
				visit(ni, nj+1);		// right
				visit(ni+1, nj+1);	// bot-right
				visit(ni+1, nj);		// bot
				visit(ni+1, nj-1);	// bot-left
				visit(ni, nj-1);		// left
				visit(ni-1, nj-1);	// top-lef
			}
		}

		inline void visit(int i, int j) {
			if (i < 0 || i >= h || j < 0 || j >= w) return;
			if (haveVisit[i * w + j]) return;				
			cv::Scalar c = img_otsu.at<uchar>(i, j);
			if (c.val[0] != 255) return;
			
			haveVisit[i * w + j] = true;
			Pixel* current = new Pixel(i, j);
			patternStack.push(current);
			hold.insert(current);
		}

		void markPatterns() {
			int i_min, i_max, j_min, j_max;
			for (set<Pixel*> pattern : patternSet) {
				i_min = h;
				j_min = w;
				i_max = -1;
				j_max = -1;
				for (Pixel* pixel : pattern) {
					int i = pixel->i;
					int j = pixel->j;
					i_min = min(i, i_min);
					i_max = max(i, i_max);
					j_min = min(j, j_min);
					j_max = max(j, j_max);
					//cout << i << " " << j << endl;
					img_mark.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255);
				}
				// draw boundary box in blue
				// cout << "(" << i_min << "," << j_min << ")	(" << i_max << "," << j_max << ")\n";
				cv::rectangle(img_mark,cv::Point(j_min, i_min), cv::Point(j_max, i_max), cv::Scalar(255, 0, 0), 1, 8, 0);
			}
		}

	};

void Salembier(const cv::Mat img, cv::Mat & img_out,int size) {
	const int h = img.rows;
	const int w = img.cols;
	cv::Mat dst0,dst45,dst90,dst135;

	if (size > h || size > w) {
		cerr << "Undefied kernel size" << endl;
		throw;
	}

	if (size % 2 == 0) {
		cerr << "Kernel size must be odd number";
		throw;
	}

	// generate linear kernel
	// 0 and 90
	cv::Mat K_L0 = cv::Mat::zeros(cv::Size(size,size), CV_8UC1);
	cv::Mat K_L90 = cv::Mat::zeros(cv::Size(size, size), CV_8UC1);
	int mid = size / 2;
	for (int i = 0; i < size; i++) {
		K_L0.at<uchar>(mid, i) = 1;
		K_L90.at<uchar>(i, mid) = 1;
	}
	// 45 and 90 
	cv::Mat K_L135 = cv::Mat::eye(cv::Size(size, size), CV_8UC1);
	cv::Mat K_L45;
	cv::rotate(K_L135, K_L45, cv::ROTATE_90_CLOCKWISE);
	// cout << K_L0 << "\n\n" << K_L45 << "\n\n" << K_L90 << "\n\n" << K_L135;
	
	// openning
	cv::morphologyEx(img, dst0, 2, K_L0);
	cv::morphologyEx(img, dst45, 2, K_L45);
	cv::morphologyEx(img, dst90, 2, K_L90);
	cv::morphologyEx(img, dst135, 2, K_L135);
	// closing
	cv::morphologyEx(dst0, dst0, 3, K_L0);
	cv::morphologyEx(dst45, dst45, 3, K_L45);
	cv::morphologyEx(dst90, dst90, 3, K_L90);
	cv::morphologyEx(dst135, dst135, 3, K_L135);

	img_out = cv::Mat(h, w, CV_8UC1);
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			cv::Scalar c = img.at<uchar>(i, j);
			cv::Scalar c0 = dst0.at<uchar>(i, j);
			cv::Scalar c45 = dst45.at<uchar>(i, j);
			cv::Scalar c90 = dst90.at<uchar>(i, j);
			cv::Scalar c135 = dst135.at<uchar>(i, j);
			img_out.at<uchar>(i,j)=max(max(max(max(c.val[0], c0.val[0]), c45.val[0]), c90.val[0]), c135.val[0]) - c.val[0];
		}
	}
}


int main(int argc, char** argv) {
	TODO:cv::Mat img = cv::imread("C:/Users/koufo/Desktop/UCB/LAB/Crack Evolutoin/images/before.png"); // change this input file
	cv::GaussianBlur(img, img, cv::Size(5, 5), 0.2);
	PatternSegmentation p = PatternSegmentation(img,5);
	p.drawPatterns();
	cv::threshold(img, img, 100, 255, 3);
	cv::imshow("Display window", img);                   // Show our image inside it.
	cv::waitKey();
	//cv::imwrite("xxx.png", img);
	return 0;
}