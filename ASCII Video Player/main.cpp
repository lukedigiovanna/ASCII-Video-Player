#include <iostream>
#include <Windows.h>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#pragma comment(lib, "Winmm.lib")

#include <chrono>

#define WIDTH 110
#define HEIGHT 48

static bool canPressRewind = true, canPressForward = true;

static float runtime = 0.0f;

void setRuntime(float rt) {
    runtime = rt;

    if (runtime < 0) runtime = 0.0f;

    mciSendStringA("stop mp3", NULL, 0, NULL);
    std::string com = "play mp3 from " + std::to_string((int)(runtime * 1000));
   // std::cout << com << std::endl;
    mciSendStringA(com.c_str(), NULL, 0, NULL);
}

int main() {
    std::string name = "arctic";

    std::string videoFilePath = name + ".mp4";
    std::vector<cv::Mat> frames;

    float videoFPS;

    try {
        //open the video file
        cv::VideoCapture cap(videoFilePath); // open the video file
        if (!cap.isOpened())  // check if we succeeded
            std::cerr << "blah blah video bad :(" << std::endl;

        //cap.get(CV_CAP_PROP_FRAME_COUNT) contains the number of frames in the video;
        for (int frameNum = 0; frameNum < cap.get(cv::CAP_PROP_FRAME_COUNT); frameNum++)
        {
            cv::Mat frame;
            cap >> frame; // get the next frame from video
            frames.push_back(frame);
        }

        videoFPS = cap.get(cv::CAP_PROP_FPS);
    }
    catch (cv::Exception & e) {
        std::cerr << e.msg << std::endl;
        exit(1);
    }

    std::string titleText = "LOADED VIDEO TITLE: " + name + "\n";
    printf(titleText.c_str());
    printf("FPS: %f\n", videoFPS);
    printf("# FRAMES: %u\n", frames.size());
    printf("DURATION: %f\n", frames.size() / videoFPS);

    Sleep(2500);

	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string com = "open \""+name+".mp3\" type mpegvideo alias mp3";
    mciSendStringA(com.c_str(), NULL, 0, NULL);
    mciSendStringA("play mp3 from 0", NULL, 0, NULL);

    TCHAR frameCache[WIDTH * HEIGHT];

	DWORD dw;
	TCHAR space = ' ', equals = '=';
    //const char* pallette = " abcdefghijklmnopqrstuvwxyz";
    const char* pallette = " ..:\"*#%";
    //const char* pallette = " beckham";

    for (short i = 0; i < HEIGHT; i++)
        for (short j = 0; j < WIDTH; j++)
            WriteConsoleOutputCharacter(console, &space, 1, { j * 2 + 1, i }, &dw);

    float duration = (float)frames.size() / videoFPS;

    auto last = std::chrono::high_resolution_clock::now();
    while (runtime < duration) {
        if (GetKeyState(VK_LEFT) & 0x8000) {
            if (canPressRewind) {
                setRuntime(runtime - 5.0f);
            }
            canPressRewind = false;
        }
        else {
            canPressRewind = true;
        }
        if (GetKeyState(VK_RIGHT) & 0x8000) {
            if (canPressForward) {
                setRuntime(runtime + 5.0f);
            }
            canPressForward = false;
        }
        else {
            canPressForward = true;
        }

        int i = (int)(runtime / duration * frames.size());
        cv::Mat& mat = frames.at(i);
        for (short i = 0; i < HEIGHT; i++) {
            for (short j = 0; j < WIDTH; j++) {
                int imgX = (int)((float)j / WIDTH * mat.cols), imgY = (int)((float)i / HEIGHT * mat.rows);
                cv::Vec3b pixel = mat.at<cv::Vec3b>(imgY, imgX);
                float val = (pixel[0] + pixel[1] + pixel[2]) / 765.0f;
                TCHAR c = (TCHAR)pallette[(int)(val * 7)];
                if (c == frameCache[j + i * WIDTH])
                    continue;
                WriteConsoleOutputCharacter(console, &c, 1, { j * 2, i }, &dw);
                frameCache[j + i * WIDTH] = c;
            }
        }
        float percentDone = runtime / duration;
        TCHAR c = ':';
        for (short i = 1; i < WIDTH * 2; i++) {
            if ((float)i / (WIDTH * 2) > percentDone)
                c = '.';
            WriteConsoleOutputCharacter(console, &c, 1, { i, HEIGHT }, &dw);
        }
        TCHAR ends = '<';
        WriteConsoleOutputCharacter(console, &ends, 1, { 0, HEIGHT }, &dw);
        ends = '>';
        WriteConsoleOutputCharacter(console, &ends, 1, { WIDTH * 2 - 3, HEIGHT }, &dw);
        // draw the time stamp
        int minutes = (int)runtime / 60;
        int seconds = (int)runtime % 60;
        char str[10];
        //for (int i = 0; i < 30; i++) str[i] = ' ';
        sprintf_s(str, "%02i:%02i", minutes, seconds);
        //for (short i = 0; i < WIDTH * 2; i++)
         //   WriteConsoleOutputCharacter(console, &space, 1, { i, HEIGHT + 1 }, &dw);
        for (short i = 0; i < 5; i++) {
            TCHAR c = (TCHAR)str[i];
            WriteConsoleOutputCharacter(console, &c, 1, { 2 + i, HEIGHT + 1 }, &dw);
        }

        Sleep(5);

        auto now = std::chrono::high_resolution_clock::now();
        runtime += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - last).count() / 1000000000.0f;
        last = now;
    }

	return 0;
}