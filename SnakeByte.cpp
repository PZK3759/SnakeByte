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
enum GameState { MAIN_MENU, PLAYING, GAME_OVER, LEADERBOARD, SETTINGS, LEVEL_TRANSITION };
enum GameMode { LEVEL_1, LEVEL_2, LEVEL_3, FREE_PLAY };

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

class SnakeGame {
private:
    RenderWindow window;
    Font font;
    
    // Background textures
    Texture menuBgTexture, level1BgTexture, level2BgTexture, level3BgTexture, transitionBgTexture;
    Sprite menuBg, level1Bg, level2Bg, level3Bg, transitionBg;
    
    // Transition
    Clock transitionClock;
    int nextLevel;
    
    // Game state
    GameState state;
    GameMode mode;
    int currentLevel;
    
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
    float bonusFoodDuration;
    Clock bonusFoodSpawnTimer;
    
    // Scoring
    int score;
    float scoreMultiplier;
    float speedMultiplier;
    Clock consecutiveTimer;
    bool consecutiveActive;
    
    // Game timing
    Clock gameClock;
    float moveInterval;
    
    // Obstacles
    std::vector<Obstacle> obstacles;
    
    // Audio
    Music bgMusic1, bgMusic2, bgMusic3;
    SoundBuffer eatBuffer, bonusBuffer, collisionBuffer, gameOverBuffer, levelCompleteBuffer;
    Sound eatSound, bonusSound, collisionSound, gameOverSound, levelCompleteSound;
    bool soundEnabled;
    
    // Leaderboard
    std::vector<std::pair<std::string, int>> leaderboard;
    std::vector<std::pair<std::string, int>> freePlayLeaderboard;
    std::string playerName;
    bool enteringName;
    int leaderboardTab; // 0 = normal, 1 = free play
    
    // Level targets
    int levelTarget;

public:
    SnakeGame() : window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SnakeByte") {
        window.setFramerateLimit(60);
        srand(time(0));
        
        if (!font.loadFromFile("arial.ttf")) {
            // Fallback - create basic game without custom font
            std::cerr << "Font not loaded\n";
        }
        
        // Load background images (optional - will work without them)
        menuBgTexture.loadFromFile("menu_bg.png");
        level1BgTexture.loadFromFile("level1_bg.png");
        level2BgTexture.loadFromFile("level2_bg.png");
        level3BgTexture.loadFromFile("level3_bg.png");
        transitionBgTexture.loadFromFile("transition_bg.png");
        
        menuBg.setTexture(menuBgTexture);
        level1Bg.setTexture(level1BgTexture);
        level2Bg.setTexture(level2BgTexture);
        level3Bg.setTexture(level3BgTexture);
        transitionBg.setTexture(transitionBgTexture);
        
        soundEnabled = true;
        state = MAIN_MENU;
        currentLevel = 1;
        enteringName = false;
        leaderboardTab = 0;
        
        loadLeaderboard();
        initAudio();
        
        bonusFoodSpawnTimer.restart();
    }
    
    void initAudio() {
        // Note: You'll need to provide actual audio files
        // For now, we'll handle missing files gracefully
        bgMusic1.openFromFile("music1.ogg");
        bgMusic2.openFromFile("music2.ogg");
        bgMusic3.openFromFile("music3.ogg");
        
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
    
    void startGame(GameMode gMode) {
        mode = gMode;
        if (mode != FREE_PLAY) {
            currentLevel = 1;
        }
        score = 0; // Reset score at game start
        resetLevel();
        state = PLAYING;
        playBgMusic();
    }
    
    void resetLevel() {
        snake.clear();
        snake.push_back({GRID_WIDTH / 2, GRID_HEIGHT / 2});
        snake.push_back({GRID_WIDTH / 2 - 1, GRID_HEIGHT / 2});
        snake.push_back({GRID_WIDTH / 2 - 2, GRID_HEIGHT / 2});
        
        direction = RIGHT;
        nextDirection = RIGHT;
        
        // Don't reset score between levels
        scoreMultiplier = 1.0f;
        speedMultiplier = 1.0f;
        consecutiveActive = false;
        
        bonusFoodActive = false;
        bonusFoodSpawnTimer.restart();
        
        obstacles.clear();
        setupLevel();
        spawnFood(false);
        
        gameClock.restart();
        moveInterval = 0.15f;
        
        if (mode == FREE_PLAY) {
            levelTarget = -1;
        } else if (currentLevel == 1) {
            levelTarget = 50;
        } else if (currentLevel == 2) {
            levelTarget = 150;
        } else {
            levelTarget = -1; // Level 3 has no target
        }
    }
    
    void setupLevel() {
        obstacles.clear();
        
        if (mode == LEVEL_2 || (mode != FREE_PLAY && currentLevel == 2)) {
            // Simple obstacles
            for (int i = 10; i < 30; i++) {
                obstacles.push_back({i, 15});
            }
        } else if (mode == LEVEL_3 || (mode != FREE_PLAY && currentLevel == 3)) {
            // More complex obstacles
            for (int i = 10; i < 30; i++) {
                obstacles.push_back({i, 12});
                obstacles.push_back({i, 18});
            }
            for (int i = 12; i < 18; i++) {
                obstacles.push_back({10, i});
                obstacles.push_back({30, i});
            }
        }
    }
    
    void playBgMusic() {
        bgMusic1.stop();
        bgMusic2.stop();
        bgMusic3.stop();
        
        if (!soundEnabled) return;
        
        if (mode == FREE_PLAY) {
            bgMusic1.play();
            bgMusic1.setLoop(true);
        } else if (currentLevel == 1) {
            bgMusic1.play();
            bgMusic1.setLoop(true);
        } else if (currentLevel == 2) {
            bgMusic2.play();
            bgMusic2.setLoop(true);
        } else if (currentLevel == 3) {
            bgMusic3.play();
            bgMusic3.setLoop(true);
        }
    }
    
    void spawnFood(bool bonus) {
        int x, y;
        bool valid;
        
        Food* targetFood = bonus ? &bonusFood : &food;
        
        do {
            valid = true;
            x = rand() % GRID_WIDTH;
            y = rand() % GRID_HEIGHT;
            
            // Check snake collision
            for (const auto& seg : snake) {
                if (seg.x == x && seg.y == y) {
                    valid = false;
                    break;
                }
            }
            
            // Check obstacle collision
            for (const auto& obs : obstacles) {
                if (obs.x == x && obs.y == y) {
                    valid = false;
                    break;
                }
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
            } else if (state == GAME_OVER && enteringName) {
                handleNameInput(event);
            } else if (state == LEVEL_TRANSITION) {
                // Skip transition on key press
                if (event.type == Event::KeyPressed || event.type == Event::MouseButtonPressed) {
                    currentLevel = nextLevel;
                    resetLevel();
                    state = PLAYING;
                    playBgMusic();
                }
            } else if (state == GAME_OVER || state == LEADERBOARD || state == SETTINGS) {
                if (event.type == Event::KeyPressed) {
                    if (event.key.code == Keyboard::Escape || event.key.code == Keyboard::BackSpace) {
                        state = MAIN_MENU;
                    }
                    // Tab switching in leaderboard
                    if (state == LEADERBOARD) {
                        if (event.key.code == Keyboard::Num1 || event.key.code == Keyboard::Left) {
                            leaderboardTab = 0;
                        } else if (event.key.code == Keyboard::Num2 || event.key.code == Keyboard::Right) {
                            leaderboardTab = 1;
                        }
                    }
                }
                // Mouse clicks for leaderboard tabs
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
    
    void handleMenuInput(Event& event) {
        if (event.type == Event::KeyPressed) {
            if (event.key.code == Keyboard::Num1 || event.key.code == Keyboard::P) {
                startGame(LEVEL_1);
            } else if (event.key.code == Keyboard::Num2 || event.key.code == Keyboard::F) {
                startGame(FREE_PLAY);
            } else if (event.key.code == Keyboard::Num3 || event.key.code == Keyboard::L) {
                state = LEADERBOARD;
            } else if (event.key.code == Keyboard::Num4 || event.key.code == Keyboard::S) {
                state = SETTINGS;
            } else if (event.key.code == Keyboard::Num5 || event.key.code == Keyboard::Escape || event.key.code == Keyboard::Q) {
                window.close();
            }
        }
        
        // Mouse support
        if (event.type == Event::MouseButtonPressed) {
            Vector2i mousePos = Mouse::getPosition(window);
            
            // Check menu button clicks (y positions: 220, 280, 340, 400, 460)
            if (mousePos.x >= 250 && mousePos.x <= 550) {
                if (mousePos.y >= 220 && mousePos.y <= 260) {
                    startGame(LEVEL_1);
                } else if (mousePos.y >= 280 && mousePos.y <= 320) {
                    startGame(FREE_PLAY);
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
            } else if (event.key.code == Keyboard::Escape) {
                state = MAIN_MENU;
                bgMusic1.stop();
                bgMusic2.stop();
                bgMusic3.stop();
            }
        }
    }
    
    void handleNameInput(Event& event) {
        if (event.type == Event::TextEntered) {
            if (event.text.unicode == 8) { // Backspace
                if (!playerName.empty()) {
                    playerName.pop_back();
                }
            } else if (event.text.unicode == 13) { // Enter
                if (!playerName.empty()) {
                    if (mode == FREE_PLAY) {
                        addToLeaderboard(playerName, score, true);
                    } else {
                        addToLeaderboard(playerName, score, false);
                    }
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
        if (state == LEVEL_TRANSITION) {
            // Auto advance after 3 seconds
            if (transitionClock.getElapsedTime().asSeconds() >= 3.0f) {
                currentLevel = nextLevel;
                resetLevel();
                state = PLAYING;
                playBgMusic();
            }
            return;
        }
        
        if (state != PLAYING) return;
        
        float elapsed = gameClock.getElapsedTime().asSeconds();
        float currentInterval = moveInterval / speedMultiplier;
        
        if (elapsed >= currentInterval) {
            gameClock.restart();
            moveSnake();
        }
        
        // Check for bonus food spawn (every 15-25 seconds)
        if (!bonusFoodActive && bonusFoodSpawnTimer.getElapsedTime().asSeconds() >= 15.0f + (rand() % 10)) {
            spawnFood(true);
            bonusFoodSpawnTimer.restart();
        }
        
        // Check bonus food timeout
        if (bonusFoodActive && bonusFoodTimer.getElapsedTime().asSeconds() >= bonusFoodDuration) {
            bonusFoodActive = false;
        }
        
        // Check consecutive timer
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
        if (mode == FREE_PLAY) {
            if (newHead.x < 0) newHead.x = GRID_WIDTH - 1;
            if (newHead.x >= GRID_WIDTH) newHead.x = 0;
            if (newHead.y < 0) newHead.y = GRID_HEIGHT - 1;
            if (newHead.y >= GRID_HEIGHT) newHead.y = 0;
        }
        
        // Check wall collision (non-free play)
        if (mode != FREE_PLAY) {
            if (newHead.x < 0 || newHead.x >= GRID_WIDTH || 
                newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
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
        for (const auto& obs : obstacles) {
            if (obs.x == newHead.x && obs.y == newHead.y) {
                gameOver();
                return;
            }
        }
        
        bool ateFood = false;
        
        // Check normal food collision
        if (newHead.x == food.x && newHead.y == food.y) {
            int points = 2;
            score += static_cast<int>(points * scoreMultiplier);
            
            if (soundEnabled) {
                eatSound.play();
            }
            
            // Update multipliers
            if (consecutiveActive) {
                scoreMultiplier += 0.2f;
            } else {
                consecutiveActive = true;
                scoreMultiplier = 1.0f;
            }
            consecutiveTimer.restart();
            
            if (mode != FREE_PLAY) {
                speedMultiplier += 0.1f;
            }
            
            spawnFood(false);
            ateFood = true;
            
            // Check level completion
            if (mode != FREE_PLAY && levelTarget > 0 && score >= levelTarget) {
                levelComplete();
                return;
            }
        }
        
        // Check bonus food collision
        if (bonusFoodActive && newHead.x == bonusFood.x && newHead.y == bonusFood.y) {
            int points = 10;
            score += static_cast<int>(points * scoreMultiplier);
            
            if (soundEnabled) {
                bonusSound.play();
            }
            
            // Update multipliers
            if (consecutiveActive) {
                scoreMultiplier += 0.2f;
            } else {
                consecutiveActive = true;
                scoreMultiplier = 1.0f;
            }
            consecutiveTimer.restart();
            
            if (mode != FREE_PLAY) {
                speedMultiplier += 0.1f;
            }
            
            bonusFoodActive = false;
            bonusFoodSpawnTimer.restart();
            ateFood = true;
            
            // Check level completion
            if (mode != FREE_PLAY && levelTarget > 0 && score >= levelTarget) {
                levelComplete();
                return;
            }
        }
        
        snake.push_front(newHead);
        
        if (!ateFood) {
            snake.pop_back();
        }
    }
    
    void levelComplete() {
        if (soundEnabled) {
            levelCompleteSound.play();
        }
        
        if (currentLevel < 3) {
            nextLevel = currentLevel + 1;
            state = LEVEL_TRANSITION;
            transitionClock.restart();
        } else {
            // Game won!
            gameOver();
        }
    }
    
    void gameOver() {
        if (soundEnabled) {
            gameOverSound.play();
        }
        
        bgMusic1.stop();
        bgMusic2.stop();
        bgMusic3.stop();
        
        // Choose correct leaderboard based on mode
        auto& targetLeaderboard = (mode == FREE_PLAY) ? freePlayLeaderboard : leaderboard;
        
        // Check if score qualifies for leaderboard (must be > 0)
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
        } else if (state == PLAYING) {
            renderGame();
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
        // Draw background if available
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
            
            // Highlight on hover
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
        // Draw background based on level
        if (mode == FREE_PLAY) {
            if (level1BgTexture.getSize().x > 0) {
                window.draw(level1Bg);
            }
        } else if (currentLevel == 1) {
            if (level1BgTexture.getSize().x > 0) {
                window.draw(level1Bg);
            }
        } else if (currentLevel == 2) {
            if (level2BgTexture.getSize().x > 0) {
                window.draw(level2Bg);
            }
        } else if (currentLevel == 3) {
            if (level3BgTexture.getSize().x > 0) {
                window.draw(level3Bg);
            }
        }
        
        // Draw borders for non-free play with thicker, more visible border
        if (mode != FREE_PLAY) {
            RectangleShape border(Vector2f(WINDOW_WIDTH - 8, WINDOW_HEIGHT - 8));
            border.setPosition(4, 4);
            border.setFillColor(Color::Transparent);
            border.setOutlineColor(Color(0, 255, 0)); // Bright green border
            border.setOutlineThickness(4);
            window.draw(border);
        }
        
        // Draw obstacles
        for (const auto& obs : obstacles) {
            RectangleShape rect(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            rect.setPosition(obs.x * GRID_SIZE + 1, obs.y * GRID_SIZE + 1);
            rect.setFillColor(Color(150, 50, 50));
            window.draw(rect);
        }
        
        // Draw normal food (circular)
        CircleShape foodCircle(GRID_SIZE / 2 - 2);
        foodCircle.setPosition(food.x * GRID_SIZE + 2, food.y * GRID_SIZE + 2);
        foodCircle.setFillColor(Color::Red);
        window.draw(foodCircle);
        
        // Draw bonus food if active (circular with pulsing animation)
        if (bonusFoodActive) {
            float pulseTime = bonusFoodTimer.getElapsedTime().asSeconds();
            float scale = 1.0f + 0.2f * sin(pulseTime * 6.0f); // Pulsing effect
            
            CircleShape bonusFoodCircle(GRID_SIZE / 2 + 2);
            bonusFoodCircle.setPosition(bonusFood.x * GRID_SIZE - 2, bonusFood.y * GRID_SIZE - 2);
            bonusFoodCircle.setScale(scale, scale);
            bonusFoodCircle.setFillColor(Color::Yellow);
            bonusFoodCircle.setOrigin(GRID_SIZE / 2 + 2, GRID_SIZE / 2 + 2);
            bonusFoodCircle.setPosition(bonusFood.x * GRID_SIZE + GRID_SIZE / 2, 
                                        bonusFood.y * GRID_SIZE + GRID_SIZE / 2);
            window.draw(bonusFoodCircle);
        }
        
        // Draw snake
        for (size_t i = 0; i < snake.size(); i++) {
            RectangleShape rect(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            rect.setPosition(snake[i].x * GRID_SIZE + 1, snake[i].y * GRID_SIZE + 1);
            rect.setFillColor(i == 0 ? Color::Green : Color(0, 200, 0));
            window.draw(rect);
        }
        
        // Draw score
        Text scoreText("Score: " + std::to_string(score), font, 20);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(10, 10);
        window.draw(scoreText);
        
        // Draw multiplier
        Text multText("x" + std::to_string(scoreMultiplier).substr(0, 4), font, 20);
        multText.setFillColor(Color::Yellow);
        multText.setPosition(10, 35);
        window.draw(multText);
        
        // Draw level
        if (mode != FREE_PLAY) {
            Text levelText("Level: " + std::to_string(currentLevel), font, 20);
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
        // Draw background if available
        if (transitionBgTexture.getSize().x > 0) {
            window.draw(transitionBg);
        }
        
        Text title("LEVEL " + std::to_string(currentLevel) + " COMPLETE!", font, 50);
        title.setFillColor(Color::Green);
        title.setPosition(WINDOW_WIDTH / 2 - 250, 150);
        window.draw(title);
        
        Text scoreText("Score: " + std::to_string(score), font, 40);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(WINDOW_WIDTH / 2 - 100, 250);
        window.draw(scoreText);
        
        Text nextText("Advancing to Level " + std::to_string(nextLevel), font, 35);
        nextText.setFillColor(Color::Yellow);
        nextText.setPosition(WINDOW_WIDTH / 2 - 180, 350);
        window.draw(nextText);
        
        Text hint("Press any key to continue...", font, 20);
        hint.setFillColor(Color(150, 150, 150));
        hint.setPosition(WINDOW_WIDTH / 2 - 120, 450);
        window.draw(hint);
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
        Text title("LEADERBOARD", font, 50);
        title.setFillColor(Color::Yellow);
        title.setPosition(WINDOW_WIDTH / 2 - 180, 50);
        window.draw(title);
        
        // Tab buttons
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
        
        // Display leaderboard based on active tab
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
                bgMusic1.stop();
                bgMusic2.stop();
                bgMusic3.stop();
            }
            sleep(milliseconds(200));
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