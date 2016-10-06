#include <cv.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <stdio.h>
#include <queue> 
#include <thread>

using namespace std;


//create a frame
IplImage* CadrMaker(IplImage* frame1, IplImage* frame2, int k){
	IplImage* src = NULL;
	IplImage* cadr1;
	IplImage* cadr2;
	//what of video is a base
	if (k != 1){
		cadr1 = frame1;
		cadr2 = frame2;
	}
	else {
		cadr2 = frame1;
		cadr1 = frame2;
	}
	//resize second video
	src = cvCreateImage(cvSize(cadr1->width / 4, cadr1->height / 4), cadr2->depth, cadr2->nChannels);
	cvResize(cadr2, src, 1);
	//post it
	cvSetImageROI(cadr1, cvRect(cadr1->width * 3 / 4, 0, src->width, src->height));
	cvZero(cadr1);
	cvCopyImage(src, cadr1);
	cvResetImageROI(cadr1);
	if(src)cvReleaseImage(&src);
	return cadr1;
}

//Show frame
void MovieMaker(std::queue <IplImage*> & Video1, std::queue <IplImage*> & Video2,int d, int k){
	IplImage* frame1;
	IplImage* frame2;
	if (k == 1){
		frame1 = Video1.front();
		frame2 = Video2.front();
	}
	else if(k==2){
		frame1 = Video2.front();
		frame2 = Video1.front();
	}
	else
	{
		printf("ERROR\n");
		exit(EXIT_FAILURE);
	}
	if (d==0){
		cvShowImage("Video Test", CadrMaker(frame1, frame2, k));
		//if we have a two video

	}
	else if (d==-1){
		IplImage* src = NULL;
		src = cvCreateImage(cvSize(frame2->width, frame2->height), frame1->depth, frame1->nChannels);
		cvResize(frame1, src, 1);
		cvZero(frame1);
		cvCopyImage(src, frame1);
		cvShowImage("Video Test", frame1);
		if(src)cvReleaseImage(&src);
		// we have only second video
	}
	else if (d==1){
		cvShowImage("Video Test", frame1);
		 // we have a base video
	}
}

//if we have a cadre in buffer and at video file -  return 1, only in a buffer - 2, else - return 0.
int NextCadr(size_t numbercadr, CvCapture* capture, int flag, std::queue <IplImage*> & Video){
	
	//we can read next cadre 
	if (flag==1){ 
		IplImage* cadr = cvQueryFrame(capture);
		if (cadr){
			Video.push(cadr);
			if ((Video.size() > numbercadr)){
				Video.pop();
				return 1;
			}
			return 1;
		}
		else{
			Video.pop();
			return 2;
		}
	}
	// we can't read next cadre, but have cadres at buffer
	else if (flag == 2){
		if (Video.size() > 1){
			Video.pop();
			return 2;
		}
		return 0;
	}
}

typedef int *func(size_t, CvCapture*, int, std::queue <IplImage*> &);

void thread_func(func f, size_t numbercadr, CvCapture* capture, int &flag, std::queue <IplImage*> & Video)
{
	flag = NextCadr(numbercadr, capture, flag, Video);
}

int main(int argc, char* argv[1])
{
	//create buffers
	std::queue <IplImage*> Video1, Video2;
	
	int n, time_sum, time_def, k;
	uint32_t time1, time2;
	char c;
	// Window to show video
	cv::namedWindow("Video Test", CV_WINDOW_NORMAL);
	// k - to identify what video are faster
	
	const char* filename1 = argc == 3 ? argv[1] : "NFS.mp4";
	const char* filename2 = argc == 3 ? argv[2] : "PonyTest.mp4";

	// have information about movies
	CvCapture* capture1 = cvCreateFileCapture(filename1);
	double fps1 = cvGetCaptureProperty(capture1, CV_CAP_PROP_FPS);
	time1 = (uint32_t)(1000 / fps1);

	CvCapture* capture2 = cvCreateFileCapture(filename2);
	double fps2 = cvGetCaptureProperty(capture2, CV_CAP_PROP_FPS);
	time2 = (uint32_t)(1000 / fps2);

	// what is faster?
	int p;
	if (time1 == time2){
		time_sum = time1;
		time_def = time1;
		n = 1;
		k = 1;
		p = 1;
	}
	else {
		time_sum = min(time1, time2);
		if (time_sum != time2){
			n = time2 / time1;
			p = time2 % time1;
			time_def = time2 - n*time_sum;
			k = 2;
		}
		else {
			n = time1 / time2;
			p = time1 % time2;
			time_def = time1 - n*time_sum;
			k = 1;
		}
	}
	
	
	//buffer filling
	size_t numbercadr1 = (size_t)fps1;//number of cadre at first video
	size_t numbercadr2 = (size_t)fps2;//number of cadre in second video
	printf("%d, %d", numbercadr1, numbercadr2);
	// Responsible for the status of the first video 1 - Entry and delete, 2 - delete, 0- empty.
	uint32_t flag1 = 1; 
	while (Video1.size() < numbercadr1 && flag1!=0){
		flag1 = NextCadr(numbercadr1, capture1, flag1, Video1);
	}
	// Responsible for the status of the second video 1 - Entry and delete, 2 - delete, 0- empty.
	uint32_t flag2 = 1;
	while (Video2.size() < numbercadr2 && flag2 != 0){
		flag2 = NextCadr(numbercadr2, capture2, flag2, Video2);
	}

	// select the next cadres
	int i = 0;
	int time0;
	int d = 0;// How many videos there? 0 - both -1 - only the second, 1 - Only the first 3 - do not have them
	//show cadres
		while (flag1 > 0 || flag2 > 0){
			MovieMaker(Video1, Video2, d, k);
			if ((flag1 != 0) && (flag2 != 0)){
				if (i % (n) == p){
					if (k == 2)
					{
						std::thread reader1(NextCadr, numbercadr1, capture1, flag1, Video1);
						reader1.join();
					}
					else if (k == 1)
					{
						
						std::thread reader2(NextCadr, numbercadr2, capture2, flag2, Video2);
						reader2.join();
					}
					time0 = time_def;
				}
				else{
					time0 = time_sum;
					
					flag1 = NextCadr(numbercadr1, capture1, flag1, Video1);
					flag2 = NextCadr(numbercadr2, capture2, flag2, Video2);
				}
			}
			else if (flag2 != 0){
				if (capture1)cvReleaseCapture(&capture1);
				d = -1;
				time0 = time2;
				std::thread reader2(NextCadr, numbercadr2, capture2, flag2, Video2);
				reader2.join();
			}
			else if (flag1 != 0){
				if (capture2)cvReleaseCapture(&capture2);
				d = 1;
				time0 = time1;
				std::thread reader1(NextCadr, numbercadr1, capture1, flag1, Video1);
				reader1.join();
			}
			c = cvWaitKey(time0);
			if (c == 27){ break; }
			i++;
		}
	
	//release resources 
	if(capture1)cvReleaseCapture(&capture1);
	if(capture2)cvReleaseCapture(&capture2);
	//Destroy Window
	cvDestroyWindow("Video Test");
	return 0;
}
