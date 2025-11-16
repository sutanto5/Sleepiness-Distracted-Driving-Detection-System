#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <iostream>
#include <cstdlib>

double eye_aspect_ratio(const std::vector<cv::Point2f>& eye) {
    double A = cv::norm(eye[1] - eye[5]);
    double B = cv::norm(eye[2] - eye[4]);
    double C = cv::norm(eye[0] - eye[3]);
    return (A + B) / (2.0 * C);
}

void playAlarmSound() {
    std::system("say 'Wake up!' &");
}

int main() {
    const std::string faceCascadePath =
        "/usr/local/Cellar/opencv/4.12.0_15/share/opencv4/haarcascades/haarcascade_frontalface_default.xml";
        const std::string facemarkModelPath = "../models/lbfmodel.yaml";


    // Load face detector
    cv::CascadeClassifier face_cascade;
    if (!face_cascade.load(faceCascadePath)) {
        std::cerr << "Could not load face cascade\n";
        return -1;
    }

    // Load facemark model (landmarks)
    cv::Ptr<cv::face::Facemark> facemark = cv::face::FacemarkLBF::create();
    try {
        facemark->loadModel(facemarkModelPath);
    } catch (const cv::Exception& e) {
        std::cerr << "Could not load facemark model: " << facemarkModelPath << "\n";
        std::cerr << e.msg << "\n";
        return -1;
    }


    // Open camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera.\n";
        return -1;
    }

    // For performance, keep resolution modest
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    // Drowsiness parameters
    const double EAR_THRESH = 0.25;        // tweak for your face / camera
    const int EAR_CONSEC_FRAMES = 15;      // how many frames below threshold → drowsy

    int lowEarFrameCount = 0;
    bool drowsy = false;
    bool alarmPlaying = false;

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        // Detect faces
        std::vector<cv::Rect> faces;
        face_cascade.detectMultiScale(
            gray,
            faces,
            1.1,
            5,
            cv::CASCADE_SCALE_IMAGE,
            cv::Size(80, 80)
        );

        if (!faces.empty()) {
            // pick the largest face (only one)
            cv::Rect bestFace = faces[0];
            for (size_t i = 1; i < faces.size(); ++i) {
                if (faces[i].area() > bestFace.area())
                    bestFace = faces[i];
            }

            // Run facemark on that single face
            std::vector<cv::Rect> singleFace{ bestFace };
            std::vector<std::vector<cv::Point2f>> landmarks;

            bool success = facemark->fit(gray, singleFace, landmarks);
            if (success && !landmarks.empty()) {
                auto& pts = landmarks[0];

                // Extract left and right eye points
                std::vector<cv::Point2f> leftEye{
                    pts[36], pts[37], pts[38], pts[39], pts[40], pts[41]
                };
                std::vector<cv::Point2f> rightEye{
                    pts[42], pts[43], pts[44], pts[45], pts[46], pts[47]
                };

                double leftEAR = eye_aspect_ratio(leftEye);
                double rightEAR = eye_aspect_ratio(rightEye);
                double ear = (leftEAR + rightEAR) / 2.0;

                // Draw face box
                cv::rectangle(frame, bestFace, cv::Scalar(255, 0, 0), 2);

                // Draw eye landmarks
                for (const auto& p : leftEye)
                    cv::circle(frame, p, 2, cv::Scalar(0, 255, 255), cv::FILLED);
                for (const auto& p : rightEye)
                    cv::circle(frame, p, 2, cv::Scalar(0, 255, 255), cv::FILLED);

                // EAR and drowsiness logic
                if (ear < EAR_THRESH) {
                    lowEarFrameCount++;
                    alarmPlaying = false;
                } else {
                    lowEarFrameCount = 0;
                    drowsy = false;
                    alarmPlaying = false;
                }

                if (lowEarFrameCount >= EAR_CONSEC_FRAMES) {
                    drowsy = true;
                }

                // Display EAR value
                std::string earText = "EAR: " + std::to_string(ear).substr(0, 5);
                cv::putText(frame, earText, cv::Point(10, 30),
                            cv::FONT_HERSHEY_SIMPLEX, 0.8,
                            cv::Scalar(0, 255, 0), 2);

                // Display drowsiness state
                if (drowsy) {
                    cv::putText(frame, "DROWSY!",
                                cv::Point(10, 70),
                                cv::FONT_HERSHEY_SIMPLEX, 1.0,
                                cv::Scalar(0, 0, 255), 3);
                    playAlarmSound();
                    alarmPlaying = true;
                }
            }
        }

        cv::imshow("Drowsiness / EAR", frame);

        int key = cv::waitKey(30);
        if (key == 27) break;      // ESC
        if (key == 's') cv::imwrite("snapshot.png", frame);
    }

    return 0;
}
