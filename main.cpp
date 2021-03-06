//James Rogers Nov 2020 (c) Plymouth University
//Student ID: 10609827
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    //Calibration file paths (you need to make these)
    string intrinsic_filename = "../Task4/intrinsics.xml";
    string extrinsic_filename = "../Task4/extrinsics.xml";

    //================================================Load Calibration Files===============================================
    //This code loads in the intrinsics.xml and extrinsics.xml calibration files and creates: map11, map12, map21, map22.
    //These four matrices are used to distort the camera images to apply the lens correction.
    Rect roi1, roi2;
    Mat Q;
    Size img_size = {640,480};

    FileStorage fs(intrinsic_filename, FileStorage::READ);
    if(!fs.isOpened()){
        printf("Failed to open file %s\n", intrinsic_filename.c_str());
        return -1;
    }

    Mat M1, D1, M2, D2;
    fs["M1"] >> M1;
    fs["D1"] >> D1;
    fs["M2"] >> M2;
    fs["D2"] >> D2;

    fs.open(extrinsic_filename, FileStorage::READ);
    if(!fs.isOpened())
    {
        printf("Failed to open file %s\n", extrinsic_filename.c_str());
        return -1;
    }
    Mat R, T, R1, P1, R2, P2;
    fs["R"] >> R;
    fs["T"] >> T;

    stereoRectify( M1, D1, M2, D2, img_size, R, T, R1, R2, P1, P2, Q, CALIB_ZERO_DISPARITY, -1, img_size, &roi1, &roi2 );

    Mat map11, map12, map21, map22;
    initUndistortRectifyMap(M1, D1, R1, P1, img_size, CV_16SC2, map11, map12);
    initUndistortRectifyMap(M2, D2, R2, P2, img_size, CV_16SC2, map21, map22);

    //===============================================Stereo SGBM Settings==================================================
    //This sets up the block matcher, which is used to create the disparity map. The various settings can be changed to
    //obtain different results. Note that some settings will crash the program.

    int SADWindowSize=3;            //must be an odd number >=3
    int numberOfDisparities=256;    //must be divisable by 16

    Ptr<StereoSGBM> sgbm = StereoSGBM::create(0,16,3);
    sgbm->setBlockSize(SADWindowSize);
    sgbm->setPreFilterCap(63);
    sgbm->setP1(8*3*SADWindowSize*SADWindowSize);
    sgbm->setP2(32*3*SADWindowSize*SADWindowSize);
    sgbm->setMinDisparity(0);
    sgbm->setNumDisparities(numberOfDisparities);
    sgbm->setUniquenessRatio(10);
    sgbm->setSpeckleWindowSize(200);
    sgbm->setSpeckleRange(32);
    sgbm->setDisp12MaxDiff(1);
    sgbm->setMode(StereoSGBM::MODE_SGBM);

    //==================================================Main Program Loop================================================
    int ImageNum=0; //Current image index (Unknown Targets)
    //int ImageNum=30; //Current image index (Known Distances)
    while (1){
        //Load images from file (Unknown Targets)
        Mat Left =imread("../Task4/Unknown Targets/left" +to_string(ImageNum)+".jpg");
        Mat Right=imread("../Task4/Unknown Targets/right"+to_string(ImageNum)+".jpg");
        cout<<"Loaded Image: "<<ImageNum << endl;

        //Load images from file (Known Distances)
        //Mat Left =imread("../Task4/Distance Targets/left" +to_string(ImageNum)+"cm.jpg");
        //Mat Right=imread("../Task4/Distance Targets/right"+to_string(ImageNum)+"cm.jpg");
        //cout<<"Loaded Image: "<<ImageNum<< "0mm" << endl;

        //Distort image to correct for lens/positional distortion
        remap(Left, Left, map11, map12, INTER_LINEAR);
        remap(Right, Right, map21, map22, INTER_LINEAR);

        //Match left and right images to create disparity image
        Mat disp16bit, disp8bit, distanceMap;
        sgbm->compute(Left, Right, disp16bit);                                  //Compute 16-bit greyscale image with the stereo block matcher
        disp16bit.convertTo(disp8bit, CV_8U, 255/(numberOfDisparities*16.));    //Convert disparity map to an 8-bit greyscale image so it can be displayed (Only for imshow, do not use for disparity calculations)
        disp16bit.convertTo(distanceMap, CV_8U, 255/(numberOfDisparities*16.)); //Convert disparity map to an 8-bit greyscale image for displaying the distance map

        //==================================Your code goes here===============================
        //f = focal length, B = baseline (distance between cameras)
        //Disparity = (B * f) / Distance
        //Distance = (B * f) / Disparity
        //(B * f) = Distance * Disparity

        //(Baseline * Focal Length) value (mm), based on known-distance testing
        float BxF= 39000.0;

        //Display disparity value of central pixel
        int disparity = disp8bit.at<uchar>(240,360);
        cout << "Disparity Value: " << disparity << endl;

        //Calculate and display estimated distance
        int distance = BxF / disparity;
        cout << "Estimated Distance: " << distance << "mm" << endl;

        //Nested for-loops to process each individual pixel in the entire image
        for(int y=0; y<distanceMap.size().height; y++){
            for(int x=0; x<distanceMap.size().width; x++){
                distanceMap.at<uchar>(y,x) = BxF/(int)distanceMap.at<uchar>(y,x);
            }
        }

        //Display images until x is pressed
        while(waitKey(10)!='x')
        {
            //imshow("Left", Left);
            //imshow("Right", Right);
            imshow("Disparity", disp8bit);
            imshow("Distance Map", distanceMap);
        }

        //Move to next image (Unknown Targets)
        ImageNum++;
        if(ImageNum>7)
        {
            ImageNum=0;
        }

        //Move to next image (Distance Targets)
        /*ImageNum = ImageNum+10;
        if(ImageNum>150)
        {
            ImageNum=30;
        }*/

    }

    return 0;
}





