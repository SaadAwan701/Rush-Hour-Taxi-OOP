# Rush Hour Taxi Simulation 🚕

A fully playable 2D grid-based arcade game developed in **C++** using the **OpenGL/GLUT** library. This project was built to demonstrate core Object-Oriented Programming principles, focusing on clean architecture and state management.

## 🧠 Architectural Highlights
* **Strict Encapsulation:** All game state, scoring, and entity management are encapsulated within a centralized `GameEngine` class to prevent global state mutation.
* **OOP & Inheritance:** Utilized inheritance and polymorphism for dynamic game entities (e.g., separating `Vehicle` physics from `Car` controls, and managing distinct `Passenger` and `Parcel` behaviors via a `Target` interface).
* **Singleton API Wrapper:** Successfully bridged modern C++ OOP design with the C-based GLUT API by routing all display and timer callbacks through a single, globally accessible engine instance.
* **Dynamic Memory & Collision:** Implemented real-time AABB (Axis-Aligned Bounding Box) collision detection for procedurally spawned traffic and obstacles.

## 🎮 Features
* Two distinct game modes: Taxi Driver and Delivery Driver.
* Real-time traffic AI with randomized movement and collision logic.
* Fuel management, penalty cooldowns, and a functioning high-score leaderboard.

## 🚀 How to Run (Linux/Ubuntu)
1. Ensure OpenGL and GLUT are installed (`sudo apt-get install freeglut3-dev`).
2. Run `make` to compile the source code.
3. Execute `./game` to start.