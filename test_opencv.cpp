#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    const std::string faceCascadePath =
        "/usr/local/Cellar/opencv/4.12.0_15/share/opencv4/haarcascades/haarcascade_frontalface_default.xml";
    const std::string eyeCascadePath =
        "/usr/local/Cellar/opencv/4.12.0_15/share/opencv4/haarcascades/haarcascade_eye.xml";

    // Load cascades
    cv::CascadeClassifier face_cascade, eye_cascade;
    if (!face_cascade.load(faceCascadePath)) {
        std::cerr << "Could not load face cascade\n";
        return -1;
    }
    if (!eye_cascade.load(eyeCascadePath)) {
        std::cerr << "Could not load eye cascade\n";
        return -1;
    }

    // Open camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera.\n";
        return -1;
    }

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        // Grayscale + contrast
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        // Detect faces
        std::vector<cv::Rect> faces;
        face_cascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));

        if (!faces.empty()) {
            // pick the largest face
            cv::Rect bestFace = faces[0];
            for (size_t i = 1; i < faces.size(); ++i)
                if (faces[i].area() > bestFace.area())
                    bestFace = faces[i];

            // draw face
            cv::rectangle(frame, bestFace, cv::Scalar(255, 0, 0), 2);

            // detect eyes inside that face
            cv::Mat faceROI = gray(bestFace);
            std::vector<cv::Rect> eyes;
            eye_cascade.detectMultiScale(faceROI, eyes, 1.1, 3, 0, cv::Size(15, 15));

            for (const auto& e : eyes) {
                cv::Rect eye(
                    bestFace.x + e.x,
                    bestFace.y + e.y,
                    e.width,
                    e.height
                );
                cv::rectangle(frame, eye, cv::Scalar(0, 255, 255), 2);
            }
        }

        cv::imshow("Face + Eye Detection", frame);

        int key = cv::waitKey(30);
        if (key == 27) break;      // ESC to quit
        if (key == 's') cv::imwrite("snapshot.png", frame);
    }

    return 0;
}
