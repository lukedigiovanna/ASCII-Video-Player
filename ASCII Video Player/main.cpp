#include <iostream>
#include <Windows.h>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include <SFML/Audio.hpp>

#pragma comment(lib, "Winmm.lib")

#include <chrono>
#include <random>

#define MAX_WIDTH 150
#define MAX_HEIGHT 100

static bool spaceDown = false;

static float runtime = 0.0f;
static sf::Sound song;

static bool useColor = true;

static bool paused = false;

void togglePause() {
    paused = !paused;
    if (paused)
        song.stop();
    else {
        song.play();
        song.setPlayingOffset(sf::seconds(runtime));
    }
}

void setRuntime(float rt) {
    runtime = rt;

    if (paused) return;

    if (runtime < 0) runtime = 0.0f;
    song.setPlayingOffset(sf::seconds(runtime));
}

static unsigned short colorCodes[16 * 4] = {
    0, 0, 0, 0, // black
    1, 0, 0, 255, // blue
    2, 0, 255, 0, // green
    3, 173, 216, 230, // light blue
    4, 255, 0, 0, // red
    5, 128, 0, 128, // purple
    6, 255, 255, 0, // yellow
    7, 211, 211, 211, // light gray
    8, 128, 128, 128, // gray
    9, 135, 206, 235, // sky blue
    10, 50, 205, 50, // lime green
    11, 0, 255, 255, // aqua
    12, 255, 192, 203, // pink
    13, 255, 0, 255, // magenta
    14, 210, 180, 140, // tan
    15, 255, 255, 255 // white
};

int getDistance(int r1, int g1, int b1, int r2, int g2, int b2) {
    int rd = r1 - r2, gd = g1 - g2, bd = b1 - b2;
    return rd * rd + gd * gd + bd * bd;
}

WORD getColor(short r, short g, short b) {
    int minIndex = 0;
    int closest = getDistance(colorCodes[minIndex * 4 + 1], colorCodes[minIndex * 4 + 2], colorCodes[minIndex * 4 + 3], r, g, b);
    for (int i = 1; i < 16; i++) {
        int distance = getDistance(colorCodes[i * 4 + 1], colorCodes[i * 4 + 2], colorCodes[i * 4 + 3], r, g, b);
        if (distance < closest) {
            closest = distance;
            minIndex = i;
        }
    }
    return colorCodes[minIndex * 4] * 16 + 15;
}

int main() {


    DWORD dw;
    TCHAR space = ' ';
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string name;
    std::cout << "NAME: ";
    std::cin >> name;

    std::string videoFilePath = name + ".mp4";
    std::vector<cv::Mat> frames;

    float videoFPS;
    try {
        //open the video file
        cv::VideoCapture cap(videoFilePath); // open the video file
        if (!cap.isOpened())  // check if we succeeded
            std::cerr << "blah blah video bad :(" << std::endl;

        for (short x = 0; x < MAX_WIDTH * 2; x++)
            for (short y = 0; y < MAX_HEIGHT; y++)
                WriteConsoleOutputCharacter(console, &space, 1, { x, y }, &dw);

        printf("LOADING VIDEO: %s\n", name.c_str());

        int numFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
        for (int frameNum = 0; frameNum < numFrames; frameNum++) {
            cv::Mat frame;
            cap >> frame; // get the next frame from video
            frames.push_back(frame);

            float progress = (float)frameNum / numFrames;
            char str[13]; // 00% complete
            sprintf_s(str, "%02i%% complete", (int)(progress * 100));
            for (short i = 0; i < 13; i++) {
                TCHAR c = (TCHAR)str[i];
                WriteConsoleOutputCharacter(console, &c, 1, { 8 + i, 3 }, &dw);
            }
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
    printf("# FRAMES: %i\n", frames.size());
    printf("DURATION: %f\n", frames.size() / videoFPS);

    Sleep(2500);


    TCHAR frameCache[MAX_WIDTH * MAX_HEIGHT];
    WORD colorCache[MAX_WIDTH * MAX_HEIGHT];

    std::string stringPallette = " ..:\"*#%";
    int numChars = stringPallette.size() - 1;
    const char* pallette = stringPallette.c_str();

    bool drawWithWords = false;
    int idx = 0;

    for (short i = 0; i < MAX_HEIGHT; i++)
        for (short j = 0; j < MAX_WIDTH; j++)
            WriteConsoleOutputCharacter(console, &space, 1, { j * 2 + 1, i }, &dw);

    float duration = (float)frames.size() / videoFPS;

    setRuntime(0);
    
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(name+".wav")) {
        return -1;
    }
    song.setBuffer(buffer);
    song.play();


    auto last = std::chrono::high_resolution_clock::now();
    float dt = 0.0f;
    short screenWidth = 0, screenHeight = 0;
    while (runtime < duration) {
        if (GetKeyState(VK_LEFT) & 0x8000) {
            setRuntime(runtime - dt * 15);
        }
        if (GetKeyState(VK_RIGHT) & 0x8000) {
            setRuntime(runtime + dt * 15);
        }

        if (GetKeyState(VK_SPACE) & 0x8000) {
            if (!spaceDown)
                togglePause();
            spaceDown = true;
        }
        else spaceDown = false;

        CONSOLE_SCREEN_BUFFER_INFO inf;
        GetConsoleScreenBufferInfo(console, &inf);
        short nScreenWidth = (inf.srWindow.Right - inf.srWindow.Left) / 2;
        short nScreenHeight = inf.srWindow.Bottom - inf.srWindow.Top - 1;
        if (nScreenWidth != screenWidth || nScreenHeight != screenHeight) {
            // clear the buffer
            WORD empty = 15;
            for (short i = 0; i < MAX_WIDTH * 2; i++) 
                for (short j = 0; j < MAX_HEIGHT; j++) {
                    WriteConsoleOutputCharacter(console, &space, 1, { i, j }, &dw);
                    WriteConsoleOutputAttribute(console, &empty, 1, { i, j }, &dw);
                }
            for (int i = 0; i < MAX_WIDTH * MAX_HEIGHT; i++) {
                frameCache[i] = 0;
                colorCache[i] = 0;
            }
        }
        screenWidth = nScreenWidth;
        screenHeight = nScreenHeight;

        int i = (int)(runtime / duration * frames.size());
        cv::Mat& mat = frames.at(i);
        idx = 0;
        for (short i = 0; i < screenHeight; i++) {
            for (short j = 0; j < screenWidth; j++) {
                int imgX = (int)((float)j / screenWidth * mat.cols), imgY = (int)((float)i / screenHeight * mat.rows);
                cv::Vec3b pixel = mat.at<cv::Vec3b>(imgY, imgX);
                float val = (pixel[0] + pixel[1] + pixel[2]) / 765.0f;
                TCHAR c = ' ';
                if (!drawWithWords) {
                    c = (TCHAR)pallette[(int)(val * numChars)];
                }
                else {
                    if (val > 0.5f) {
                        c = pallette[idx];
                        idx = (idx + 1) % numChars;
                    }
                }
                if (c != frameCache[j + i * MAX_WIDTH]) {
                    WriteConsoleOutputCharacter(console, &c, 1, { j * 2, i }, &dw);
                    frameCache[j + i * MAX_WIDTH] = c;
                }
                if (useColor) {
                    WORD cc = getColor(pixel[2], pixel[1], pixel[0]);
                    if (cc != colorCache[j + i * MAX_WIDTH]) {
                        WriteConsoleOutputAttribute(console, &cc, 1, { j * 2, i }, &dw);
                        WriteConsoleOutputAttribute(console, &cc, 1, { j * 2 + 1, i }, &dw);
                        colorCache[j + i * MAX_WIDTH] = cc;
                    }
                }
            }
        }

        float percentDone = runtime / duration;
        TCHAR c = ':';
        for (short i = 1; i < screenWidth * 2 - 3; i++) {
            if ((float)i / (screenWidth * 2) > percentDone)
                c = '.';
            WriteConsoleOutputCharacter(console, &c, 1, { i, screenHeight }, &dw);
        }
        TCHAR ends = '<';
        WriteConsoleOutputCharacter(console, &ends, 1, { 0, screenHeight }, &dw);
        ends = '>';
        WriteConsoleOutputCharacter(console, &ends, 1, { screenWidth * 2 - 3, screenHeight }, &dw);
        // draw the time stamp
        int minutes = (int)runtime / 60;
        int seconds = (int)runtime % 60;
        char str[10];
        sprintf_s(str, "%02i:%02i", minutes, seconds);
        //for (short i = 0; i < WIDTH * 2; i++)
         //   WriteConsoleOutputCharacter(console, &space, 1, { i, HEIGHT + 1 }, &dw);
        for (short i = 0; i < 5; i++) {
            TCHAR c = (TCHAR)str[i];
            WriteConsoleOutputCharacter(console, &c, 1, { 2 + i, screenHeight + 1 }, &dw);
        }

        //Sleep(5);

        auto now = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - last).count() / 1000000000.0f;
        if (!paused)
            runtime += dt;
        last = now;
    }

    for (short i = 0; i < MAX_WIDTH; i++) for (short j = 0; j < MAX_HEIGHT; j++)
        WriteConsoleOutputCharacter(console, &space, 1, { i, j }, &dw);

    std::cout << "THANKS FOR WATCHING!" << std::endl;

    Sleep(3000);

	return 0;
}