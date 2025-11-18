#include "level.h"


Level::Level(int num, int target, const std::string& levelName, 
             const std::string& music, const std::string& bgImage,
             Color borderCol, float thickness, bool showBorder)
    : levelNumber(num), scoreTarget(target), name(levelName),
      bgMusicFile(music), bgImageFile(bgImage), 
      borderColor(borderCol), borderThickness(thickness), hasBorder(showBorder) {}


void Level::addObstacle(int x, int y) {
    obstacles.push_back({x, y});
}


void Level::clearObstacles() {
    obstacles.clear();
}

// Setup default obstacles for each level
void Level::setupDefaultObstacles(int playfieldStartRow) {
    //TODO: Change obstacle layout in level 2 and 3
    
    clearObstacles();
    
    if (levelNumber == 2) {
        
        for (int i = 5; i < 15; i++) {
            addObstacle(i, playfieldStartRow + 7);  // Add offset
        }
        for (int i = 25; i < 35; i++) {
            addObstacle(i, playfieldStartRow + 17); // Add offset
        }

        for(int i = 5; i < 15;i++){
            addObstacle(i, playfieldStartRow + 17);
        }

        for(int i = 25; i < 35;i++){
            addObstacle(i, playfieldStartRow + 7);
        }
        
    } else if (levelNumber == 3) {
        
        // Top-left corner
        for (int i = 2; i < 8; i++) {
            for (int j = playfieldStartRow + 2; j < playfieldStartRow + 6; j++) {
                addObstacle(i, j);
            }
        }
        // Top-right corner
        for (int i = 32; i < 38; i++) {
            for (int j = playfieldStartRow + 2; j < playfieldStartRow + 6; j++) {
                addObstacle(i, j);
            }
        }
        // Bottom-left corner
        for (int i = 2; i < 8; i++) {
            for (int j = 24; j < 28; j++) {
                addObstacle(i, j);
            }
        }
        // Bottom-right corner
        for (int i = 32; i < 38; i++) {
            for (int j = 24; j < 28; j++) {
                addObstacle(i, j);
            }
        }
    }
}

// Check if obstacle exists at position
bool Level::isObstacleAt(int x, int y) const {
    for (const auto& obs : obstacles) {
        if (obs.x == x && obs.y == y) return true;
    }
    return false;
}