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
enum GameState { MAIN_MENU, PLAYING, GAME_OVER, LEADERBOARD, SETTINGS };
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
    Clock foodTimer;
    bool bonusFoodActive;
    Clock bonusFoodTimer;
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
    
    // Obstacles
    std::vector<Obstacle> obstacles;
    
    // Audio
    Music bgMusic1, bgMusic2, bgMusic3;
    SoundBuffer eatBuffer, bonusBuffer, collisionBuffer, gameOverBuffer, levelCompleteBuffer;
    Sound eatSound, bonusSound, collisionSound, gameOverSound, levelCompleteSound;
    bool soundEnabled;
    
    // Leaderboard
    std::vector<std::pair<std::string, int>> leaderboard;
    std::string playerName;
    bool enteringName;
    
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
        
        soundEnabled = true;
        state = MAIN_MENU;
        currentLevel = 1;
        enteringName = false;
        
        loadLeaderboard();
        initAudio();
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
        
        score = 0;
        scoreMultiplier = 1.0f;
        speedMultiplier = 1.0f;
        consecutiveActive = false;
        
        bonusFoodActive = false;
        
        obstacles.clear();
        setupLevel();
        spawnFood(false);
        
        gameClock.restart();
        moveInterval = 0.15f;
        
        if (mode == FREE_PLAY) {
            levelTarget = -1;
        } else {
            levelTarget = currentLevel * 10;
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
        } while (!valid);
        
        food.x = x;
        food.y = y;
        food.isBonus = bonus;
        food.spawnTimer.restart();
        
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
            } else if (state == GAME_OVER || state == LEADERBOARD || state == SETTINGS) {
                if (event.type == Event::KeyPressed) {
                    if (event.key.code == Keyboard::Escape || event.key.code == Keyboard::BackSpace) {
                        state = MAIN_MENU;
                    }
                }
            }
        }
    }
    
    void handleMenuInput(Event& event) {
        if (event.type == Event::KeyPressed) {
            if (event.key.code == Keyboard::Num1) {
                startGame(LEVEL_1);
            } else if (event.key.code == Keyboard::Num2) {
                startGame(FREE_PLAY);
            } else if (event.key.code == Keyboard::Num3) {
                state = LEADERBOARD;
            } else if (event.key.code == Keyboard::Num4) {
                state = SETTINGS;
            } else if (event.key.code == Keyboard::Num5 || event.key.code == Keyboard::Escape) {
                window.close();
            }
        }
    }
    
    void handleGameInput(Event& event) {
        if (event.type == Event::KeyPressed) {
            if (event.key.code == Keyboard::Up && direction != DOWN) {
                nextDirection = UP;
            } else if (event.key.code == Keyboard::Down && direction != UP) {
                nextDirection = DOWN;
            } else if (event.key.code == Keyboard::Left && direction != RIGHT) {
                nextDirection = LEFT;
            } else if (event.key.code == Keyboard::Right && direction != LEFT) {
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
                    addToLeaderboard(playerName, score);
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
        if (state != PLAYING) return;
        
        float elapsed = gameClock.getElapsedTime().asSeconds();
        float currentInterval = moveInterval / speedMultiplier;
        
        if (elapsed >= currentInterval) {
            gameClock.restart();
            moveSnake();
        }
        
        // Check for bonus food spawn
        if (!bonusFoodActive && !food.isBonus) {
            if (rand() % 300 == 0) { // Random chance
                spawnFood(true);
            }
        }
        
        // Check bonus food timeout
        if (bonusFoodActive && bonusFoodTimer.getElapsedTime().asSeconds() >= bonusFoodDuration) {
            bonusFoodActive = false;
            spawnFood(false);
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
        
        snake.push_front(newHead);
        
        // Check food collision
        if (newHead.x == food.x && newHead.y == food.y) {
            int points = food.isBonus ? 10 : 2;
            score += static_cast<int>(points * scoreMultiplier);
            
            if (soundEnabled) {
                if (food.isBonus) {
                    bonusSound.play();
                } else {
                    eatSound.play();
                }
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
            spawnFood(false);
            
            // Check level completion
            if (mode != FREE_PLAY && score >= levelTarget) {
                levelComplete();
            }
        } else {
            snake.pop_back();
        }
    }
    
    void levelComplete() {
        if (soundEnabled) {
            levelCompleteSound.play();
        }
        
        if (currentLevel < 3) {
            currentLevel++;
            resetLevel();
            playBgMusic();
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
        
        // Check if score qualifies for leaderboard
        if (leaderboard.size() < 5 || score > leaderboard.back().second) {
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
        Text title("SNAKEBYTE", font, 60);
        title.setFillColor(Color::Green);
        title.setPosition(WINDOW_WIDTH / 2 - 150, 50);
        window.draw(title);
        
        Text subtitle("Classic Snake Game", font, 20);
        subtitle.setFillColor(Color(150, 150, 150));
        subtitle.setPosition(WINDOW_WIDTH / 2 - 100, 130);
        window.draw(subtitle);
        
        std::vector<std::string> options = {
            "1. Play Levels",
            "2. Free Play",
            "3. Leaderboard",
            "4. Settings",
            "5. Exit"
        };
        
        for (size_t i = 0; i < options.size(); i++) {
            Text option(options[i], font, 30);
            option.setFillColor(Color::White);
            option.setPosition(WINDOW_WIDTH / 2 - 100, 220 + i * 60);
            window.draw(option);
        }
    }
    
    void renderGame() {
        // Draw borders for non-free play
        if (mode != FREE_PLAY) {
            RectangleShape border(Vector2f(WINDOW_WIDTH - 4, WINDOW_HEIGHT - 4));
            border.setPosition(2, 2);
            border.setFillColor(Color::Transparent);
            border.setOutlineColor(Color(100, 100, 100));
            border.setOutlineThickness(2);
            window.draw(border);
        }
        
        // Draw obstacles
        for (const auto& obs : obstacles) {
            RectangleShape rect(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            rect.setPosition(obs.x * GRID_SIZE + 1, obs.y * GRID_SIZE + 1);
            rect.setFillColor(Color(150, 50, 50));
            window.draw(rect);
        }
        
        // Draw food
        RectangleShape foodRect(Vector2f(food.isBonus ? GRID_SIZE : GRID_SIZE - 4, 
                                         food.isBonus ? GRID_SIZE : GRID_SIZE - 4));
        foodRect.setPosition(food.x * GRID_SIZE + (food.isBonus ? 0 : 2), 
                            food.y * GRID_SIZE + (food.isBonus ? 0 : 2));
        foodRect.setFillColor(food.isBonus ? Color::Yellow : Color::Red);
        window.draw(foodRect);
        
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
        
        for (size_t i = 0; i < leaderboard.size() && i < 5; i++) {
            std::string entry = std::to_string(i + 1) + ". " + 
                               leaderboard[i].first + " - " + 
                               std::to_string(leaderboard[i].second);
            Text text(entry, font, 30);
            text.setFillColor(Color::White);
            text.setPosition(WINDOW_WIDTH / 2 - 150, 150 + i * 60);
            window.draw(text);
        }
        
        Text back("Press ESC to return", font, 20);
        back.setFillColor(Color(150, 150, 150));
        back.setPosition(WINDOW_WIDTH / 2 - 120, 500);
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
                for (auto& entry : j) {
                    leaderboard.push_back({entry["name"], entry["score"]});
                }
            } catch (...) {
                leaderboard.clear();
            }
            file.close();
        }
    }
    
    void saveLeaderboard() {
        json j = json::array();
        for (const auto& entry : leaderboard) {
            j.push_back({{"name", entry.first}, {"score", entry.second}});
        }
        
        std::ofstream file("leaderboard.json");
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
        }
    }
    
    void addToLeaderboard(const std::string& name, int newScore) {
        leaderboard.push_back({name, newScore});
        std::sort(leaderboard.begin(), leaderboard.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        if (leaderboard.size() > 5) {
            leaderboard.resize(5);
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