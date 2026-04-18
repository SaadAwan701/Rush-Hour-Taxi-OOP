//============================================================================
// Name        : Rush Hour Taxi/Delivery Game
// Author      : Muhammad Saad
// Version     : 2.0
// Copyright   : 
// Description : Basic 2D game of Rush Hour
//============================================================================




#include "util.h"
#include <iostream>
#include <cmath>
#include <fstream>
using namespace std;

const int SCREEN_WIDTH = 1020;
const int SCREEN_HEIGHT = 840;
const int MAX_STATIONS = 2;

enum GameState { MENU, COLOR_SELECTION, NAME_INPUT, RUNNING, GAME_OVER, LEADERBOARD_VIEW };
enum GameMode { TAXI, DELIVERY };

struct MoveResult {
    bool hitTree;
    bool hitPassenger;
    bool moved;

    MoveResult() {
        hitTree = false;
        hitPassenger = false;
        moved = false;
    }
};

struct LeaderboardEntry {
    char name[50];
    int score;
};

class Leaderboard {
private:
    static const int MAX_ENTRIES = 10;
    LeaderboardEntry entries[MAX_ENTRIES];
    int entryCount;
    string filename;

public:
    Leaderboard() {
        entryCount = 0;
        filename = "highscores.dat";
    }

    void save() {
        ofstream fout(filename, ios::binary | ios::trunc);
        if (!fout) {
            cout << "Failed to open file for writing!" << endl;
            return;
        }
        for (int i = 0; i < entryCount; i++) {
            fout.write(reinterpret_cast<const char*>(&entries[i]), sizeof(LeaderboardEntry));
        }
        fout.close();
    }

    void load() {
        ifstream fin(filename, ios::binary);
        if (!fin) {
            cout << "Could not open highscores.dat for reading." << endl;
            return;
        }
        entryCount = 0;
        while (entryCount < MAX_ENTRIES &&
               fin.read(reinterpret_cast<char*>(&entries[entryCount]), sizeof(LeaderboardEntry))) {
            entryCount++;
        }
        fin.close();
    }

    void update(const string& name, int score) {
        load();
        for (int i = 0; i < entryCount; i++) {
            if (entries[i].score == score && string(entries[i].name) == name) {
                return;
            }
        }

        int insertIdx = entryCount;
        for (int i = 0; i < entryCount; i++) {
            if (score > entries[i].score) {
                insertIdx = i;
                break;
            }
        }

        if (entryCount < MAX_ENTRIES || insertIdx < MAX_ENTRIES) {
            for (int j = min(entryCount, MAX_ENTRIES - 1); j > insertIdx; j--) {
                entries[j] = entries[j - 1];
            }
            LeaderboardEntry newEntry;
            strncpy(newEntry.name, name.c_str(), sizeof(newEntry.name) - 1);
            newEntry.name[sizeof(newEntry.name) - 1] = '\0';
            newEntry.score = score;
            entries[insertIdx] = newEntry;
            if (entryCount < MAX_ENTRIES) entryCount++;
            save();
        }
    }

    void display(int x, int y) {
        DrawString(x, y, "Leaderboard (Top 10):", colors[YELLOW]);
        for (int i = 0; i < entryCount; i++) {
            DrawString(x, y - 30 * (i + 1),
                       string(entries[i].name) + ": " + Num2Str(entries[i].score),
                       colors[WHITE]);
        }
    }
};

class MapElement {
public:
    virtual void draw() = 0;
};

class Tree : public MapElement {
private:
    int x, y;

public:
    Tree(int x_, int y_) {
        x = x_;
        y = y_;
    }

    void draw() override {
        DrawRectangle(x + 18, y + 20, 5, 10, colors[BROWN]);
        DrawCircle(x + 20, y + 15, 15, colors[GREEN]);
    }

    int getX() { return x; }
    int getY() { return y; }
};

class Obstacle {
private:
    int x, y, w, h;
    float* color;

public:
    Obstacle() {
        x = 0; y = 0; w = 0; h = 0;
        color = colors[GRAY];
    }

    Obstacle(int x_, int y_, int width, int height, float* c) {
        x = x_; y = y_; w = width; h = height; color = c;
    }

    void draw() {
        DrawRectangle(x, y, w, h, color);
    }

    bool isCollision(int cx, int cy, int cw = 40, int ch = 20) {
        return !(cx + cw < x || cx > x + w || cy + ch < y || cy > y + h);
    }

    int getX() { return x; }
    int getY() { return y; }
    int getW() { return w; }
    int getH() { return h; }
};

class Target {
public:
    virtual void draw() = 0;
    virtual void spawnSafe(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[]) = 0;
    virtual bool isNear(int x, int y) = 0;
    virtual bool isPicked() = 0;
    virtual void pick() = 0;
    virtual void spawnDropLocation(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[], int& dropX, int& dropY, bool& dropZoneVisible) = 0;
    virtual bool isDelivery() = 0;
};

class Passenger : public Target {
private:
    int x, y;
    bool pickedUp;

    bool collidesWithObstacle(int px, int py, Obstacle* obstacles, int obstacleCount) {
        for (int i = 0; i < obstacleCount; i++) {
            if (obstacles[i].isCollision(px, py, 40, 40)) return true;
        }
        return false;
    }

    bool isOverlappingFuelStation(int px, int py, int fuelStationX[], int fuelStationY[]) {
        for (int i = 0; i < MAX_STATIONS; i++) {
            int fx = fuelStationX[i];
            int fy = fuelStationY[i];
            if (!(px + 40 < fx - 5 || px > fx + 65 || py + 40 < fy - 5 || py > fy + 65)) {
                return true;
            }
        }
        return false;
    }

public:
    Passenger(int x_ = 0, int y_ = 0) {
        x = x_; y = y_;
        pickedUp = false;
    }

    bool isDelivery() override { return false; }

    void draw() override {
        if (!pickedUp) {
            DrawCircle(x + 5, y + 20, 7, colors[MAGENTA]);
            DrawLine(x + 5, y + 20, x + 5, y + 5, 2, colors[MAGENTA]);
            DrawLine(x + 5, y + 15, x - 5, y + 10, 2, colors[MAGENTA]);
            DrawLine(x + 5, y + 15, x + 15, y + 10, 2, colors[MAGENTA]);
            DrawLine(x + 5, y + 5, x - 2, y - 5, 2, colors[MAGENTA]);
            DrawLine(x + 5, y + 5, x + 12, y - 5, 2, colors[MAGENTA]);
        }
    }

    bool isNear(int cx, int cy) override {
        return !pickedUp && abs(cx - x) < 40 && abs(cy - y) < 40;
    }

    void pick() override { pickedUp = true; }
    bool isPicked() override { return pickedUp; }

    void spawnSafe(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[]) override {
        do {
            x = GetRandInRange(100, 900);
            y = GetRandInRange(100, 700);
        } while (collidesWithObstacle(x, y, obstacles, obstacleCount) ||
                 isOverlappingFuelStation(x, y, fuelStationX, fuelStationY));
        pickedUp = false;
    }

    void spawnDropLocation(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[], int& dropX, int& dropY, bool& dropZoneVisible) override {
        do {
            dropX = GetRandInRange(100, 900);
            dropY = GetRandInRange(100, 700);
        } while (collidesWithObstacle(dropX, dropY, obstacles, obstacleCount) ||
                 isOverlappingFuelStation(dropX, dropY, fuelStationX, fuelStationY));
        dropZoneVisible = true;
    }

    void respawn(int newX, int newY) {
        x = newX; y = newY; pickedUp = false;
    }

    int getX() { return x; }
    int getY() { return y; }
};

class Parcel : public Target {
private:
    int x, y;
    bool picked;

    bool collidesWithObstacle(int px, int py, Obstacle* obstacles, int obstacleCount) {
        for (int i = 0; i < obstacleCount; i++) {
            if (obstacles[i].isCollision(px, py, 30, 30)) return true;
        }
        return false;
    }

    bool isOverlappingFuelStation(int px, int py, int fuelStationX[], int fuelStationY[]) {
        for (int i = 0; i < MAX_STATIONS; i++) {
            int fx = fuelStationX[i];
            int fy = fuelStationY[i];
            if (!(px + 30 < fx - 5 || px > fx + 65 || py + 30 < fy - 5 || py > fy + 65)) {
                return true;
            }
        }
        return false;
    }

public:
    Parcel() {
        x = 0; y = 0; picked = false;
    }

    bool isDelivery() override { return true; }

    void draw() override {
        if (!picked) {
            DrawRoundRect(x, y, 30, 30, colors[ORANGE], 5);
            DrawString(x + 5, y + 10, "BOX", colors[BLACK]);
        }
    }

    bool isNear(int cx, int cy) override {
        return !picked && abs(cx - x) < 40 && abs(cy - y) < 40;
    }

    void pick() override { picked = true; }
    bool isPicked() override { return picked; }

    void spawnSafe(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[]) override {
        do {
            x = GetRandInRange(100, 900);
            y = GetRandInRange(100, 700);
        } while (collidesWithObstacle(x, y, obstacles, obstacleCount) ||
                 isOverlappingFuelStation(x, y, fuelStationX, fuelStationY));
        picked = false;
    }

    void spawnDropLocation(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[], int& dropX, int& dropY, bool& dropZoneVisible) override {
        do {
            dropX = GetRandInRange(100, 900);
            dropY = GetRandInRange(100, 700);
        } while (collidesWithObstacle(dropX, dropY, obstacles, obstacleCount) ||
                 isOverlappingFuelStation(dropX, dropY, fuelStationX, fuelStationY));
        dropZoneVisible = true;
    }
};

class Vehicle {
private:
    int x, y;
    float* color;
    int dx, dy;
    Obstacle* obstacles;
    int obstacleCount;
    int frameCounter;

    void assignRandomDirection() {
        int possibleDirs[4][2] = { {10, 0}, {-10, 0}, {0, 10}, {0, -10} };
        int shuffled[4] = {0, 1, 2, 3};
        for (int i = 0; i < 4; i++) {
            int r = GetRandInRange(0, 3);
            swap(shuffled[i], shuffled[r]);
        }
        for (int i = 0; i < 4; i++) {
            int ndx = possibleDirs[shuffled[i]][0];
            int ndy = possibleDirs[shuffled[i]][1];
            int newX = x + ndx;
            int newY = y + ndy;
            if (newX < 0 || newX > SCREEN_WIDTH - 40 || newY < 0 || newY > SCREEN_HEIGHT - 20) continue;
            bool willCollide = false;
            for (int j = 0; j < obstacleCount; j++) {
                if (obstacles[j].isCollision(newX, newY)) {
                    willCollide = true;
                    break;
                }
            }
            if (!willCollide) {
                dx = ndx; dy = ndy; return;
            }
        }
        dx = 0; dy = 0;
    }

public:
    Vehicle(int x_, int y_, float* c) {
        x = x_; y = y_; color = c;
        dx = 0; dy = 0;
        obstacles = nullptr;
        obstacleCount = 0;
        frameCounter = 0;
        assignRandomDirection();
    }

    void setupCollisionCheck(Obstacle* obs, int count) {
        obstacles = obs;
        obstacleCount = count;
    }

    void draw() {
        DrawRectangle(x, y, 40, 20, color);
        DrawCircle(x + 5, y - 5, 5, colors[BLACK]);
        DrawCircle(x + 35, y - 5, 5, colors[BLACK]);
    }

    void move() {
        frameCounter++;
        if (frameCounter >= 100) {
            assignRandomDirection();
            frameCounter = 0;
        }
        int newX = x + dx;
        int newY = y + dy;
        bool willCollide = false;
        for (int i = 0; i < obstacleCount; i++) {
            if (obstacles[i].isCollision(newX, newY)) {
                willCollide = true;
                break;
            }
        }
        if (newX < 0 || newX > SCREEN_WIDTH - 40 || newY < 0 || newY > SCREEN_HEIGHT - 20) {
            willCollide = true;
        }
        if (willCollide) assignRandomDirection();
        else { x = newX; y = newY; }
    }

    void speedUp() {
        if (dx > 0) dx += 2;
        else if (dx < 0) dx -= 2;
        if (dy > 0) dy += 2;
        else if (dy < 0) dy -= 2;
    }

    bool isCollision(int cx, int cy) {
        return !(cx + 40 < x || cx > x + 40 || cy + 20 < y || cy > y + 20);
    }

    int getX() { return x; }
    int getY() { return y; }
};

class PassengerManager {
private:
    Passenger passengers[4];
    int count;

public:
    PassengerManager() {
        count = 0;
    }

    void spawnPassengers(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[], int minCount = 2, int maxCount = 4) {
        count = GetRandInRange(minCount, maxCount);
        for (int i = 0; i < count; i++) {
            passengers[i].spawnSafe(obstacles, obstacleCount, fuelStationX, fuelStationY);
        }
    }

    void removePickedPassenger() {
        for (int i = 0; i < count; i++) {
            if (passengers[i].isPicked()) {
                for (int j = i; j < count - 1; j++) {
                    passengers[j] = passengers[j + 1];
                }
                count--;
                return;
            }
        }
    }

    void spawnNewPassenger(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[]) {
        if (count >= 4) return;
        passengers[count].spawnSafe(obstacles, obstacleCount, fuelStationX, fuelStationY);
        count++;
    }

    Passenger* getNear(int x, int y) {
        for (int i = 0; i < count; i++) {
            if (passengers[i].isNear(x, y) && !passengers[i].isPicked()) {
                return &passengers[i];
            }
        }
        return nullptr;
    }

    Passenger* getAt(int index) {
        if (index >= 0 && index < count) return &passengers[index];
        return nullptr;
    }

    int getCount() { return count; }

    void draw() {
        for (int i = 0; i < count; i++) {
            if (!passengers[i].isPicked()) passengers[i].draw();
        }
    }
};

class ParcelManager {
private:
    Parcel parcels[3];
    int count;

public:
    ParcelManager() {
        count = 0;
    }

    void spawnParcels(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[], int minCount = 2, int maxCount = 3) {
        count = GetRandInRange(minCount, maxCount);
        for (int i = 0; i < count; i++) {
            parcels[i].spawnSafe(obstacles, obstacleCount, fuelStationX, fuelStationY);
        }
    }

    void removePickedParcel() {
        for (int i = 0; i < count; i++) {
            if (parcels[i].isPicked()) {
                for (int j = i; j < count - 1; j++) {
                    parcels[j] = parcels[j + 1];
                }
                count--;
                return;
            }
        }
    }

    void spawnNewParcel(Obstacle* obstacles, int obstacleCount, int fuelStationX[], int fuelStationY[]) {
        if (count < 3) {
            parcels[count].spawnSafe(obstacles, obstacleCount, fuelStationX, fuelStationY);
            count++;
        }
    }

    Parcel* getNear(int x, int y) {
        for (int i = 0; i < count; i++) {
            if (parcels[i].isNear(x, y) && !parcels[i].isPicked()) {
                return &parcels[i];
            }
        }
        return nullptr;
    }

    void draw() {
        for (int i = 0; i < count; i++) {
            if (!parcels[i].isPicked()) parcels[i].draw();
        }
    }
};

class Car {
private:
    int x, y;
    float* color;
    Obstacle* obstacles;
    int obstacleCount;
    Vehicle** vehicles;
    int vehicleCount;
    bool isOnTree;
    bool isOnPassenger;
    PassengerManager* passengerManager;
    MapElement** elements;
    int elementCount;

public:
    Car(int x_ = 60, int y_ = 750, float* c = colors[BLUE]) {
        x = x_; y = y_; color = c;
        obstacles = nullptr;
        obstacleCount = 0;
        vehicles = nullptr;
        vehicleCount = 0;
        isOnTree = false;
        isOnPassenger = false;
        passengerManager = nullptr;
        elements = nullptr;
        elementCount = 0;
    }

    void setupCollisionCheck(Obstacle* obs, int obsCount, Vehicle** v, int vCount,
                             PassengerManager* pm, MapElement** e, int elemCount) {
        obstacles = obs;
        obstacleCount = obsCount;
        vehicles = v;
        vehicleCount = vCount;
        passengerManager = pm;
        elements = e;
        elementCount = elemCount;
    }

    void draw() {
        DrawRectangle(x, y, 40, 20, color);
        DrawCircle(x + 5, y - 5, 5, colors[BLACK]);
        DrawCircle(x + 35, y - 5, 5, colors[BLACK]);
    }

    MoveResult move(int dx, int dy, bool hasPassenger, GameMode selectedMode, int& fuel, bool& gameOver) {
        MoveResult result;
        int newX = x + dx;
        int newY = y + dy;

        if (newX < 0 || newX + 40 > SCREEN_WIDTH || newY < 0 || newY + 20 > SCREEN_HEIGHT) {
            return result;
        }

        bool blocked = false;
        for (int i = 0; i < obstacleCount; i++) {
            if (obstacles[i].isCollision(newX, newY)) blocked = true;
        }
        for (int i = 0; i < vehicleCount; i++) {
            if (vehicles[i] && vehicles[i]->isCollision(newX, newY)) blocked = true;
        }

        if (!blocked) {
            x = newX;
            y = newY;
            result.moved = true;

            bool collidedWithTree = false;
            if (elements) {
                for (int i = 0; i < elementCount; i++) {
                    Tree* tree = dynamic_cast<Tree*>(elements[i]);
                    if (tree && abs(newX - tree->getX()) < 25 && abs(newY - tree->getY()) < 25) {
                        collidedWithTree = true;
                        if (!isOnTree) {
                            result.hitTree = true;
                            isOnTree = true;
                        }
                        break;
                    }
                }
            }
            if (!collidedWithTree) isOnTree = false;

            bool collidedWithPassenger = false;
            if (passengerManager) {
                for (int i = 0; i < passengerManager->getCount(); i++) {
                    Passenger* p = passengerManager->getAt(i);
                    if (p && !p->isPicked() && abs(newX - p->getX()) < 25 && abs(newY - p->getY()) < 25) {
                        collidedWithPassenger = true;
                        if (hasPassenger && !isOnPassenger) {
                            result.hitPassenger = true;
                            isOnPassenger = true;
                        }
                        break;
                    }
                }
            }
            if (!collidedWithPassenger) isOnPassenger = false;

            static int moveCounter = 0;
            moveCounter++;
            if (moveCounter >= 3) {
                fuel--;
                moveCounter = 0;
            }
            if (fuel <= 0) gameOver = true;
        }

        return result;
    }

    int getX() { return x; }
    int getY() { return y; }
};

class BuildingManager {
private:
    bool isSafePlacement(int x, int y, int w, int h, Obstacle* obstacles, int obstacleCount,
                         int fuelStationX[], int fuelStationY[], int dropX, int dropY) {
        if (!(x + w < 60 || x > 100 || y + h < 750 || y > 790)) return false;
        if (!(x + w < dropX || x > dropX + 40 || y + h < dropY || y > dropY + 40)) return false;
        for (int i = 0; i < MAX_STATIONS; i++) {
            if (!(x + w < fuelStationX[i] || x > fuelStationX[i] + 60 ||
                  y + h < fuelStationY[i] || y > fuelStationY[i] + 60)) return false;
        }
        for (int i = 0; i < obstacleCount; i++) {
            if (!(x + w < obstacles[i].getX() || x > obstacles[i].getX() + obstacles[i].getW() ||
                  y + h < obstacles[i].getY() || y > obstacles[i].getY() + obstacles[i].getH())) {
                return false;
            }
        }
        return true;
    }

public:
    void generateBlockLines(Obstacle* obstacles, int& count, int fuelStationX[], int fuelStationY[], int dropX, int dropY) {
        const int maxRows = SCREEN_HEIGHT / 40;
        const int maxCols = SCREEN_WIDTH / 40;

        for (int row = 2; row < maxRows - 2 && count < 50; row += 3) {
            if (GetRandInRange(0, 100) < 40) continue;
            int x = GetRandInRange(1, maxCols - 6) * 40;
            int y = row * 40;
            int length = GetRandInRange(4, 7) * 40;
            if (!isSafePlacement(x, y, length, 40, obstacles, count, fuelStationX, fuelStationY, dropX, dropY)) continue;
            obstacles[count++] = Obstacle(x, y, length, 40, colors[BLACK]);
        }

        for (int col = 2; col < maxCols - 2 && count < 50; col += 4) {
            if (GetRandInRange(0, 100) < 40) continue;
            int x = col * 40;
            int y = GetRandInRange(1, maxRows - 6) * 40;
            int height = GetRandInRange(4, 6) * 40;
            if (!isSafePlacement(x, y, 40, height, obstacles, count, fuelStationX, fuelStationY, dropX, dropY)) continue;
            obstacles[count++] = Obstacle(x, y, 40, height, colors[BLACK]);
        }
    }
};

class TreeManager {
private:
    bool isOverlappingFuelStation(int x, int y, int fuelStationX[], int fuelStationY[]) {
        for (int i = 0; i < MAX_STATIONS; i++) {
            int fx = fuelStationX[i];
            int fy = fuelStationY[i];
            if (!(x + 30 < fx - 5 || x > fx + 65 || y + 30 < fy - 5 || y > fy + 65)) {
                return true;
            }
        }
        return false;
    }

public:
    void generateTrees(MapElement** elements, int& elementCount, Obstacle* obstacles, int obstacleCount,
                       int fuelStationX[], int fuelStationY[], int dropX, int dropY) {
        const int maxTrees = 20;
        int attempts = 0;

        for (int x = 60; x <= SCREEN_WIDTH - 60 && elementCount < 50; x += 100) {
            int y = 180;
            if (!isOverlappingFuelStation(x, y, fuelStationX, fuelStationY)) {
                elements[elementCount++] = new Tree(x, y);
            }
        }

        for (int y = 100; y <= SCREEN_HEIGHT - 120 && elementCount < 50; y += 100) {
            int x = 260;
            if (!isOverlappingFuelStation(x, y, fuelStationX, fuelStationY)) {
                elements[elementCount++] = new Tree(x, y);
            }
        }

        while (elementCount < 50 && attempts < 300 && elementCount < maxTrees) {
            int x = GetRandInRange(60, SCREEN_WIDTH - 90);
            int y = GetRandInRange(60, SCREEN_HEIGHT - 90);
            bool overlaps = false;

            for (int i = 0; i < obstacleCount; i++) {
                if (!(x + 30 < obstacles[i].getX() || x > obstacles[i].getX() + obstacles[i].getW() ||
                      y + 30 < obstacles[i].getY() || y > obstacles[i].getY() + obstacles[i].getH())) {
                    overlaps = true;
                    break;
                }
            }

            if (!(x + 30 < 60 || x > 100 || y + 30 < 750 || y > 790)) overlaps = true;
            if (!(x + 30 < dropX || x > dropX + 40 || y + 30 < dropY || y > dropY + 40)) overlaps = true;
            if (isOverlappingFuelStation(x, y, fuelStationX, fuelStationY)) overlaps = true;

            if (!overlaps) {
                elements[elementCount++] = new Tree(x, y);
            }
            attempts++;
        }
    }
};

class GameEngine {
private:
    GameState currentState;
    GameMode selectedMode;

    Car* player;
    float* carColor;

    int score;
    int timeLeft;
    bool gameOver;
    int fuel;
    int wallet;
    int dropCount;

    bool hasPassenger;
    bool dropZoneVisible;
    int dropX, dropY;
    Target* currentTarget;

    Vehicle* traffic[10];
    int trafficCount;

    int fuelStationX[MAX_STATIONS];
    int fuelStationY[MAX_STATIONS];

    MapElement* elements[50];
    int elementCount;

    Obstacle obstacles[50];
    int obstacleCount;

    PassengerManager passengerManager;
    ParcelManager parcelManager;
    Leaderboard leaderboard;

    string currentPlayerName;
    string currentRoleStr;
    string roleChangeMessage;
    int roleMessageTimer;
    int carPenaltyCooldown;
    bool scoreSaved;

    bool isOverlappingFuelStation(int x, int y, int w = 40, int h = 40) {
        for (int i = 0; i < MAX_STATIONS; i++) {
            int fx = fuelStationX[i];
            int fy = fuelStationY[i];
            if (!(x + w < fx - 5 || x > fx + 65 || y + h < fy - 5 || y > fy + 65)) {
                return true;
            }
        }
        return false;
    }

    bool collidesWithObstacle(int x, int y, int w = 40, int h = 40) {
        for (int i = 0; i < obstacleCount; i++) {
            if (obstacles[i].isCollision(x, y, w, h)) return true;
        }
        return false;
    }

    bool isFarFromOthers(int x, int y, int existingCount) {
        for (int i = 0; i < existingCount; i++) {
            int ddx = x - fuelStationX[i];
            int ddy = y - fuelStationY[i];
            if (sqrt(ddx * ddx + ddy * ddy) < 200) return false;
        }
        return true;
    }

    bool isSafeTrafficSpawn(int x, int y) {
        for (int i = 0; i < obstacleCount; i++) {
            if (!(x + 40 < obstacles[i].getX() || x > obstacles[i].getX() + obstacles[i].getW() ||
                  y + 20 < obstacles[i].getY() || y > obstacles[i].getY() + obstacles[i].getH())) {
                return false;
            }
        }
        return true;
    }

    void spawnFuelStation() {
        for (int i = 0; i < MAX_STATIONS; i++) {
            int x, y;
            int attempts = 0;
            do {
                x = GetRandInRange(100, 900);
                y = GetRandInRange(100, 700);
                attempts++;
            } while ((!isFarFromOthers(x, y, i) || collidesWithObstacle(x, y, 60, 60)) && attempts < 100);
            fuelStationX[i] = x;
            fuelStationY[i] = y;
        }
    }

    void drawMap() {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, colors[LIGHT_GRAY]);

        DrawRectangle(0, 200, SCREEN_WIDTH, 80, colors[DARK_GRAY]);
        DrawRectangle(0, 500, SCREEN_WIDTH, 80, colors[DARK_GRAY]);
        DrawRectangle(300, 0, 80, SCREEN_HEIGHT, colors[DARK_GRAY]);
        DrawRectangle(700, 0, 80, SCREEN_HEIGHT, colors[DARK_GRAY]);

        DrawRectangle(0, 0, 80, 80, colors[GRAY]);
        DrawString(5, 40, "GARAGE", colors[WHITE]);

        if (player) {
            int px = player->getX();
            int py = player->getY();
            if (px < 100 && py < 100) {
                DrawString(90, 50, "Press 'P' to switch roles", colors[BLACK]);
            }
        }

        for (int i = 20; i < SCREEN_WIDTH; i += 60) {
            DrawRectangle(i, 240, 30, 5, colors[YELLOW]);
            DrawRectangle(i, 540, 30, 5, colors[YELLOW]);
        }
        for (int i = 20; i < SCREEN_HEIGHT; i += 60) {
            DrawRectangle(340, i, 5, 30, colors[YELLOW]);
            DrawRectangle(740, i, 5, 30, colors[YELLOW]);
        }

        for (int i = 0; i < MAX_STATIONS; i++) {
            DrawRoundRect(fuelStationX[i], fuelStationY[i], 60, 60, colors[ORANGE_RED], 10);
            DrawString(fuelStationX[i] + 5, fuelStationY[i] + 25, "FUEL", colors[BLACK]);
        }

        for (int i = 0; i < elementCount; i++) {
            if (elements[i]) elements[i]->draw();
        }

        if (dropZoneVisible) {
            DrawSquare(dropX, dropY, 40, colors[GREEN]);
            DrawString(dropX - 5, dropY + 45, "DROP", colors[WHITE]);
        }
    }

public:
    GameEngine() {
        currentState = MENU;
        selectedMode = TAXI;
        player = nullptr;
        carColor = colors[BLUE];
        score = 0;
        timeLeft = 180;
        gameOver = false;
        fuel = 100;
        wallet = 0;
        dropCount = 0;
        hasPassenger = false;
        dropZoneVisible = false;
        dropX = 0;
        dropY = 0;
        currentTarget = nullptr;
        trafficCount = 2;
        elementCount = 0;
        obstacleCount = 0;
        roleMessageTimer = 0;
        carPenaltyCooldown = 0;
        scoreSaved = false;
        currentPlayerName = "";
        currentRoleStr = "Taxi Driver";
        roleChangeMessage = "";
        for (int i = 0; i < 10; i++) traffic[i] = nullptr;
        for (int i = 0; i < 50; i++) elements[i] = nullptr;
    }

    void init() {
        InitRandomizer();
        spawnFuelStation();

        BuildingManager buildingManager;
        buildingManager.generateBlockLines(obstacles, obstacleCount, fuelStationX, fuelStationY, dropX, dropY);

        TreeManager treeManager;
        treeManager.generateTrees(elements, elementCount, obstacles, obstacleCount, fuelStationX, fuelStationY, dropX, dropY);

        for (int i = 0; i < 2; i++) {
            traffic[i] = new Vehicle(GetRandInRange(100, 900), GetRandInRange(100, 700), colors[GetRandInRange(0, 100)]);
            traffic[i]->setupCollisionCheck(obstacles, obstacleCount);
        }
    }

    void render() {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        if (currentState == MENU) {
            DrawString(300, 770, "RUSH HOUR TAXI GAME", colors[ORANGE_RED]);
            DrawString(350, 740, "BY SAAD MALIK", colors[CYAN]);
            DrawString(380, 530, "1 - Taxi Mode", colors[YELLOW]);
            DrawString(380, 500, "2 - Delivery Mode", colors[YELLOW]);
            DrawString(380, 470, "R - Random Role", colors[YELLOW]);
            DrawString(380, 440, "Press L - Leaderboard", colors[WHITE]);
            glutSwapBuffers();
            return;
        }

        if (currentState == COLOR_SELECTION) {
            DrawString(360, 500, "Pick Car Color", colors[YELLOW]);
            DrawString(360, 470, "R - Red | G - Green | B - Blue | Y - Yellow", colors[YELLOW]);
            glutSwapBuffers();
            return;
        }

        if (currentState == NAME_INPUT) {
            DrawString(360, 500, "Enter your name:", colors[YELLOW]);
            DrawString(360, 470, currentPlayerName.c_str(), colors[WHITE]);
            DrawString(360, 440, "Press Enter to continue", colors[WHITE]);
            glutSwapBuffers();
            return;
        }

        if (currentState == LEADERBOARD_VIEW) {
            leaderboard.display(360, 500);
            DrawString(360, 600, "Press M to return to Menu", colors[WHITE]);
            glutSwapBuffers();
            return;
        }

        drawMap();

        for (int i = 0; i < obstacleCount; i++) obstacles[i].draw();
        for (int i = 0; i < trafficCount; i++) {
            if (traffic[i]) traffic[i]->draw();
        }

        if (selectedMode == TAXI) passengerManager.draw();
        else parcelManager.draw();

        if (player) player->draw();

        DrawString(30, 800, "Score: " + Num2Str(score), colors[RED]);
        DrawString(30, 770, "Time: " + Num2Str(timeLeft) + "s", colors[RED]);
        DrawString(30, 740, "Fuel: " + Num2Str(fuel) + "%", colors[CYAN]);
        DrawString(800, 800, "Role: " + currentRoleStr, colors[BLACK]);
        DrawRectangle(390, 805, 370, 35, colors[BLACK]);
        DrawString(400, 810, "MUHAMMAD SAAD - i240058", colors[WHITE]);

        if (roleMessageTimer > 0) {
            DrawRectangle(390, 395, 250, 25, colors[BLACK]);
            DrawString(400, 400, roleChangeMessage, colors[YELLOW]);
        }

        if (gameOver) {
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            DrawString(400, 500, "GAME OVER", colors[RED]);
            if (score >= 100) {
                DrawString(360, 360, "CONGRATULATIONS! You Won!", colors[GREEN]);
            }
            DrawString(380, 460, "Final Score: " + Num2Str(score), colors[YELLOW]);
            DrawString(360, 420, "Press ESC to exit", colors[WHITE]);
            DrawString(360, 380, "Press R to restart", colors[WHITE]);

            if (!scoreSaved) {
                leaderboard.update(currentPlayerName, score);
                scoreSaved = true;
            }
        }

        glutSwapBuffers();
    }

    void updateTimer(int value) {
        if (!gameOver) {
            for (int i = 0; i < MAX_STATIONS; i++) {
                if (player && abs(player->getX() - fuelStationX[i]) < 50 &&
                    abs(player->getY() - fuelStationY[i]) < 50 && fuel < 100) {
                    fuel += 2;
                    if (fuel > 100) fuel = 100;
                }
            }

            if (roleMessageTimer > 0) roleMessageTimer--;

            for (int i = 0; i < trafficCount; i++) {
                if (traffic[i]) {
                    traffic[i]->move();
                    if (player && traffic[i]->isCollision(player->getX(), player->getY())) {
                        if (carPenaltyCooldown == 0) {
                            if (selectedMode == TAXI) score -= 3;
                            else if (selectedMode == DELIVERY) score -= 5;
                            if (score < 0) gameOver = true;
                            carPenaltyCooldown = 10;
                        }
                    }
                }
            }

            static int frameCount = 0;
            frameCount++;
            if (frameCount >= 10) {
                timeLeft--;
                frameCount = 0;
                if (timeLeft <= 0) gameOver = true;
            }

            if (carPenaltyCooldown > 0) carPenaltyCooldown--;
        }

        glutPostRedisplay();
        // glutTimerFunc(100, ::Timer, 0);
    }

    void handleNonPrintableKey(int key, int x, int y) {
        if (player && currentState == RUNNING) {
            MoveResult result;
            if (key == GLUT_KEY_LEFT)  result = player->move(-10, 0, hasPassenger, selectedMode, fuel, gameOver);
            if (key == GLUT_KEY_RIGHT) result = player->move(10, 0, hasPassenger, selectedMode, fuel, gameOver);
            if (key == GLUT_KEY_UP)    result = player->move(0, 10, hasPassenger, selectedMode, fuel, gameOver);
            if (key == GLUT_KEY_DOWN)  result = player->move(0, -10, hasPassenger, selectedMode, fuel, gameOver);

            if (result.hitTree) {
                score += (selectedMode == TAXI) ? -2 : -4;
                if (score < 0) gameOver = true;
            }
            if (result.hitPassenger) {
                score += (selectedMode == TAXI) ? -5 : -8;
                if (score < 0) gameOver = true;
            }
        }
        glutPostRedisplay();
    }

    void handlePrintableKey(unsigned char key, int x, int y) {
        if (key == 27) exit(1);

        if (gameOver && (key == 'r' || key == 'R')) {
            score = 0;
            timeLeft = 180;
            fuel = 100;
            gameOver = false;
            hasPassenger = false;
            dropZoneVisible = false;
            dropCount = 0;
            wallet = 0;
            scoreSaved = false;

            passengerManager.spawnPassengers(obstacles, obstacleCount, fuelStationX, fuelStationY);
            parcelManager.spawnParcels(obstacles, obstacleCount, fuelStationX, fuelStationY);
            spawnFuelStation();

            trafficCount = 2;
            for (int i = 0; i < trafficCount; i++) {
                int tx, ty;
                do {
                    tx = GetRandInRange(100, 900);
                    ty = GetRandInRange(100, 700);
                } while (!isSafeTrafficSpawn(tx, ty));
                traffic[i] = new Vehicle(tx, ty, colors[GetRandInRange(0, 100)]);
                traffic[i]->setupCollisionCheck(obstacles, obstacleCount);
            }

            if (player) delete player;
            player = new Car(60, 750, carColor);
            player->setupCollisionCheck(obstacles, obstacleCount, traffic, trafficCount,
                                        &passengerManager, elements, elementCount);
            currentTarget = nullptr;
            currentRoleStr = (selectedMode == TAXI) ? "Taxi Driver" : "Delivery Driver";
            currentState = RUNNING;
            glutPostRedisplay();
            return;
        }

        if (gameOver) return;

        if (currentState == MENU) {
            if (key == '1') {
                selectedMode = TAXI;
                currentState = NAME_INPUT;
            } else if (key == '2') {
                selectedMode = DELIVERY;
                currentState = NAME_INPUT;
            } else if (key == 'r' || key == 'R') {
                selectedMode = (GetRandInRange(0, 1) == 0) ? TAXI : DELIVERY;
                currentState = NAME_INPUT;
            } else if (key == 'l' || key == 'L') {
                leaderboard.load();
                currentState = LEADERBOARD_VIEW;
            }
            return;
        }

        if (currentState == NAME_INPUT) {
            if (key == 13 && !currentPlayerName.empty()) {
                currentState = COLOR_SELECTION;
            } else if (key == 8 || key == 127) {
                if (!currentPlayerName.empty()) currentPlayerName.pop_back();
            } else if (isprint(key) && currentPlayerName.length() < 20) {
                currentPlayerName += key;
            }
            glutPostRedisplay();
            return;
        }

        if (currentState == COLOR_SELECTION) {
            if (key == 'r') carColor = colors[RED];
            else if (key == 'g') carColor = colors[GREEN];
            else if (key == 'b') carColor = colors[BLUE];
            else if (key == 'y') carColor = colors[YELLOW];

            currentState = RUNNING;
            currentRoleStr = (selectedMode == TAXI) ? "Taxi Driver" : "Delivery Driver";

            passengerManager.spawnPassengers(obstacles, obstacleCount, fuelStationX, fuelStationY);
            parcelManager.spawnParcels(obstacles, obstacleCount, fuelStationX, fuelStationY);

            player = new Car(60, 750, carColor);
            spawnFuelStation();

            if (player) {
                player->setupCollisionCheck(obstacles, obstacleCount, traffic, trafficCount,
                                            &passengerManager, elements, elementCount);
            }
            return;
        }

        if (currentState == LEADERBOARD_VIEW && (key == 'm' || key == 'M')) {
            currentState = MENU;
            return;
        }

        if ((key == 'p' || key == 'P') && player) {
            if (player->getX() < 80 && player->getY() < 80) {
                selectedMode = (selectedMode == TAXI) ? DELIVERY : TAXI;
                roleChangeMessage = "Role changed! New role: " + string((selectedMode == TAXI) ? "TAXI" : "DELIVERY");
                roleMessageTimer = 50;
                currentRoleStr = (selectedMode == TAXI) ? "Taxi Driver" : "Delivery Driver";
            }
        }

        if (currentState == RUNNING && key == ' ') {
            Passenger* nearPassenger = passengerManager.getNear(player->getX(), player->getY());
            Parcel* nearParcel = parcelManager.getNear(player->getX(), player->getY());

            if (selectedMode == TAXI && nearPassenger && !hasPassenger) {
                nearPassenger->pick();
                hasPassenger = true;
                nearPassenger->spawnDropLocation(obstacles, obstacleCount, fuelStationX, fuelStationY, dropX, dropY, dropZoneVisible);
                currentTarget = nearPassenger;

            } else if (selectedMode == DELIVERY && nearParcel && !hasPassenger) {
                nearParcel->pick();
                hasPassenger = true;
                nearParcel->spawnDropLocation(obstacles, obstacleCount, fuelStationX, fuelStationY, dropX, dropY, dropZoneVisible);
                currentTarget = nearParcel;

            } else if (player && hasPassenger &&
                       abs(player->getX() - dropX) < 40 &&
                       abs(player->getY() - dropY) < 40) {

                hasPassenger = false;
                dropZoneVisible = false;
                score += (currentTarget->isDelivery() ? 20 : 10);
                wallet += score;

                if (currentTarget->isDelivery()) {
                    parcelManager.removePickedParcel();
                    parcelManager.spawnNewParcel(obstacles, obstacleCount, fuelStationX, fuelStationY);
                } else {
                    passengerManager.removePickedPassenger();
                    passengerManager.spawnNewPassenger(obstacles, obstacleCount, fuelStationX, fuelStationY);
                }

                dropCount++;

                if (dropCount % 2 == 0 && trafficCount < 10) {
                    int tx, ty;
                    do {
                        tx = GetRandInRange(100, 900);
                        ty = GetRandInRange(100, 700);
                    } while (!isSafeTrafficSpawn(tx, ty));

                    traffic[trafficCount] = new Vehicle(tx, ty, colors[GetRandInRange(0, 100)]);
                    traffic[trafficCount]->setupCollisionCheck(obstacles, obstacleCount);
                    trafficCount++;

                    for (int i = 0; i < trafficCount; i++) {
                        if (traffic[i]) traffic[i]->speedUp();
                    }

                    player->setupCollisionCheck(obstacles, obstacleCount, traffic, trafficCount,
                                                &passengerManager, elements, elementCount);
                }
            }
        }

        glutPostRedisplay();
    }
};

GameEngine engine;

void GameDisplay() { engine.render(); }
void Timer(int v) { engine.updateTimer(v);
glutTimerFunc(100, Timer, 0);
}
void NonPrintableKeys(int k, int x, int y) { engine.handleNonPrintableKey(k, x, y); }
void PrintableKeys(unsigned char k, int x, int y) { engine.handlePrintableKey(k, x, y); }

void SetCanvasSize(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char* argv[]) {
    engine.init();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(50, 50);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutCreateWindow("Rush Hour — My OOP project :)");

    SetCanvasSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutDisplayFunc(GameDisplay);
    glutSpecialFunc(NonPrintableKeys);
    glutKeyboardFunc(PrintableKeys);
    glutTimerFunc(100, Timer, 0);

    glutMainLoop();
    return 0;
}
