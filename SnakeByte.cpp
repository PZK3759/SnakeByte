#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <deque>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "json.hpp"

using json = nlohmann::json;
using namespace sf;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int GRID_SIZE = 20;
const int GRID_WIDTH = WINDOW_WIDTH / GRID_SIZE;
const int GRID_HEIGHT = WINDOW_HEIGHT / GRID_SIZE;

enum Direction { UP, DOWN, LEFT, RIGHT };
enum GameState { MAIN_MENU, PLAYING, GAME_OVER, LEADERBOARD, SETTINGS, LEVEL_TRANSITION, PAUSED };

struct SnakeSegment {
    int x, y;
};

struct Food {
    int x, y;
    bool isBonus;
    Clock spawnTimer;
};

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
    
    Level(int num, int target, const std::string& levelName, 
          const std::string& music, const std::string& bgImage,
          Color borderCol = Color::Green, float thickness = 4.0f, bool showBorder = true)
        : levelNumber(num), scoreTarget(target), name(levelName),
          bgMusicFile(music), bgImageFile(bgImage), 
          borderColor(borderCol), borderThickness(thickness), hasBorder(showBorder) {}
    
    void addObstacle(int x, int y) {
        obstacles.push_back({x, y});
    }
    
    void clearObstacles() {
        obstacles.clear();
    }
    
    // Setup default obstacles for each level
    void setupDefaultObstacles() {
        clearObstacles();
        
        if (levelNumber == 2) {
            // Level 2: Single horizontal wall (away from center spawn)
            for (int i = 5; i < 15; i++) {
                addObstacle(i, 10);
            }
            for (int i = 25; i < 35; i++) {
                addObstacle(i, 20);
            }
        } else if (levelNumber == 3) {
            // Level 3: Corner obstacles (avoid center)
            // Top-left corner
            for (int i = 2; i < 8; i++) {
                for (int j = 2; j < 6; j++) {
                    addObstacle(i, j);
                }
            }
            // Top-right corner
            for (int i = 32; i < 38; i++) {
                for (int j = 2; j < 6; j++) {
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
    
    bool isObstacleAt(int x, int y) const {
        for (const auto& obs : obstacles) {
            if (obs.x == x && obs.y == y) return true;
        }
        return false;
    }
};

class SnakeGame {
private:
    RenderWindow window;
    Font font;
    
    // Background textures
    Texture menuBgTexture, transitionBgTexture, pauseBgTexture, leaderboardBgTexture, settingsBgTexture;
    Sprite menuBg, transitionBg, pauseBg, leaderboardBg, settingsBg;
    
    // Level system
    std::vector<Level> levels;
    int currentLevelIndex;
    Texture currentLevelBgTexture;
    Sprite currentLevelBg;
    Music currentBgMusic;
    
    // Game state
    GameState state;
    bool isFreePlay;
    
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

public:
    SnakeGame() : window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SnakeByte") {
        window.setFramerateLimit(60);
        srand(time(0));
        
        if (!font.loadFromFile("arial.ttf")) {
            std::cerr << "Font not loaded\n";
        }
        
        // Initialize levels
        initializeLevels();
        
        // Load background images
        menuBgTexture.loadFromFile("menu_bg.png");
        transitionBgTexture.loadFromFile("transition_bg.png");
        pauseBgTexture.loadFromFile("pause_bg.png");
        leaderboardBgTexture.loadFromFile("leaderboard_bg.png");
        settingsBgTexture.loadFromFile("settings_bg.png");
        
        menuBg.setTexture(menuBgTexture);
        transitionBg.setTexture(transitionBgTexture);
        pauseBg.setTexture(pauseBgTexture);
        leaderboardBg.setTexture(leaderboardBgTexture);
        settingsBg.setTexture(settingsBgTexture);
        
        loadSettings();
        
        // soundEnabled = true;
        state = MAIN_MENU;
        currentLevelIndex = 0;
        enteringName = false;
        leaderboardTab = 0;
        previousState = MAIN_MENU;
        isFreePlay = false;
        
        loadLeaderboard();
        initAudio();
        
        bonusFoodSpawnTimer.restart();
    }
    
    void initializeLevels() {

        std::string music1 = "assets/audios/music1.ogg";
        std::string bg1 = "assets/images/level1_bg.png";

        // Level 1: Simple box border, target score 50
        Level level1(1, 50, "Level 1", music1, bg1, 
                     Color(0, 255, 0), 4.0f, true);
        levels.push_back(level1);
        
        // Level 2: Add obstacles, target score 150
        Level level2(2, 150, "Level 2", music1, bg1,
                     Color(255, 255, 0), 4.0f, true);
        level2.setupDefaultObstacles();
        levels.push_back(level2);
        
        // Level 3: More complex obstacles, no target (play until death)
        Level level3(3, -1, "Level 3", music1, bg1,
                     Color(255, 100, 0), 5.0f, true);
        level3.setupDefaultObstacles();
        levels.push_back(level3);
    }
    
    void initAudio() {
        eatBuffer.loadFromFile("eat.wav");
        bonusBuffer.loadFromFile("bonus.wav");
        collisionBuffer.loadFromFile("collision.wav");
        gameOverBuffer.loadFromFile("gameover.wav");
        levelCompleteBuffer.loadFromFile("levelcomplete.wav");
        
        eatSound.setBuffer(eatBuffer);
        bonusSound.setBuffer(bonusBuffer);
        collisionSound.setBuffer(collisionBuffer);
        gameOverSound.setBuffer(gameOverBuffer);
        levelCompleteSound.setBuffer(levelCompleteBuffer);
    }
    
    void startGame(bool freePlay) {
        isFreePlay = freePlay;
        currentLevelIndex = 0;
        score = 0;
        resetLevel();
        state = PLAYING;
        loadLevelAssets();
        playBgMusic();
    }
    
    void loadLevelAssets() {
        if (isFreePlay) {
            // Use Level 1 assets for free play
            currentLevelBgTexture.loadFromFile(levels[0].bgImageFile);
            currentLevelBg.setTexture(currentLevelBgTexture);
            // currentBgMusic.openFromFile(levels[0].bgMusicFile);
        } else if (currentLevelIndex < levels.size()) {
            currentLevelBgTexture.loadFromFile(levels[currentLevelIndex].bgImageFile);
            currentLevelBg.setTexture(currentLevelBgTexture);
            // currentBgMusic.openFromFile(levels[currentLevelIndex].bgMusicFile);
        }
    }
    
    void playBgMusic() {
        currentBgMusic.stop();
        
        if (soundEnabled) {
            if (currentBgMusic.openFromFile(isFreePlay ? levels[0].bgMusicFile : levels[currentLevelIndex].bgMusicFile)) {
                currentBgMusic.setLoop(true);
                currentBgMusic.play();
                std::cout << "Playing music: " << (isFreePlay ? levels[0].bgMusicFile : levels[currentLevelIndex].bgMusicFile) << std::endl;
            } else {
                std::cerr << "Failed to load music file" << std::endl;
            }
        }
    }
    
    void resetLevel() {
        // Reset snake to starting position and size (center of screen, away from borders)
        snake.clear();
        int centerX = GRID_WIDTH / 2;
        int centerY = GRID_HEIGHT / 2;
        snake.push_back({centerX, centerY});
        snake.push_back({centerX - 1, centerY});
        snake.push_back({centerX - 2, centerY});
        
        direction = RIGHT;
        nextDirection = RIGHT;
        
        // Reset multipliers to default for new level
        scoreMultiplier = 1.0f;
        speedMultiplier = 1.0f;
        consecutiveActive = false;
        
        bonusFoodActive = false;
        bonusFoodSpawnTimer.restart();
        
        spawnFood(false);
        
        gameClock.restart();
        moveInterval = 0.15f;
    }
    
    Level& getCurrentLevel() {
        if (isFreePlay) {
            return levels[0]; // Free play uses level 1 layout but no obstacles
        }
        return levels[currentLevelIndex];
    }
    
    void spawnFood(bool bonus) {
        int x, y;
        bool valid;
        
        Food* targetFood = bonus ? &bonusFood : &food;
        
        do {
            valid = true;
            x = rand() % GRID_WIDTH;
            y = rand() % GRID_HEIGHT;
            
            // Check if inside border area
            if (!isFreePlay) {
                if (x == 0 || x == GRID_WIDTH - 1 || y == 0 || y == GRID_HEIGHT - 1) {
                    valid = false;
                    continue;
                }
            }
            
            // Check snake collision
            for (const auto& seg : snake) {
                if (seg.x == x && seg.y == y) {
                    valid = false;
                    break;
                }
            }
            
            // Check obstacle collision
            if (!isFreePlay && getCurrentLevel().isObstacleAt(x, y)) {
                valid = false;
            }
            
            // Check collision with other food
            if (bonus && food.x == x && food.y == y) {
                valid = false;
            }
            if (!bonus && bonusFoodActive && bonusFood.x == x && bonusFood.y == y) {
                valid = false;
            }
        } while (!valid);
        
        targetFood->x = x;
        targetFood->y = y;
        targetFood->isBonus = bonus;
        targetFood->spawnTimer.restart();
        
        if (bonus) {
            bonusFoodActive = true;
            bonusFoodDuration = 5.0f + (rand() % 3);
            bonusFoodTimer.restart();
        }
    }
    
    void handleInput() {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
            
            if (state == MAIN_MENU) {
                handleMenuInput(event);
            } else if (state == PLAYING) {
                handleGameInput(event);
            } else if (state == PAUSED) {
                if (event.type == Event::KeyPressed || event.type == Event::MouseButtonPressed) {
                    state = previousState;
                }
            } else if (state == GAME_OVER && enteringName) {
                handleNameInput(event);
            } else if (state == LEVEL_TRANSITION) {
                if (event.type == Event::KeyPressed && event.key.code != Keyboard::Escape) {
                    advanceToNextLevel();
                } else if (event.type == Event::MouseButtonPressed) {
                    advanceToNextLevel();
                }
            } else if (state == GAME_OVER || state == LEADERBOARD || state == SETTINGS) {
                if (event.type == Event::KeyPressed) {
                    if (event.key.code == Keyboard::Escape || event.key.code == Keyboard::BackSpace) {
                        state = MAIN_MENU;
                    }
                    if (state == LEADERBOARD) {
                        if (event.key.code == Keyboard::Num1 || event.key.code == Keyboard::Left) {
                            leaderboardTab = 0;
                        } else if (event.key.code == Keyboard::Num2 || event.key.code == Keyboard::Right) {
                            leaderboardTab = 1;
                        }
                    }
                }
                if (state == LEADERBOARD && event.type == Event::MouseButtonPressed) {
                    Vector2i mousePos = Mouse::getPosition(window);
                    if (mousePos.y >= 120 && mousePos.y <= 160) {
                        if (mousePos.x >= 200 && mousePos.x <= 350) {
                            leaderboardTab = 0;
                        } else if (mousePos.x >= 450 && mousePos.x <= 600) {
                            leaderboardTab = 1;
                        }
                    }
                }
            }
        }
    }
    
    void advanceToNextLevel() {
        currentLevelIndex = nextLevelIndex;
        resetLevel();
        loadLevelAssets();
        state = PLAYING;
        playBgMusic();
    }
    
    void handleMenuInput(Event& event) {
        if (event.type == Event::KeyPressed) {
            if (event.key.code == Keyboard::Num1 || event.key.code == Keyboard::P) {
                startGame(false);
            } else if (event.key.code == Keyboard::Num2 || event.key.code == Keyboard::F) {
                startGame(true);
            } else if (event.key.code == Keyboard::Num3 || event.key.code == Keyboard::L) {
                state = LEADERBOARD;
            } else if (event.key.code == Keyboard::Num4 || event.key.code == Keyboard::S) {
                state = SETTINGS;
            } else if (event.key.code == Keyboard::Num5 || event.key.code == Keyboard::Escape || event.key.code == Keyboard::Q) {
                window.close();
            }
        }
        
        if (event.type == Event::MouseButtonPressed) {
            Vector2i mousePos = Mouse::getPosition(window);
            
            if (mousePos.x >= 250 && mousePos.x <= 550) {
                if (mousePos.y >= 220 && mousePos.y <= 260) {
                    startGame(false);
                } else if (mousePos.y >= 280 && mousePos.y <= 320) {
                    startGame(true);
                } else if (mousePos.y >= 340 && mousePos.y <= 380) {
                    state = LEADERBOARD;
                } else if (mousePos.y >= 400 && mousePos.y <= 440) {
                    state = SETTINGS;
                } else if (mousePos.y >= 460 && mousePos.y <= 500) {
                    window.close();
                }
            }
        }
    }
    
    void handleGameInput(Event& event) {
        if (event.type == Event::KeyPressed) {
            if ((event.key.code == Keyboard::Up || event.key.code == Keyboard::W) && direction != DOWN) {
                nextDirection = UP;
            } else if ((event.key.code == Keyboard::Down || event.key.code == Keyboard::S) && direction != UP) {
                nextDirection = DOWN;
            } else if ((event.key.code == Keyboard::Left || event.key.code == Keyboard::A) && direction != RIGHT) {
                nextDirection = LEFT;
            } else if ((event.key.code == Keyboard::Right || event.key.code == Keyboard::D) && direction != LEFT) {
                nextDirection = RIGHT;
            } else if (event.key.code == Keyboard::Escape || event.key.code == Keyboard::P) {
                previousState = PLAYING;
                state = PAUSED;
            }
        }
    }
    
    void handleNameInput(Event& event) {
        if (event.type == Event::TextEntered) {
            if (event.text.unicode == 8) {
                if (!playerName.empty()) {
                    playerName.pop_back();
                }
            } else if (event.text.unicode == 13) {
                if (!playerName.empty()) {
                    addToLeaderboard(playerName, score, isFreePlay);
                    saveLeaderboard();
                    enteringName = false;
                    state = LEADERBOARD;
                }
            } else if (event.text.unicode < 128 && playerName.length() < 15) {
                playerName += static_cast<char>(event.text.unicode);
            }
        }
    }
    
    void update() {
        if (state == PAUSED || state == LEVEL_TRANSITION) {
            return;
        }
        
        if (state != PLAYING) return;

        if (soundEnabled && currentBgMusic.getStatus() != sf::Music::Playing) {
    std::cout << "Music stopped, restarting..." << std::endl;
    playBgMusic();
}
        
        float elapsed = gameClock.getElapsedTime().asSeconds();
        float currentInterval = moveInterval / speedMultiplier;
        
        if (elapsed >= currentInterval) {
            gameClock.restart();
            moveSnake();
        }
        
        if (!bonusFoodActive && bonusFoodSpawnTimer.getElapsedTime().asSeconds() >= 15.0f + (rand() % 10)) {
            spawnFood(true);
            bonusFoodSpawnTimer.restart();
        }
        
        if (bonusFoodActive && bonusFoodTimer.getElapsedTime().asSeconds() >= bonusFoodDuration) {
            bonusFoodActive = false;
        }
        
        if (consecutiveActive && consecutiveTimer.getElapsedTime().asSeconds() >= 2.0f) {
            scoreMultiplier = 1.0f;
            consecutiveActive = false;
        }
    }
    
    void moveSnake() {
        direction = nextDirection;
        
        SnakeSegment newHead = snake.front();
        
        switch (direction) {
            case UP: newHead.y--; break;
            case DOWN: newHead.y++; break;
            case LEFT: newHead.x--; break;
            case RIGHT: newHead.x++; break;
        }
        
        // Handle wrapping for free play
        if (isFreePlay) {
            if (newHead.x < 0) newHead.x = GRID_WIDTH - 1;
            if (newHead.x >= GRID_WIDTH) newHead.x = 0;
            if (newHead.y < 0) newHead.y = GRID_HEIGHT - 1;
            if (newHead.y >= GRID_HEIGHT) newHead.y = 0;
        } else {
            // Check border collision (treat borders as walls)
            if (newHead.x <= 0 || newHead.x >= GRID_WIDTH - 1 || 
                newHead.y <= 0 || newHead.y >= GRID_HEIGHT - 1) {
                gameOver();
                return;
            }
        }
        
        // Check self collision
        for (const auto& seg : snake) {
            if (seg.x == newHead.x && seg.y == newHead.y) {
                gameOver();
                return;
            }
        }
        
        // Check obstacle collision
        if (!isFreePlay && getCurrentLevel().isObstacleAt(newHead.x, newHead.y)) {
            gameOver();
            return;
        }
        
        bool ateFood = false;
        
        // Check normal food collision
        if (newHead.x == food.x && newHead.y == food.y) {
            int points = 2;
            score += static_cast<int>(points * scoreMultiplier);
            
            if (soundEnabled) {
                eatSound.play();
            }
            
            updateMultipliers();
            
            if (!isFreePlay) {
                speedMultiplier += 0.1f;
            }
            
            spawnFood(false);
            ateFood = true;
            
            checkLevelCompletion();
        }
        
        // Check bonus food collision
        if (bonusFoodActive && newHead.x == bonusFood.x && newHead.y == bonusFood.y) {
            int points = 10;
            score += static_cast<int>(points * scoreMultiplier);
            
            if (soundEnabled) {
                bonusSound.play();
            }
            
            updateMultipliers();
            
            if (!isFreePlay) {
                speedMultiplier += 0.1f;
            }
            
            bonusFoodActive = false;
            bonusFoodSpawnTimer.restart();
            ateFood = true;
            
            checkLevelCompletion();
        }
        
        snake.push_front(newHead);
        
        if (!ateFood) {
            snake.pop_back();
        }
    }
    
    void updateMultipliers() {
        if (consecutiveActive) {
            scoreMultiplier += 0.2f;
        } else {
            consecutiveActive = true;
            scoreMultiplier = 1.0f;
        }
        consecutiveTimer.restart();
    }
    
    void checkLevelCompletion() {
        if (isFreePlay) return;
        
        Level& level = getCurrentLevel();
        if (level.scoreTarget > 0 && score >= level.scoreTarget) {
            levelComplete();
        }
    }
    
    void levelComplete() {
        if (soundEnabled) {
            levelCompleteSound.play();
        }
        
        if (currentLevelIndex < levels.size() - 1) {
            nextLevelIndex = currentLevelIndex + 1;
            state = LEVEL_TRANSITION;
            transitionClock.restart();
            currentBgMusic.stop();
        } else {
            gameOver();
        }
    }
    
    void gameOver() {
        if (soundEnabled) {
            gameOverSound.play();
        }
        
        currentBgMusic.stop();
        
        auto& targetLeaderboard = isFreePlay ? freePlayLeaderboard : leaderboard;
        
        if (score > 0 && (targetLeaderboard.size() < 5 || score > targetLeaderboard.back().second)) {
            enteringName = true;
            playerName = "";
        }
        
        state = GAME_OVER;
    }
    
    void render() {
        window.clear(Color(20, 20, 20));
        
        if (state == MAIN_MENU) {
            renderMainMenu();
        } else if (state == PLAYING || state == PAUSED) {
            renderGame();
            if (state == PAUSED) {
                renderPauseOverlay();
            }
        } else if (state == LEVEL_TRANSITION) {
            renderLevelTransition();
        } else if (state == GAME_OVER) {
            renderGameOver();
        } else if (state == LEADERBOARD) {
            renderLeaderboard();
        } else if (state == SETTINGS) {
            renderSettings();
        }
        
        window.display();
    }
    
    void renderMainMenu() {
        if (menuBgTexture.getSize().x > 0) {
            window.draw(menuBg);
        }
        
        Text title("SNAKEBYTE", font, 60);
        title.setFillColor(Color::Green);
        title.setPosition(WINDOW_WIDTH / 2 - 150, 50);
        window.draw(title);
        
        Text subtitle("Classic Snake Game", font, 20);
        subtitle.setFillColor(Color(150, 150, 150));
        subtitle.setPosition(WINDOW_WIDTH / 2 - 100, 130);
        window.draw(subtitle);
        
        std::vector<std::string> options = {
            "1. Play Levels (Press 1 or P)",
            "2. Free Play (Press 2 or F)",
            "3. Leaderboard (Press 3 or L)",
            "4. Settings (Press 4 or S)",
            "5. Exit (Press 5 or Q)"
        };
        
        Vector2i mousePos = Mouse::getPosition(window);
        
        for (size_t i = 0; i < options.size(); i++) {
            Text option(options[i], font, 24);
            int yPos = 220 + i * 60;
            
            if (mousePos.x >= 250 && mousePos.x <= 550 && 
                mousePos.y >= yPos && mousePos.y <= yPos + 40) {
                option.setFillColor(Color::Yellow);
            } else {
                option.setFillColor(Color::White);
            }
            
            option.setPosition(WINDOW_WIDTH / 2 - 180, yPos);
            window.draw(option);
        }
        
        Text hint("Use Arrow Keys or WASD to play", font, 18);
        hint.setFillColor(Color(100, 100, 100));
        hint.setPosition(WINDOW_WIDTH / 2 - 130, 540);
        window.draw(hint);
    }
    
    void renderGame() {
        if (currentLevelBgTexture.getSize().x > 0) {
            window.draw(currentLevelBg);
        }
        
        Color wallColor(96, 26, 48); // #601a30
        Color wallBorderColor(150, 50, 80); // Lighter shade for brick effect
        
        // Draw borders as grid squares with brick effect
        if (!isFreePlay) {
            // Top border
            for (int i = 0; i < GRID_WIDTH; i++) {
                RectangleShape borderSquare(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
                borderSquare.setPosition(i * GRID_SIZE + 1, 1);
                borderSquare.setFillColor(wallColor);
                borderSquare.setOutlineColor(wallBorderColor);
                borderSquare.setOutlineThickness(1);
                window.draw(borderSquare);
            }
            
            // Bottom border
            for (int i = 0; i < GRID_WIDTH; i++) {
                RectangleShape borderSquare(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
                borderSquare.setPosition(i * GRID_SIZE + 1, (GRID_HEIGHT - 1) * GRID_SIZE + 1);
                borderSquare.setFillColor(wallColor);
                borderSquare.setOutlineColor(wallBorderColor);
                borderSquare.setOutlineThickness(1);
                window.draw(borderSquare);
            }
            
            // Left border
            for (int i = 1; i < GRID_HEIGHT - 1; i++) {
                RectangleShape borderSquare(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
                borderSquare.setPosition(1, i * GRID_SIZE + 1);
                borderSquare.setFillColor(wallColor);
                borderSquare.setOutlineColor(wallBorderColor);
                borderSquare.setOutlineThickness(1);
                window.draw(borderSquare);
            }
            
            // Right border
            for (int i = 1; i < GRID_HEIGHT - 1; i++) {
                RectangleShape borderSquare(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
                borderSquare.setPosition((GRID_WIDTH - 1) * GRID_SIZE + 1, i * GRID_SIZE + 1);
                borderSquare.setFillColor(wallColor);
                borderSquare.setOutlineColor(wallBorderColor);
                borderSquare.setOutlineThickness(1);
                window.draw(borderSquare);
            }
            
            // Draw obstacles as grid squares with brick effect
            for (const auto& obs : getCurrentLevel().obstacles) {
                RectangleShape rect(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
                rect.setPosition(obs.x * GRID_SIZE + 1, obs.y * GRID_SIZE + 1);
                rect.setFillColor(wallColor);
                rect.setOutlineColor(wallBorderColor);
                rect.setOutlineThickness(1);
                window.draw(rect);
            }
        }
        
        // Draw normal food
        CircleShape foodCircle(GRID_SIZE / 2 - 2);
        foodCircle.setPosition(food.x * GRID_SIZE + 2, food.y * GRID_SIZE + 2);
        foodCircle.setFillColor(Color::Red);
        window.draw(foodCircle);
        
        // Draw bonus food with animation
        if (bonusFoodActive) {
            float pulseTime = bonusFoodTimer.getElapsedTime().asSeconds();
            float scale = 1.0f + 0.2f * sin(pulseTime * 6.0f);
            
            CircleShape bonusFoodCircle(GRID_SIZE / 2 + 2);
            bonusFoodCircle.setScale(scale, scale);
            bonusFoodCircle.setFillColor(Color::Yellow);
            bonusFoodCircle.setOrigin(GRID_SIZE / 2 + 2, GRID_SIZE / 2 + 2);
            bonusFoodCircle.setPosition(bonusFood.x * GRID_SIZE + GRID_SIZE / 2, 
                                        bonusFood.y * GRID_SIZE + GRID_SIZE / 2);
            window.draw(bonusFoodCircle);
        }
        
        // Draw snake with visible grid squares
        for (size_t i = 0; i < snake.size(); i++) {
            RectangleShape rect(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            rect.setPosition(snake[i].x * GRID_SIZE + 1, snake[i].y * GRID_SIZE + 1);
            rect.setFillColor(i == 0 ? Color::Green : Color(0, 200, 0));
            rect.setOutlineColor(Color(0, 150, 0));
            rect.setOutlineThickness(1);
            window.draw(rect);
        }
        
        // Draw UI (outside borders)
        Text scoreText("Score: " + std::to_string(score), font, 20);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(10, 10);
        window.draw(scoreText);
        
        Text multText("x" + std::to_string(scoreMultiplier).substr(0, 4), font, 20);
        multText.setFillColor(Color::Yellow);
        multText.setPosition(10, 35);
        window.draw(multText);
        
        if (!isFreePlay) {
            Text levelText("Level: " + std::to_string(getCurrentLevel().levelNumber), font, 20);
            levelText.setFillColor(Color::White);
            levelText.setPosition(WINDOW_WIDTH - 120, 10);
            window.draw(levelText);
        } else {
            Text modeText("Free Play", font, 20);
            modeText.setFillColor(Color::White);
            modeText.setPosition(WINDOW_WIDTH - 120, 10);
            window.draw(modeText);
        }
    }
    
    void renderLevelTransition() {
        if (transitionBgTexture.getSize().x > 0) {
            window.draw(transitionBg);
        }
        
        Text title("LEVEL " + std::to_string(levels[currentLevelIndex].levelNumber) + " COMPLETE!", font, 50);
        title.setFillColor(Color::Green);
        title.setPosition(WINDOW_WIDTH / 2 - 250, 150);
        window.draw(title);
        
        Text scoreText("Score: " + std::to_string(score), font, 40);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(WINDOW_WIDTH / 2 - 100, 250);
        window.draw(scoreText);
        
        Text nextText("Advancing to Level " + std::to_string(levels[nextLevelIndex].levelNumber), font, 35);
        nextText.setFillColor(Color::Yellow);
        nextText.setPosition(WINDOW_WIDTH / 2 - 180, 350);
        window.draw(nextText);
        
        Text resetInfo("Snake reset to default size and speed", font, 20);
        resetInfo.setFillColor(Color(150, 200, 150));
        resetInfo.setPosition(WINDOW_WIDTH / 2 - 180, 400);
        window.draw(resetInfo);
        
        Text hint("Press any key to continue...", font, 22);
        hint.setFillColor(Color::White);
        hint.setPosition(WINDOW_WIDTH / 2 - 130, 480);
        window.draw(hint);
    }
    
    void renderPauseOverlay() {
        RectangleShape overlay(Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        overlay.setFillColor(Color(0, 0, 0, 180));
        window.draw(overlay);
        
        if (pauseBgTexture.getSize().x > 0) {
            pauseBg.setColor(Color(255, 255, 255, 200));
            window.draw(pauseBg);
        }
        
        Text title("GAME PAUSED", font, 60);
        title.setFillColor(Color::Yellow);
        title.setPosition(WINDOW_WIDTH / 2 - 180, 200);
        window.draw(title);
        
        Text hint("Press any key to continue", font, 25);
        hint.setFillColor(Color::White);
        hint.setPosition(WINDOW_WIDTH / 2 - 150, 320);
        window.draw(hint);
        
        Text controls("Controls: Arrow Keys or WASD", font, 20);
        controls.setFillColor(Color(200, 200, 200));
        controls.setPosition(WINDOW_WIDTH / 2 - 140, 400);
        window.draw(controls);
        
        Text pauseKey("Pause: ESC or P", font, 20);
        pauseKey.setFillColor(Color(200, 200, 200));
        pauseKey.setPosition(WINDOW_WIDTH / 2 - 80, 430);
        window.draw(pauseKey);
    }
    
    void renderGameOver() {
        Text title("GAME OVER", font, 50);
        title.setFillColor(Color::Red);
        title.setPosition(WINDOW_WIDTH / 2 - 150, 150);
        window.draw(title);
        
        Text scoreText("Final Score: " + std::to_string(score), font, 30);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(WINDOW_WIDTH / 2 - 120, 230);
        window.draw(scoreText);
        
        if (enteringName) {
            Text prompt("Enter your name:", font, 25);
            prompt.setFillColor(Color::Yellow);
            prompt.setPosition(WINDOW_WIDTH / 2 - 120, 300);
            window.draw(prompt);
            
            Text nameText(playerName + "_", font, 30);
            nameText.setFillColor(Color::White);
            nameText.setPosition(WINDOW_WIDTH / 2 - 100, 340);
            window.draw(nameText);
        } else {
            Text back("Press ESC to return to menu", font, 20);
            back.setFillColor(Color(150, 150, 150));
            back.setPosition(WINDOW_WIDTH / 2 - 150, 400);
            window.draw(back);
        }
    }
    
    void renderLeaderboard() {
        if (leaderboardBgTexture.getSize().x > 0) {
            window.draw(leaderboardBg);
        }
        
        Text title("LEADERBOARD", font, 50);
        title.setFillColor(Color::Yellow);
        title.setPosition(WINDOW_WIDTH / 2 - 180, 50);
        window.draw(title);
        
        Vector2i mousePos = Mouse::getPosition(window);
        
        RectangleShape normalTab(Vector2f(150, 40));
        normalTab.setPosition(200, 120);
        normalTab.setFillColor(leaderboardTab == 0 ? Color(0, 150, 0) : Color(50, 50, 50));
        if (mousePos.x >= 200 && mousePos.x <= 350 && mousePos.y >= 120 && mousePos.y <= 160) {
            normalTab.setOutlineColor(Color::White);
            normalTab.setOutlineThickness(2);
        }
        window.draw(normalTab);
        
        Text normalText("Normal Play", font, 20);
        normalText.setFillColor(Color::White);
        normalText.setPosition(220, 130);
        window.draw(normalText);
        
        RectangleShape freeTab(Vector2f(150, 40));
        freeTab.setPosition(450, 120);
        freeTab.setFillColor(leaderboardTab == 1 ? Color(0, 150, 0) : Color(50, 50, 50));
        if (mousePos.x >= 450 && mousePos.x <= 600 && mousePos.y >= 120 && mousePos.y <= 160) {
            freeTab.setOutlineColor(Color::White);
            freeTab.setOutlineThickness(2);
        }
        window.draw(freeTab);
        
        Text freeText("Free Play", font, 20);
        freeText.setFillColor(Color::White);
        freeText.setPosition(475, 130);
        window.draw(freeText);
        
        auto& activeLeaderboard = (leaderboardTab == 0) ? leaderboard : freePlayLeaderboard;
        
        for (size_t i = 0; i < activeLeaderboard.size() && i < 5; i++) {
            std::string entry = std::to_string(i + 1) + ". " + 
                               activeLeaderboard[i].first + " - " + 
                               std::to_string(activeLeaderboard[i].second);
            Text text(entry, font, 30);
            text.setFillColor(Color::White);
            text.setPosition(WINDOW_WIDTH / 2 - 150, 200 + i * 60);
            window.draw(text);
        }
        
        Text back("Press ESC to return", font, 20);
        back.setFillColor(Color(150, 150, 150));
        back.setPosition(WINDOW_WIDTH / 2 - 120, 520);
        window.draw(back);
    }
    
    void renderSettings() {
        if (settingsBgTexture.getSize().x > 0) {
            window.draw(settingsBg);
        }
        
        Text title("SETTINGS", font, 50);
        title.setFillColor(Color::Cyan);
        title.setPosition(WINDOW_WIDTH / 2 - 120, 100);
        window.draw(title);
        
        Text soundText("Sound: " + std::string(soundEnabled ? "ON" : "OFF"), font, 30);
        soundText.setFillColor(Color::White);
        soundText.setPosition(WINDOW_WIDTH / 2 - 100, 250);
        window.draw(soundText);
        
        Text toggle("Press S to toggle", font, 20);
        toggle.setFillColor(Color(150, 150, 150));
        toggle.setPosition(WINDOW_WIDTH / 2 - 100, 300);
        window.draw(toggle);
        
        Text back("Press ESC to return", font, 20);
        back.setFillColor(Color(150, 150, 150));
        back.setPosition(WINDOW_WIDTH / 2 - 120, 450);
        window.draw(back);
        
        if (Keyboard::isKeyPressed(Keyboard::S)) {
            soundEnabled = !soundEnabled;
            if (!soundEnabled) {
                currentBgMusic.stop();
            }
            saveSettings();
            sleep(milliseconds(200));
        }
    }
    
    void loadSettings() {
        std::ifstream file("settings.json");
        if (file.is_open()) {
            try {
                json j;
                file >> j;
                file.close();
                
                if (j.contains("sound")) {
                    soundEnabled = j["sound"].get<bool>();
                    std::cout << "Settings loaded: sound = " << (soundEnabled ? "ON" : "OFF") << std::endl;
                } else {
                    soundEnabled = true;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error loading settings: " << e.what() << std::endl;
                soundEnabled = true;
                saveSettings();
            }
        } else {
            // No settings file, create with defaults
            std::cout << "No settings file found, creating with defaults" << std::endl;
            soundEnabled = true;
            saveSettings();
        }
    }
    
    void saveSettings() {
        json j;
        j["sound"] = soundEnabled;
        
        std::ofstream file("settings.json");
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
            std::cout << "Settings saved: sound = " << (soundEnabled ? "ON" : "OFF") << std::endl;
        } else {
            std::cerr << "Failed to save settings" << std::endl;
        }
    }
    
    void loadLeaderboard() {
        std::ifstream file("leaderboard.json");
        if (file.is_open()) {
            try {
                json j;
                file >> j;
                leaderboard.clear();
                freePlayLeaderboard.clear();
                
                if (j.contains("normal")) {
                    for (auto& entry : j["normal"]) {
                        leaderboard.push_back({entry["name"], entry["score"]});
                    }
                }
                
                if (j.contains("freeplay")) {
                    for (auto& entry : j["freeplay"]) {
                        freePlayLeaderboard.push_back({entry["name"], entry["score"]});
                    }
                }
            } catch (...) {
                leaderboard.clear();
                freePlayLeaderboard.clear();
            }
            file.close();
        }
    }
    
    void saveLeaderboard() {
        json j;
        j["normal"] = json::array();
        j["freeplay"] = json::array();
        
        for (const auto& entry : leaderboard) {
            j["normal"].push_back({{"name", entry.first}, {"score", entry.second}});
        }
        
        for (const auto& entry : freePlayLeaderboard) {
            j["freeplay"].push_back({{"name", entry.first}, {"score", entry.second}});
        }
        
        std::ofstream file("leaderboard.json");
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
        }
    }
    
    void addToLeaderboard(const std::string& name, int newScore, bool isFreePlay) {
        auto& targetLeaderboard = isFreePlay ? freePlayLeaderboard : leaderboard;
        
        targetLeaderboard.push_back({name, newScore});
        std::sort(targetLeaderboard.begin(), targetLeaderboard.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        if (targetLeaderboard.size() > 5) {
            targetLeaderboard.resize(5);
        }
    }
    
    void run() {
        while (window.isOpen()) {
            handleInput();
            update();
            render();
        }
    }
};

int main() {
    SnakeGame game;
    game.run();
    return 0;
}