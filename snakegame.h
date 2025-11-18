#ifndef SNAKEGAME_H
#define SNAKEGAME_H

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <deque>
#include <string>
#include "level.h"

using namespace sf;

// Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int GRID_SIZE = 20;
const int GRID_WIDTH = WINDOW_WIDTH / GRID_SIZE;
const int GRID_HEIGHT = WINDOW_HEIGHT / GRID_SIZE;

const int UI_ROWS = 3;  
const int PLAYFIELD_START_ROW = UI_ROWS;  
const int PLAYFIELD_HEIGHT = GRID_HEIGHT - UI_ROWS;
const int UI_AREA_HEIGHT = UI_ROWS * GRID_SIZE; 

// Enums
enum Direction { UP, DOWN, LEFT, RIGHT };
enum GameState { MAIN_MENU, PLAYING, GAME_OVER, LEADERBOARD, SETTINGS, LEVEL_TRANSITION, PAUSED, LEVEL_SELECT };

// Structures
struct SnakeSegment {
    int x, y;
};

struct Food {
    int x, y;
    bool isBonus;
    Clock spawnTimer;
};

// Main game class
class SnakeGame {
private:
    // Window and font
    RenderWindow window;
    Font font;
    
    // Background textures and sprites
    Texture menuBgTexture, transitionBgTexture, pauseBgTexture, leaderboardBgTexture, settingsBgTexture, uiAreaBgTexture,levelSelectBgTexture;
    Sprite menuBg, transitionBg, pauseBg, leaderboardBg, settingsBg, uiAreaBg, levelSelectBg;
    
    // Level system
    std::vector<Level> levels;
    int currentLevelIndex;
    Texture currentLevelBgTexture;
    Sprite currentLevelBg;
    Music currentBgMusic;
    
    // Game state
    GameState state;
    bool isFreePlay;
    bool isLevelSelectMode;
    
    // Snake
    std::deque<SnakeSegment> snake;
    Direction direction;
    Direction nextDirection;
    
    // Food
    Food food;
    Food bonusFood;
    Clock foodTimer;
    bool bonusFoodActive;
    Clock bonusFoodTimer;
    Clock bonusFoodSpawnTimer;
    float bonusFoodDuration;
    
    // Scoring
    int score;
    float scoreMultiplier;
    float speedMultiplier;
    Clock consecutiveTimer;
    bool consecutiveActive;
    
    // Game timing
    Clock gameClock;
    float moveInterval;
    
    // Audio
    SoundBuffer eatBuffer, bonusBuffer, collisionBuffer, gameOverBuffer, levelCompleteBuffer;
    Sound eatSound, bonusSound, collisionSound, gameOverSound, levelCompleteSound;
    bool soundEnabled;
    
    // Leaderboard
    std::vector<std::pair<std::string, int>> leaderboard;
    std::vector<std::pair<std::string, int>> freePlayLeaderboard;
    std::string playerName;
    bool enteringName;
    int leaderboardTab;
    
    // Transition
    Clock transitionClock;
    int nextLevelIndex;
    
    // Pause
    GameState previousState;
    
    // Private methods
    void initializeLevels();
    void initAudio();
    void loadLevelAssets();
    void playBgMusic();
    void resetLevel();
    Level& getCurrentLevel();
    void spawnFood(bool bonus);
    void handleInput();
    void advanceToNextLevel();
    void handleMenuInput(Event& event);
    void handleLevelSelectInput(Event& event);
    void handleGameInput(Event& event);
    void handleNameInput(Event& event);
    void startLevelFromSelect(int levelIndex);
    void update();
    void moveSnake();
    void updateMultipliers();
    void checkLevelCompletion();
    void levelComplete();
    void gameOver();
    void render();
    void renderMainMenu();
    void renderLevelSelect();
    void renderGame();
    void renderLevelTransition();
    void renderPauseOverlay();
    void renderGameOver();
    void renderLeaderboard();
    void renderSettings();
    void loadSettings();
    void saveSettings();
    void loadLeaderboard();
    void saveLeaderboard();
    void addToLeaderboard(const std::string& name, int newScore, bool isFreePlay);

public:
    
    SnakeGame();
    // Main game loop
    void run();
    void startGame(bool freePlay);
};

#endif