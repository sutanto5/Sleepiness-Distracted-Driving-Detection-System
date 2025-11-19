# Sleepiness & Distracted Driving Detection System

A real-time multimodal safety system that detects **driver drowsiness** and **distracted driving** using computer vision, physiological signals, and haptic alerts. Developed for **NeuroHacks – Fall 2025**, then expanded into a standalone C++/OpenCV project.

Project Description Link: https://www.canva.com/design/DAG40b61ie4/w3XLhRhBflYMHmtVIe6W3Q/edit?utm_content=DAG40b61ie4&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton
---

## 🚗 Overview

This project leverages facial landmark tracking, heart-rate monitoring, and behavioral metrics to identify dangerous driving states. It alerts the driver through haptic feedback, helping prevent accidents caused by fatigue or inattention.

Core components:
- **Facial detection & tracking**
- **Heart-rate monitoring (Garmin ECG)**
- **Haptic feedback alerts**
- **Real-time C++ inference**

---

## ✨ Features

### 🧠 Vision-Based Detection
- **Eye Aspect Ratio (EAR)** for drowsiness  
  - Alarm if EAR < 0.25 for 15 frames
- **Gaze Offset** for visual distraction  
  - Alarm if offset > 0.25 for 10 frames
- **Head Rotation** for cognitive distraction  
  - Alarm if rotation > 0.1 for 30 frames

### ❤️ Heart-Rate Monitoring (Garmin ECG)
- Real-time HR + HRV tracking
- HR drop/spike detection for early fatigue detection
- Built using **Monkey C** and the **Garmin SDK**
- Custom activity creation for faster sampling

### 🔔 Haptic Alerts
- Vibrations trigger instantly when unsafe conditions are detected

---

## 🧰 Tech Stack

- **C++**
- **OpenCV (Haar Cascade + LBF Facial Landmarks)**
- **CMake**
- **Monkey C**
- **Garmin ECG Sensor + SDK**

---

## 📂 Repository Structure

Sleepiness-Distracted-Driving-Detection-System/
│
├── drowsy.cpp # Drowsiness detection
├── distracted.cpp # Distraction detection
├── models/ # Haar cascades + LBF models
├── garmin55/ # Garmin ECG/HR monitoring code
├── build/ # Build artifacts
├── CMakeLists.txt # Build configuration


---

## 🧪 Detection Logic

### EAR (Eye Aspect Ratio)
Based on vertical vs. horizontal eyelid landmark distances.  
**Trigger:** EAR < 0.25 for 15+ frames.

### Gaze Offset
Measures deviation of the face from the center of the frame.  
**Trigger:** offset > 0.25 for 10+ frames.

### Head Rotation
Yaw/Pitch estimation using facial landmarks.  
**Trigger:** rotation > 0.1 for 30+ frames.

### Heart-Rate Based Fatigue Detection
- HR drop → possible drowsiness  
- HR spike → stress response  
Used as a **predictive layer** before visual fatigue appears.

---

## 🛠️ Getting Started

### Prerequisites
- C++11+
- OpenCV 4.x
- CMake 3.0+
- Webcam
- (Optional) Garmin ECG Sensor

### Build Instructions

```bash
git clone https://github.com/sutanto5/Sleepiness-Distracted-Driving-Detection-System
cd Sleepiness-Distracted-Driving-Detection-System
mkdir build && cd build
cmake ..
make
