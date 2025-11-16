#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <iostream>
#include <cstdlib>
#include <cmath>

// EAR for single eye (6 landmarks)
double eye_aspect_ratio(const std::vector<cv::Point2f>& eye) {
    double A = cv::norm(eye[1] - eye[5]);
    double B = cv::norm(eye[2] - eye[4]);
    double C = cv::norm(eye[0] - eye[3]);
    return (A + B) / (2.0 * C);
}

void playAlarmSound() {
    std::system("say 'Wake up! Pay attention!' &");
}

// Approx horizontal gaze offset: how far eyes are horizontally from frame center (normalized)
double gaze_horizontal_offset(const std::vector<cv::Point2f>& leftEye,
                              const std::vector<cv::Point2f>& rightEye,
                              int frameWidth) {
    if (leftEye.empty() || rightEye.empty() || frameWidth <= 0) return 0.0;

    cv::Point2f leftCenter(0.f, 0.f), rightCenter(0.f, 0.f);

    for (const auto& p : leftEye)  leftCenter  += p;
    for (const auto& p : rightEye) rightCenter += p;

    leftCenter  *= (1.0f / static_cast<float>(leftEye.size()));
    rightCenter *= (1.0f / static_cast<float>(rightEye.size()));

    cv::Point2f midEye = (leftCenter + rightCenter) * 0.5f;

    double centerX = frameWidth / 2.0;
    double offsetPixels = midEye.x - centerX;   // + = right, - = left
    return offsetPixels / frameWidth;           // normalize
}

// Approx head rotation metric based on nose vs eye midpoint (2D yaw-ish)
// > 0 → nose right of eye midpoint, < 0 → nose left
double head_rotation_metric(const std::vector<cv::Point2f>& leftEye,
                            const std::vector<cv::Point2f>& rightEye,
                            const cv::Point2f& noseTip) {
    if (leftEye.empty() || rightEye.empty()) return 0.0;

    cv::Point2f leftCorner  = leftEye.front();        // landmark 36
    cv::Point2f rightCorner = rightEye[3];            // landmark 45 (index 3 in rightEye vector)

    double interEyeDist = cv::norm(rightCorner - leftCorner);
    if (interEyeDist <= 1e-6) return 0.0;

    // Midpoint between eyes
    cv::Point2f midEye = (leftCorner + rightCorner) * 0.5f;

    // Positive if nose is to the right of eye-mid, negative if left
    double dx = noseTip.x - midEye.x;
    return dx / interEyeDist;     // normalized by eye distance
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

    // Performance-friendly resolution
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    // Drowsiness parameters
    const double EAR_THRESH = 0.25;           // tweak for your face/cam
    const int EAR_CONSEC_FRAMES = 15;         // frames with low EAR → drowsy

    // Distraction (gaze) parameters
    const double GAZE_OFFSET_THRESH = 0.25;   // how far from center is "too far"
    const int DISTRACT_CONSEC_FRAMES = 10;    // frames beyond threshold → distracted

    // Distraction (head rotation) parameters
    const double ROTATION_THRESH = 0.1;      // normalized nose shift, ~0.3–0.4 is pretty turned
    const int ROTATE_CONSEC_FRAMES = 30;      // frames with strong rotation → distracted

    int lowEarFrameCount      = 0;
    int gazeDistractCount     = 0;
    int rotateDistractCount   = 0;

    bool drowsy     = false;
    bool distracted = false;
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
            // pick the largest face (assume that's the driver)
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
                if (pts.size() >= 68) {
                    // Extract left and right eye points
                    std::vector<cv::Point2f> leftEye{
                        pts[36], pts[37], pts[38], pts[39], pts[40], pts[41]
                    };
                    std::vector<cv::Point2f> rightEye{
                        pts[42], pts[43], pts[44], pts[45], pts[46], pts[47]
                    };
                    cv::Point2f noseTip = pts[30];

                    double leftEAR  = eye_aspect_ratio(leftEye);
                    double rightEAR = eye_aspect_ratio(rightEye);
                    double ear      = (leftEAR + rightEAR) / 2.0;

                    // Draw face box
                    cv::rectangle(frame, bestFace, cv::Scalar(255, 0, 0), 2);

                    // Draw eye landmarks
                    for (const auto& p : leftEye)
                        cv::circle(frame, p, 2, cv::Scalar(0, 255, 255), cv::FILLED);
                    for (const auto& p : rightEye)
                        cv::circle(frame, p, 2, cv::Scalar(0, 255, 255), cv::FILLED);

                    // Draw nose tip
                    cv::circle(frame, noseTip, 3, cv::Scalar(0, 0, 255), cv::FILLED);

                    // --- DROWSINESS: EAR-based ---
                    if (ear < EAR_THRESH) {
                        lowEarFrameCount++;
                    } else {
                        lowEarFrameCount = 0;
                        drowsy = false;
                    }
                    if (lowEarFrameCount >= EAR_CONSEC_FRAMES) {
                        drowsy = true;
                    }

                    // --- DISTRACTION 1: gaze offset-based ---
                    double gazeOffset = gaze_horizontal_offset(leftEye, rightEye, frame.cols);
                    bool gazeFar = std::fabs(gazeOffset) > GAZE_OFFSET_THRESH;
                    if (gazeFar) {
                        gazeDistractCount++;
                    } else {
                        gazeDistractCount = 0;
                    }
                    bool gazeDistracted = (gazeDistractCount >= DISTRACT_CONSEC_FRAMES);

                    // --- DISTRACTION 2: head rotation-based ---
                    double rotationMetric = head_rotation_metric(leftEye, rightEye, noseTip);
                    bool rotatedFar = std::fabs(rotationMetric) > ROTATION_THRESH;
                    if (rotatedFar) {
                        rotateDistractCount++;
                    } else {
                        rotateDistractCount = 0;
                    }
                    bool rotatedDistracted = (rotateDistractCount >= ROTATE_CONSEC_FRAMES);

                    // Combine distraction conditions
                    distracted = gazeDistracted || rotatedDistracted;

                    // Display EAR, gaze and rotation metrics
                    std::string earText = "EAR: " + std::to_string(ear).substr(0, 5);
                    cv::putText(frame, earText, cv::Point(10, 30),
                                cv::FONT_HERSHEY_SIMPLEX, 0.7,
                                cv::Scalar(0, 255, 0), 2);

                    std::string gazeText = "Gaze offset: " +
                                           std::to_string(gazeOffset).substr(0, 6);
                    cv::putText(frame, gazeText, cv::Point(10, 60),
                                cv::FONT_HERSHEY_SIMPLEX, 0.7,
                                cv::Scalar(255, 255, 0), 2);

                    std::string rotText = "Head rot: " +
                                          std::to_string(rotationMetric).substr(0, 6);
                    cv::putText(frame, rotText, cv::Point(10, 90),
                                cv::FONT_HERSHEY_SIMPLEX, 0.7,
                                cv::Scalar(0, 255, 255), 2);

                    // States
                    if (drowsy) {
                        cv::putText(frame, "DROWSY",
                                    cv::Point(10, 130),
                                    cv::FONT_HERSHEY_SIMPLEX, 1.0,
                                    cv::Scalar(0, 0, 255), 3);
                    }
                    if (distracted) {
                        cv::putText(frame, "DISTRACTED",
                                    cv::Point(10, 170),
                                    cv::FONT_HERSHEY_SIMPLEX, 1.0,
                                    cv::Scalar(0, 0, 255), 3);
                    }

                    // --- Alarm: fire if drowsy OR distracted ---
                    bool alert = drowsy || distracted;
                    if (alert && !alarmPlaying) {
                        playAlarmSound();
                        alarmPlaying = true;
                    } else if (!alert) {
                        alarmPlaying = false;
                    }
                }
            }
        }

        cv::imshow("Driver Monitoring (EAR + Gaze + Rotation)", frame);

        int key = cv::waitKey(30);
        if (key == 27) break;      // ESC
        if (key == 's') cv::imwrite("snapshot.png", frame);
    }

    return 0;
}
