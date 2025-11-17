#ifndef LEVEL_H
#define LEVEL_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

using namespace sf;

// Obstacle structure
struct Obstacle {
    int x, y;
};

// Level class to encapsulate level-specific data
class Level {
public:
    int levelNumber;
    int scoreTarget;
    std::string name;
    std::vector<Obstacle> obstacles;
    std::string bgMusicFile;
    std::string bgImageFile;
    Color borderColor;
    float borderThickness;
    bool hasBorder;
    
    // Constructor
    Level(int num, int target, const std::string& levelName, 
          const std::string& music, const std::string& bgImage,
          Color borderCol = Color::Green, float thickness = 4.0f, bool showBorder = true);
    
    // Add obstacle to level
    void addObstacle(int x, int y);
    
    // Clear all obstacles
    void clearObstacles();
    
    // Setup default obstacles for each level
    void setupDefaultObstacles(int playfieldStartRow);
    
    // Check if obstacle exists at position
    bool isObstacleAt(int x, int y) const;
};

#endif