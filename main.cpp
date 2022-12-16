#include <iostream>
#include <stdio.h>
#include <string>

#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/core.hpp>      
#include <opencv2/highgui.hpp>
#include "video-factory.pb.h"
#include "test01.pb.h"

#define NCOLS 20
#define NROWS 20
#define COLOR_SAME 40

using namespace cv;
using namespace std;

Scalar getMSSIM(const Mat& i1, const Mat& i2);
Mat sampleImg;
int restore_count = 0;

bool visualize = false;

float color_dist(Vec3b& a, Vec3b& b) {
	return sqrt(pow(a[0] - b[0], 2) + pow(a[1] - b[1], 2) + pow(a[2] - b[2], 2));
}

// 20 X 20 ( NCOLS X NROWS ) ���� �ڽ����� ��ȭ üũ
void motionDetect(Mat& source, Mat& buffer, Mat& restore, int frame, int width, int height, int xStep = 8, int yStep = 4) {
	int boxPx, boxPy, curPx, curPy;
	bool flag = false;

	FileStorage fs;
	string filename = "json/boxDetect" + to_string(frame) + ".json";
	fs.open(filename, FileStorage::WRITE);
	if (!fs.isOpened()) {
		cout << "jsonFileStorageTest.json  file can't open" << endl;
	}
	
	// visualize
	Mat m;
	source.copyTo(m);

	for (int i = 0; i < NCOLS; i++) {
		for (int j = 0; j < NROWS; j++) {
			boxPx = (i * 1920 / NCOLS);
			boxPy = (j * 1080 / NROWS);
			flag = false; // ���� �ڽ� �Ѱ����� �ٸ� Į�� �� �߰�(1/72 = 98.5%) 
			for (int x = 0; x < (1920 / NCOLS); x += xStep) {
				if (flag) break;
				for (int y = 0; y < (1080 / NROWS); y += yStep) {
					curPx = boxPx + x;
					curPy = boxPy + y;
					float d = color_dist(source.at<Vec3b>(curPy, curPx), buffer.at<Vec3b>(curPy, curPx));
					if (d > COLOR_SAME) {
						flag = true;
						break;
					}
				}
			}
			if (flag) {
				// �� �ڽ� ���� ī����
				restore_count++;
				// (���� ��) �̹��� ���� �κ� �ٲ㼭 ���� ����
				source(Rect(boxPx, boxPy, 1920 / NCOLS, 1080 / NROWS)).copyTo(buffer(Rect(boxPx, boxPy, 1920 / NCOLS, 1080 / NROWS)));
				
				// Json 
				fs.startWriteStruct("box", FileNode::MAP);

				fs << "boxID" << restore_count;
				fs << "frame" << frame;
				fs << "posX" << boxPx;
				fs << "posY" << boxPy;
				fs << "mat" << source(Rect(boxPx, boxPy, 5, 5));
				//fs << "mat" << source(Rect(boxPx, boxPy, 1920 / NCOLS, 1080 / NROWS));
				fs.endWriteStruct();

				// ProtoBuf
				SomeMessage protoMsg;
				SomeMessage_Mat protoMat;
				SomeMessage_Box protoBox;
				protoMat.set_rows(1080 / NROWS);
				protoMat.set_cols(1920 / NCOLS);

				protoBox.set_boxid(restore_count);
				protoBox.set_frame(frame);
				protoBox.set_posx(boxPx);
				protoBox.set_posy(boxPy);
				
				

				//TODO �� �ڽ� �����ؼ� restore �� �ش�κ� ��������.
				
				//visualize
				if (visualize) {
					for (int x = 0; x < (1920 / NCOLS); x++) {
						for (int y = 0; y < (1080 / NROWS); y++) {
							m.at<Vec3b>(boxPy + y, boxPx + x) = Vec3b(220, 20, 220);
						}
					}
				}				

				//for (int x = 0; x < (1920 / NCOLS); x++) {
				//	for (int y = 0; y < (1080 / NROWS); y++) {
				//		//TODO �̹��� �����ؾ���.
				//		source(Rect())
				//		buffer.at<Vec3b>(boxPy + y, boxPx + x) = ;
				//		source.at<Vec3b>(boxPy + y, boxPx + x) = Vec3b(220, 20, 220);
				//	}
				//}
			}
		}
	}

	// Json File close
	fs.release();

	if (visualize){
		imshow("Window1", m);
	}
	
}

int main(int ac, char** av) {
	
	// ���� ��ġ ���ϸ�
	string source_path = "source_motion.mkv";		
	string dest_path = "output_motion.mkv";
	string window_name = "Window1";
	string source_img = "image/img_source1.jpg";
	
	cv::VideoCapture videoCapture(source_path);
	cv::VideoWriter videoWriter;
	cv::Mat videoFrame;
	
	Mat img1 = imread(source_img);
	Mat simg1;
	cv::resize(img1, simg1, Size(1920, 1080));
	if (!videoCapture.isOpened()) {
		std::cout << "Can't open video !!!" << std::endl;
		return -1;
	}
		
	float videoFPS = videoCapture.get(cv::CAP_PROP_FPS);
	int videoWidth = videoCapture.get(cv::CAP_PROP_FRAME_WIDTH);
	int videoHeight = videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT);

	std::cout << "Video Info" << std::endl;
	std::cout << "video FPS : " << videoFPS << std::endl;
	std::cout << "video width : " << videoWidth << std::endl;
	std::cout << "video height : " << videoHeight << std::endl;
	
	videoWriter.open(dest_path, cv::VideoWriter::fourcc('X','2','6','4'), videoFPS/2, cv::Size(videoWidth, videoHeight), true);
	if (!videoWriter.isOpened())
	{
		std::cout << "Can't write video !!! check setting" << std::endl;
		return -1;
	}

	////test
	//Mat test = Mat(Size(1920, 1080), CV_8UC3);
	//for (int i = 0; i < 1920; i++) {
	//	for (int j = 0; j < 1080; j++) {
	//		test.at<Vec3b>(j, i) = Vec3b(30, 30, 30);
	//	}
	//}
	//Mat test2 = Mat(Size(1920, 1080), CV_8UC3);
	//for (int i = 0; i < 1920; i++) {
	//	for (int j = 0; j < 1080; j++) {
	//		test2.at<Vec3b>(j, i) = Vec3b(80, 80, 80);
	//	}
	//}
	//motionDetect(test, test2, 1920, 1080);
	//return 0;
	
	

	Mat bufferFrame, restoreFrame;
	Mat boxFrames[NROWS][NCOLS];
	int count = 1;

	videoCapture >> videoFrame;
	if (videoFrame.empty()) {
		std::cout << "Video END" << std::endl;
		waitKey(0);
		return 0;
	}
	videoFrame.copyTo(bufferFrame);

	// TODO ù������ �����ؾ���. ������ �׳� ���� ���
	videoFrame.copyTo(restoreFrame);

	videoWriter << restoreFrame;
	cv::imshow(window_name, bufferFrame);


	double time = (double)getTickCount();
	// count : ���� ������, count < N : �����ɷ��� Test�� �ʹ� �� �����Ӹ� ���
	while (true && count < 600 ) {
		//cout << count <<endl;
		count++;
		//VideoCapture�� ���� �������� �޾ƿ´�
		videoCapture >> videoFrame;

		//ĸ�� ȭ���� ���� ���� Video�� ���� ���
		if (videoFrame.empty()) {
			std::cout << "Video END" << std::endl;
			break;
		}
		
		motionDetect(videoFrame, bufferFrame, restoreFrame, count, videoWidth, videoHeight);

		//�޾ƿ� Frame�� �����Ѵ�.
		videoWriter << restoreFrame;

		//window�� frame ���.
		//cv::imshow(window_name, bufferFrame);


		// Visualize
		////'ESC'Ű�� ������ ����ȴ�.
		////FPS�� �̿��Ͽ� ���� ��� �ӵ��� �����Ͽ��ش�.
		/*if (visualize && cv::waitKey(1000 / videoFPS) == 27) {
			std::cout << "Stop video record" << std::endl;
			break;
		}*/
	}
	time = 1000 * ((double)getTickCount() - time) / getTickFrequency();
	cout << "�ð� : " << time << endl;

	std::cout << "Video END" << std::endl;
	cout << "�� ������ �� : " << count << endl;
	cout << "�� box �� : " << count * 400 << endl;
	cout << "������ box�� : " << restore_count << endl;
	cout << "���� ���� " << ((float)restore_count * 100.0) / (((float)count) * 400.0) << endl;
	
	return 0;

}

