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
void Level::setupDefaultObstacles(int PLAYFIELD_START_ROW) {
    //TODO: Change obstacle layout in level 2 and 3
    
    clearObstacles();
    
    if (levelNumber == 2) {
        
        for (int i = 7; i < 32; i++) {
            addObstacle(i, PLAYFIELD_START_ROW + 9); 
        }
        

        for(int i = 7; i < 32;i++){
            addObstacle(i, PLAYFIELD_START_ROW + 17);
        }

        
        
    } else if (levelNumber == 3) {

        //Left vertical wall
        for(int j = PLAYFIELD_START_ROW+1; j<PLAYFIELD_START_ROW+16; j++){
            addObstacle(14, j);
        }

        //Right-Bottom Horizontal wall
        for(int i = 1; i< 15; i++){
            addObstacle(i, PLAYFIELD_START_ROW+20);
        }

        //Top Right Horizontal wall
        for(int i = 25; i<39;i++){
            addObstacle(i, PLAYFIELD_START_ROW+6);
        }

        //Right vertical wall
        for(int j = PLAYFIELD_START_ROW + 11; j < 29; j++){
            addObstacle(25, j);
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