#include "snakegame.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>

using json = nlohmann::json;

// Constructor
SnakeGame::SnakeGame() : window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SnakeByte") {
    window.setFramerateLimit(60);
    srand(time(0));
    
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Font not loaded\n";
    }
    
    initializeLevels();
    
    // Load background images
    menuBgTexture.loadFromFile("assets/images/menu_bg.png");
    transitionBgTexture.loadFromFile("assets/images/level_complete.png");
    pauseBgTexture.loadFromFile("pause_bg.png");
    leaderboardBgTexture.loadFromFile("assets/images/menu_bg.png");
    settingsBgTexture.loadFromFile("assets/images/menu_bg.png");
    uiAreaBgTexture.loadFromFile("assets/images/score_ui.png");
    levelSelectBgTexture.loadFromFile("level_select_bg.png");
    gameoverBgTexture.loadFromFile("assets/images/gameoverbg_2.png");

    menuBg.setTexture(menuBgTexture);
    transitionBg.setTexture(transitionBgTexture);
    pauseBg.setTexture(pauseBgTexture);
    leaderboardBg.setTexture(leaderboardBgTexture);
    settingsBg.setTexture(settingsBgTexture);
    uiAreaBg.setTexture(uiAreaBgTexture);
    uiAreaBg.setTextureRect(IntRect(0, 0, WINDOW_WIDTH, UI_AREA_HEIGHT));
    levelSelectBg.setTexture(levelSelectBgTexture);
    gameoverBg.setTexture(gameoverBgTexture);
    
    loadSettings();
    state = MAIN_MENU;
    currentLevelIndex = 0;
    enteringName = false;
    leaderboardTab = 0;
    previousState = MAIN_MENU;
    isFreePlay = false;
    isLevelSelectMode = false;
    
    loadLeaderboard();
    initAudio();
    bonusFoodSpawnTimer.restart();
    speedBoostFoodSpawnTimer.restart();
    shrinkFoodSpawnTimer.restart();
    slowDownFoodSpawnTimer.restart();

    bonusFoodActive = false;
    speedBoostFoodActive = false;
    shrinkFoodActive = false;
    slowDownFoodActive = false;

    speedBoostActive = false;
    slowDownActive = false;

    showEffectText = false;
}

void SnakeGame::initializeLevels() {
    //TODO: Add more levels
    //TODO: Adjust Level targets
    std::string music1 = "assets/audios/genesis_flash.ogg";
    std::string music2 = "assets/audios/8bit_dash.ogg";
    std::string music3 = "assets/audios/arcade-speed-run.ogg";
    std::string bg1 = "assets/images/level1_bg.png";
    std::string bg2 = "assets/images/levelbg_2.png";
    
    Level level1(1, 60, "Level 1", music1, bg2, Color(0, 255, 0), 4.0f, true);
    levels.push_back(level1);
    
    Level level2(2, 120, "Level 2", music2, bg2, Color(255, 255, 0), 4.0f, true);
    level2.setupDefaultObstacles(PLAYFIELD_START_ROW);
    levels.push_back(level2);
    
    Level level3(3, -1, "Level 3", music3, bg2, Color(255, 100, 0), 5.0f, true);
    level3.setupDefaultObstacles(PLAYFIELD_START_ROW);
    levels.push_back(level3);
}

void SnakeGame::initAudio() {
    eatBuffer.loadFromFile("assets/audios/sound-effects/eat.wav");
    bonusBuffer.loadFromFile("assets/audios/sound-effects/bonus_eat.wav");
    collisionBuffer.loadFromFile("collision.wav");
    gameOverBuffer.loadFromFile("assets/audios/sound-effects/gameover2.wav");
    levelCompleteBuffer.loadFromFile("assets/audios/sound-effects/levelcomplete2.wav");
    speedBoostBuffer.loadFromFile("assets/audios/sound-effects/speedboost.wav");
    shrinkBuffer.loadFromFile("assets/audios/sound-effects/scissors.wav");
    slowDownBuffer.loadFromFile("assets/audios/sound-effects/slow down.wav");
    
    eatSound.setBuffer(eatBuffer);
    bonusSound.setBuffer(bonusBuffer);
    collisionSound.setBuffer(collisionBuffer);
    gameOverSound.setBuffer(gameOverBuffer);
    levelCompleteSound.setBuffer(levelCompleteBuffer);
    speedBoostSound.setBuffer(speedBoostBuffer);
    shrinkSound.setBuffer(shrinkBuffer);
    slowDownSound.setBuffer(slowDownBuffer); 
}

void SnakeGame::startGame(bool freePlay) {
    isFreePlay = freePlay;
    currentLevelIndex = 0;
    score = 0;
    resetLevel();
    state = PLAYING;
    loadLevelAssets();
    playBgMusic();
}

void SnakeGame::loadLevelAssets() {
    if (isFreePlay) {
        currentLevelBgTexture.loadFromFile(levels[0].bgImageFile);
        currentLevelBg.setTexture(currentLevelBgTexture);
    } else if (currentLevelIndex < levels.size()) {
        currentLevelBgTexture.loadFromFile(levels[currentLevelIndex].bgImageFile);
        currentLevelBg.setTexture(currentLevelBgTexture);
    }
}

void SnakeGame::playBgMusic() {
    currentBgMusic.stop();
    if (soundEnabled) {
        if (currentBgMusic.openFromFile(isFreePlay ? levels[0].bgMusicFile : levels[currentLevelIndex].bgMusicFile)) {
            currentBgMusic.setLoop(true);
            currentBgMusic.play();
        }
    }
}

void SnakeGame::resetLevel() {
    snake.clear();
    int centerX = GRID_WIDTH / 2;
    int centerY = PLAYFIELD_START_ROW + (PLAYFIELD_HEIGHT / 2);
    snake.push_back({centerX, centerY});
    snake.push_back({centerX - 1, centerY});
    snake.push_back({centerX - 2, centerY});
    
    direction = RIGHT;
    nextDirection = RIGHT;
    scoreMultiplier = 1.0f;
    speedMultiplier = 1.0f;
    consecutiveActive = false;
    bonusFoodActive = false;
    speedBoostFoodActive = false;
    shrinkFoodActive = false;
    slowDownFoodActive = false;
    speedBoostActive = false;
    slowDownActive = false;
    bonusFoodSpawnTimer.restart();
    speedBoostFoodSpawnTimer.restart();
    shrinkFoodSpawnTimer.restart();
    slowDownFoodSpawnTimer.restart();
    spawnFood(NORMAL);
    gameClock.restart();
    foodTimer.restart();
    moveInterval = 0.15f;
}

Level& SnakeGame::getCurrentLevel() {
    if (isFreePlay) {
        return levels[0];
    }
    return levels[currentLevelIndex];
}

void SnakeGame::spawnFood(FoodType foodType) {
        int x, y;
    bool valid;
    
    Food* targetFood;
    
    // Select which food to spawn
    switch(foodType) {
        case BONUS: targetFood = &bonusFood; break;
        case SPEED_BOOST: targetFood = &speedBoostFood; break;
        case SHRINK: targetFood = &shrinkFood; break;
        case SLOW_DOWN: targetFood = &slowDownFood; break;
        default: targetFood = &food; break;
    }
    
    do {
        valid = true;
        x = rand() % GRID_WIDTH;
        y = PLAYFIELD_START_ROW + 1 + rand() % (PLAYFIELD_HEIGHT - 2);
        
        // Check if inside border area
        if (!isFreePlay) {
            if (x == 0 || x == GRID_WIDTH - 1 || 
                y == PLAYFIELD_START_ROW || y == GRID_HEIGHT - 1) {
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
        
        // Check collision with ALL foods
        if (food.x == x && food.y == y) valid = false;
        if (bonusFoodActive && bonusFood.x == x && bonusFood.y == y) valid = false;
        if (speedBoostFoodActive && speedBoostFood.x == x && speedBoostFood.y == y) valid = false;
        if (shrinkFoodActive && shrinkFood.x == x && shrinkFood.y == y) valid = false;
        if (slowDownFoodActive && slowDownFood.x == x && slowDownFood.y == y) valid = false;
        
    } while (!valid);
    
    targetFood->x = x;
    targetFood->y = y;
    targetFood->type = foodType;
    targetFood->spawnTimer.restart();
    
    // Set active flags and durations
    if (foodType == BONUS) {
        bonusFoodActive = true;
        bonusFoodDuration = 5.0f + (rand() % 3);
        bonusFoodTimer.restart();
    } else if (foodType == SPEED_BOOST) {
        speedBoostFoodActive = true;
        speedBoostFoodDuration = 5.0f + (rand() % 3);
        speedBoostFoodTimer.restart();
    } else if (foodType == SHRINK) {
        shrinkFoodActive = true;
        shrinkFoodDuration = 5.0f + (rand() % 3);
        shrinkFoodTimer.restart();
    } else if (foodType == SLOW_DOWN) {
        slowDownFoodActive = true;
        slowDownFoodDuration = 5.0f + (rand() % 3);
        slowDownFoodTimer.restart();
    }
}

void SnakeGame::handleInput() {
    Event event;
    while (window.pollEvent(event)) {
        if (event.type == Event::Closed) {
            window.close();
        }
        
        if (state == MAIN_MENU) {
            handleMenuInput(event);
        } else if (state == LEVEL_SELECT) { 
            handleLevelSelectInput(event);
        } 
        else if (state == PLAYING) {
            handleGameInput(event);
        } else if (state == PAUSED) {
            if (event.type == Event::KeyPressed) {
                if(event.key.code == Keyboard::Escape || event.key.code == Keyboard::P || event.key.code == Keyboard::Space){
                    state = previousState;
                    if (soundEnabled) {
                        currentBgMusic.play();  // NEW: Resume music
                    }
                } else if(event.key.code == Keyboard::Q){
                    state = MAIN_MENU;
                    currentBgMusic.stop();
                }
            }
        } else if (state == GAME_OVER && enteringName) {
            handleNameInput(event);
        } else if (state == LEVEL_TRANSITION) {
            if (event.type == Event::KeyPressed) {
                if(event.key.code == Keyboard::Space || event.key.code == Keyboard::Enter){
                    advanceToNextLevel();
                }
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

void SnakeGame::advanceToNextLevel() {
    currentLevelIndex = nextLevelIndex;
    resetLevel();
    loadLevelAssets();
    state = PLAYING;
    playBgMusic();
}

void SnakeGame::handleMenuInput(Event& event) {
    if (event.type == Event::KeyPressed) {
        if (event.key.code == Keyboard::Num1 || event.key.code == Keyboard::P) {
            startGame(false);
        } else if (event.key.code == Keyboard::Num2 || event.key.code == Keyboard::F) {
            startGame(true);
        } else if (event.key.code == Keyboard::Num3 || event.key.code == Keyboard::V) {
            state = LEVEL_SELECT; 
        } else if (event.key.code == Keyboard::Num4 || event.key.code == Keyboard::L) {
            state = LEADERBOARD;
        } else if (event.key.code == Keyboard::Num5 || event.key.code == Keyboard::S) {
            state = SETTINGS;
        } else if (event.key.code == Keyboard::Num0 || event.key.code == Keyboard::Escape || event.key.code == Keyboard::Q) {
            window.close();
        }
    }
    
    if (event.type == Event::MouseButtonPressed) {
        Vector2i mousePos = Mouse::getPosition(window);
        
        if (mousePos.x >= 200 && mousePos.x <= 600) {
            // Option 1: Play Levels (y: 200-245)
            if (mousePos.y >= 200 && mousePos.y <= 245) {
                startGame(false);
            } 
            // Option 2: Free Play (y: 255-300)
            else if (mousePos.y >= 255 && mousePos.y <= 300) {
                startGame(true);
            } 
            // Option 3: Level Select (y: 310-355)
            else if (mousePos.y >= 310 && mousePos.y <= 355) {
                state = LEVEL_SELECT;
            } 
            // Option 4: Leaderboard (y: 365-410)
            else if (mousePos.y >= 365 && mousePos.y <= 410) {
                state = LEADERBOARD;
            } 
            // Option 5: Settings (y: 420-465)
            else if (mousePos.y >= 420 && mousePos.y <= 465) {
                state = SETTINGS;
            } 
            // Option 6: Exit (y: 475-520)
            else if (mousePos.y >= 475 && mousePos.y <= 520) {
                window.close();
            }
        }
    }
}


void SnakeGame::handleLevelSelectInput(Event& event) {
    if (event.type == Event::KeyPressed) {
        if (event.key.code == Keyboard::Escape || event.key.code == Keyboard::BackSpace) {
            state = MAIN_MENU;
            return;
        }
        
        // Select level by number key
        if (event.key.code >= Keyboard::Num1 && event.key.code <= Keyboard::Num9) {
            int selectedLevel = event.key.code - Keyboard::Num1;  // 0-based
            if (selectedLevel < levels.size()) {
                startLevelFromSelect(selectedLevel);
            }
        }
    }
    
    if (event.type == Event::MouseButtonPressed) {
        Vector2i mousePos = Mouse::getPosition(window);
        
        // Check level button clicks
        for (size_t i = 0; i < levels.size(); i++) {
            int yPos = 150 + i * 80;
            if (mousePos.x >= 200 && mousePos.x <= 600 && 
                mousePos.y >= yPos && mousePos.y <= yPos + 60) {
                startLevelFromSelect(i);
                break;
            }
        }
    }
}

void SnakeGame::handleGameInput(Event& event) {
    if (event.type == Event::KeyPressed) {
        if ((event.key.code == Keyboard::Up || event.key.code == Keyboard::W) && direction != DOWN) {
            nextDirection = UP;
        } else if ((event.key.code == Keyboard::Down || event.key.code == Keyboard::S) && direction != UP) {
            nextDirection = DOWN;
        } else if ((event.key.code == Keyboard::Left || event.key.code == Keyboard::A) && direction != RIGHT) {
            nextDirection = LEFT;
        } else if ((event.key.code == Keyboard::Right || event.key.code == Keyboard::D) && direction != LEFT) {
            nextDirection = RIGHT;
        } else if (event.key.code == Keyboard::Escape || event.key.code == Keyboard::P || event.key.code == Keyboard::Space) {
            previousState = PLAYING;
            state = PAUSED;
            currentBgMusic.pause();
        }
    }
}

void SnakeGame::handleNameInput(Event& event) {
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

void SnakeGame::startLevelFromSelect(int levelIndex) {
    isFreePlay = false;
    isLevelSelectMode = true;  // Mark as level select mode
    currentLevelIndex = levelIndex;
    score = 0;
    resetLevel();
    loadLevelAssets();
    state = PLAYING;
    playBgMusic();
}

void SnakeGame::update() {
    if (state == PAUSED || state == LEVEL_TRANSITION) return;
    if (state != PLAYING) return;

    if (soundEnabled && currentBgMusic.getStatus() != sf::Music::Playing) {
        playBgMusic();
    }
    
    float elapsed = gameClock.getElapsedTime().asSeconds();
    float currentInterval = moveInterval / speedMultiplier;
    
    if (elapsed >= currentInterval) {
        gameClock.restart();
        moveSnake();
    }
    
    // Hide effect popup after 3 seconds
    if (showEffectText && effectTextTimer.getElapsedTime().asSeconds() >= 3.0f) {
        showEffectText = false;
    }

    // Spawn bonus food (every 15-25 seconds)
    if (!bonusFoodActive && bonusFoodSpawnTimer.getElapsedTime().asSeconds() >= 15.0f + (rand() % 10)) {
        spawnFood(BONUS);
        bonusFoodSpawnTimer.restart();
    }
    
    // Spawn speed boost food (every 25-35 seconds)
    if (!speedBoostFoodActive && speedBoostFoodSpawnTimer.getElapsedTime().asSeconds() >= 25.0f + (rand() % 10)) {
        spawnFood(SPEED_BOOST);
        speedBoostFoodSpawnTimer.restart();
    }
    
    // Spawn shrink food (every 30-40 seconds)
    if (!shrinkFoodActive && shrinkFoodSpawnTimer.getElapsedTime().asSeconds() >= 30.0f + (rand() % 10)) {
        spawnFood(SHRINK);
        shrinkFoodSpawnTimer.restart();
    }
    
    // Spawn slow down food (every 35-45 seconds)
    if (!slowDownFoodActive && slowDownFoodSpawnTimer.getElapsedTime().asSeconds() >= 35.0f + (rand() % 10)) {
        spawnFood(SLOW_DOWN);
        slowDownFoodSpawnTimer.restart();
    }
    
    // Check food timeouts
    if (bonusFoodActive && bonusFoodTimer.getElapsedTime().asSeconds() >= bonusFoodDuration) {
        bonusFoodActive = false;
    }
    if (speedBoostFoodActive && speedBoostFoodTimer.getElapsedTime().asSeconds() >= speedBoostFoodDuration) {
        speedBoostFoodActive = false;
    }
    if (shrinkFoodActive && shrinkFoodTimer.getElapsedTime().asSeconds() >= shrinkFoodDuration) {
        shrinkFoodActive = false;
    }
    if (slowDownFoodActive && slowDownFoodTimer.getElapsedTime().asSeconds() >= slowDownFoodDuration) {
        slowDownFoodActive = false;
    }
    
    // Check speed boost effect timeout
    if (speedBoostActive && speedBoostEffectTimer.getElapsedTime().asSeconds() >= speedBoostEffectDuration) {
        speedBoostActive = false;
        if (isFreePlay) {
            speedMultiplier = 1.3f;  // Reset to default in free play
        } else {
            speedMultiplier = originalSpeedMultiplier;  // Restore saved speed in levels
        }
    }
    
    // Check slow down effect timeout
    if (slowDownActive && slowDownEffectTimer.getElapsedTime().asSeconds() >= slowDownEffectDuration) {
        slowDownActive = false;
        if (isFreePlay) {
            speedMultiplier = 1.3f;  // Reset to default in free play
        } else {
            speedMultiplier = originalSpeedMultiplier;  // Restore saved speed in levels
        }

    }
    
    if (consecutiveActive && consecutiveTimer.getElapsedTime().asSeconds() >= 2.0f) {
        scoreMultiplier = 1.0f;
        consecutiveActive = false;
    }
}

void SnakeGame::moveSnake() {
    direction = nextDirection;
    SnakeSegment newHead = snake.front();
    
    switch (direction) {
        case UP: newHead.y--; break;
        case DOWN: newHead.y++; break;
        case LEFT: newHead.x--; break;
        case RIGHT: newHead.x++; break;
    }
    
    if (isFreePlay) {
        speedMultiplier = 1.3f;
        if (newHead.x < 0) newHead.x = GRID_WIDTH - 1;
        if (newHead.x >= GRID_WIDTH) newHead.x = 0;
        if (newHead.y < PLAYFIELD_START_ROW) newHead.y = GRID_HEIGHT - 1;
        if (newHead.y >= GRID_HEIGHT) newHead.y = PLAYFIELD_START_ROW;
    } else {
        if (newHead.x <= 0 || newHead.x >= GRID_WIDTH - 1 || 
            newHead.y <= PLAYFIELD_START_ROW || newHead.y >= GRID_HEIGHT - 1) {
            gameOver();
            return;
        }
    }
    
    for (const auto& seg : snake) {
        if (seg.x == newHead.x && seg.y == newHead.y) {
            gameOver();
            return;
        }
    }
    
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
    
        spawnFood(NORMAL);
        ateFood = true;
    
        checkLevelCompletion();
    }

    // Check bonus food collision (yellow)
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

    // Check speed boost food collision (purple)
    if (speedBoostFoodActive && newHead.x == speedBoostFood.x && newHead.y == speedBoostFood.y) {
        // int points = 5;
        // score += static_cast<int>(points * scoreMultiplier);
    
        if (soundEnabled) {
            speedBoostSound.play();
        }
    
        // updateMultipliers();
    
        
        if (!speedBoostActive && !slowDownActive) {
            originalSpeedMultiplier = speedMultiplier;
        }
    
        // Cancel slow down if active
        if (slowDownActive) {
            slowDownActive = false;
        }
    
        speedBoostActive = true;
    
        
        if (isFreePlay) {
            speedMultiplier = 2.0f;
        } else {
            speedMultiplier = originalSpeedMultiplier * 1.5f;
        }
        speedBoostEffectDuration = 10.0f;
        speedBoostEffectTimer.restart();

        // Show effect popup
        activeEffectText = "SPEED BOOST ACTIVATED!";
        std::cout << "SPEED BOOST ACTIVATED!";
        showEffectText = true;                        
        effectTextTimer.restart(); 
    
        speedBoostFoodActive = false;
        speedBoostFoodSpawnTimer.restart();
        ateFood = true;
    
        checkLevelCompletion();
    }

    // Check shrink food collision (green)
    if (shrinkFoodActive && newHead.x == shrinkFood.x && newHead.y == shrinkFood.y) {
        // int points = 8;
        // score += static_cast<int>(points * scoreMultiplier);
    
        if (soundEnabled) {
            shrinkSound.play();
        }
    
        // updateMultipliers();
    
        // Shrink snake by 40% (needs adjusting)
        int removeCount = snake.size() * 0.4f;
        if (removeCount > 0 && snake.size() > 3) {  // Keep at least 3 segments
            for (int i = 0; i < removeCount && snake.size() > 3; i++) {
                snake.pop_back();
            }
        }

        //show effect popup
        activeEffectText = "SNAKE SHRANK!";
        std::cout << "SNAKE SHRANK!\n";
        showEffectText = true;
        effectTextTimer.restart();

        shrinkFoodActive = false;
        shrinkFoodSpawnTimer.restart();
        ateFood = true;
    
        checkLevelCompletion();
    }

    // Check slow down food collision (black)
    if (slowDownFoodActive && newHead.x == slowDownFood.x && newHead.y == slowDownFood.y) {
        // int points = 3;
        // score += static_cast<int>(points * scoreMultiplier);
    
        if (soundEnabled) {
            slowDownSound.play();
        }
    
        // updateMultipliers();
    
        // Activate slow down
        if (!slowDownActive && !speedBoostActive) {
            originalSpeedMultiplier = speedMultiplier;
        }
    
        // Cancel speed boost if active
        if (speedBoostActive) {
            speedBoostActive = false;
        }
    
        slowDownActive = true;
    
        
        if (isFreePlay) {
            speedMultiplier = 0.5f;
        } else {
            speedMultiplier = originalSpeedMultiplier * 0.5f;
        }
        slowDownEffectDuration = 10.0f;
        slowDownEffectTimer.restart();
        
        // Show effect popup
        activeEffectText = "SLOWED DOWN!";  
        std::cout << "SLOWED DOWN!";
        showEffectText = true;            
        effectTextTimer.restart(); 

        slowDownFoodActive = false;
        slowDownFoodSpawnTimer.restart();
        ateFood = true;
    
        checkLevelCompletion();
    }
    
    snake.push_front(newHead);
    if (!ateFood) snake.pop_back();
}

void SnakeGame::updateMultipliers() {
    if (consecutiveActive) {
        scoreMultiplier += 0.2f;
    } else {
        consecutiveActive = true;
        scoreMultiplier = 1.0f;
    }
    consecutiveTimer.restart();
}

void SnakeGame::checkLevelCompletion() {
    if (isFreePlay || isLevelSelectMode) return;
    Level& level = getCurrentLevel();
    if (level.scoreTarget > 0 && score >= level.scoreTarget) {
        levelComplete();
    }
}

void SnakeGame::levelComplete() {
    if (soundEnabled) levelCompleteSound.play();
    if (currentLevelIndex < levels.size() - 1) {
        nextLevelIndex = currentLevelIndex + 1;
        state = LEVEL_TRANSITION;
        transitionClock.restart();
        currentBgMusic.stop();
    } else {
        gameOver();
    }
}

void SnakeGame::gameOver() {
    if (soundEnabled) {
        gameOverSound.play();
    }
    
    currentBgMusic.stop();
    
    // Don't add to leaderboard if in level select mode
    if (!isLevelSelectMode) { 
        auto& targetLeaderboard = isFreePlay ? freePlayLeaderboard : leaderboard;
        
        if (score > 0 && (targetLeaderboard.size() < 5 || score > targetLeaderboard.back().second)) {
            enteringName = true;
            playerName = "";
        }
    }
    
    state = GAME_OVER;
    
    // Reset level select mode flag
    isLevelSelectMode = false;
}

void SnakeGame::render() {
    window.clear(Color(20, 20, 20));
    
    if (state == MAIN_MENU) {
        renderMainMenu();
    } else if (state == LEVEL_SELECT) { 
        renderLevelSelect();
    } else if (state == PLAYING || state == PAUSED) {
        renderGame();
        if (state == PAUSED) renderPauseOverlay();
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

void SnakeGame::renderMainMenu() {
    if (menuBgTexture.getSize().x > 0) {
        window.draw(menuBg);
    }
    
    Text title("SNAKEBYTE", font, 60);
    title.setStyle(Text::Bold);
    title.setFillColor(Color(75,44,107));
    title.setPosition(WINDOW_WIDTH / 2 - 150, 50);
    title.setOutlineThickness(1);
    title.setOutlineColor(Color::Black);
    window.draw(title);
    
    Text subtitle("Classic Snake Game", font, 20);
    subtitle.setFillColor(Color(117, 128, 103));
    subtitle.setPosition(WINDOW_WIDTH / 2 - 140, 130);
    window.draw(subtitle);
    
    std::vector<std::string> options = {
        "1. Play Quest",
        "2. Play Survival",
        "3. Select Levels",
        "4. Leaderboard",
        "5. Settings",
        "0. Exit"
    };
    
    Vector2i mousePos = Mouse::getPosition(window);
    
    for (size_t i = 0; i < options.size(); i++) {
        Text option(options[i], font, 22);  
        int yPos = 200 + i * 55;  
        
        if (mousePos.x >= 200 && mousePos.x <= 600 && 
            mousePos.y >= yPos && mousePos.y <= yPos + 45) {
            option.setFillColor(Color::Yellow);
        } else {
            option.setFillColor(Color::White);
        }
        
        option.setPosition(200, yPos); 
        window.draw(option);
    }
    
    Text hint("Use Arrow Keys or WASD to play", font, 18);
    hint.setFillColor(Color(100, 100, 100));
    hint.setPosition(WINDOW_WIDTH / 2 - 130, 540);
    window.draw(hint);
}

void SnakeGame::renderLevelSelect() {
    if (levelSelectBgTexture.getSize().x > 0) {
        window.draw(levelSelectBg);
    }
    
    Text title("SELECT LEVEL", font, 50);
    title.setFillColor(Color::Cyan);
    title.setPosition(WINDOW_WIDTH / 2 - 150, 50);
    window.draw(title);
    
    Vector2i mousePos = Mouse::getPosition(window);
    
    // Draw level buttons
    for (size_t i = 0; i < levels.size(); i++) {
        int yPos = 150 + i * 80;
        
        // Level button background
        RectangleShape levelButton(Vector2f(400, 60));
        levelButton.setPosition(200, yPos);
        
        // Highlight on hover
        if (mousePos.x >= 200 && mousePos.x <= 600 && 
            mousePos.y >= yPos && mousePos.y <= yPos + 60) {
            levelButton.setFillColor(Color(0, 100, 150));
            levelButton.setOutlineColor(Color::Cyan);
            levelButton.setOutlineThickness(3);
        } else {
            levelButton.setFillColor(Color(50, 50, 70));
            levelButton.setOutlineColor(Color(100, 100, 120));
            levelButton.setOutlineThickness(2);
        }
        window.draw(levelButton);
        
        // Level name and info
        std::string levelText = std::to_string(i + 1) + ". " + levels[i].name;
        // if (levels[i].scoreTarget > 0) {
        //     levelText += " - Target: " + std::to_string(levels[i].scoreTarget);
        // } else {
        //     levelText += " - Endless";
        // }
        
        Text levelName(levelText, font, 28);
        levelName.setFillColor(Color::White);
        levelName.setPosition(220, yPos + 15);
        window.draw(levelName);
    }
    
    Text hint("Press ESC to return to main menu", font, 20);
    hint.setFillColor(Color(150, 150, 150));
    hint.setPosition(WINDOW_WIDTH / 2 - 150, 520);
    window.draw(hint);
}


void SnakeGame::renderGame() {
    if (uiAreaBgTexture.getSize().x > 0) window.draw(uiAreaBg);
    if (currentLevelBgTexture.getSize().x > 0) {
        currentLevelBg.setPosition(0, UI_AREA_HEIGHT);
        window.draw(currentLevelBg);
    }
    
    Color wallColor(96, 26, 48);
    Color wallBorderColor(62, 21, 47);
    
    if (!isFreePlay) {
        for (int i = 0; i < GRID_WIDTH; i++) {
            RectangleShape borderSquare(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            borderSquare.setPosition(i * GRID_SIZE + 1, PLAYFIELD_START_ROW * GRID_SIZE + 1);
            borderSquare.setFillColor(wallColor);
            borderSquare.setOutlineColor(wallBorderColor);
            borderSquare.setOutlineThickness(1);
            window.draw(borderSquare);
        }
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            RectangleShape borderSquare(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            borderSquare.setPosition(i * GRID_SIZE + 1, (GRID_HEIGHT - 1) * GRID_SIZE + 1);
            borderSquare.setFillColor(wallColor);
            borderSquare.setOutlineColor(wallBorderColor);
            borderSquare.setOutlineThickness(1);
            window.draw(borderSquare);
        }
        
        for (int i = PLAYFIELD_START_ROW + 1; i < GRID_HEIGHT - 1; i++) {
            RectangleShape borderSquare(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            borderSquare.setPosition(1, i * GRID_SIZE + 1);
            borderSquare.setFillColor(wallColor);
            borderSquare.setOutlineColor(wallBorderColor);
            borderSquare.setOutlineThickness(1);
            window.draw(borderSquare);
        }
        
        for (int i = PLAYFIELD_START_ROW + 1; i < GRID_HEIGHT - 1; i++) {
            RectangleShape borderSquare(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            borderSquare.setPosition((GRID_WIDTH - 1) * GRID_SIZE + 1, i * GRID_SIZE + 1);
            borderSquare.setFillColor(wallColor);
            borderSquare.setOutlineColor(wallBorderColor);
            borderSquare.setOutlineThickness(1);
            window.draw(borderSquare);
        }
        
        for (const auto& obs : getCurrentLevel().obstacles) {
            RectangleShape rect(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
            rect.setPosition(obs.x * GRID_SIZE + 1, obs.y * GRID_SIZE + 1);
            rect.setFillColor(wallColor);
            rect.setOutlineColor(wallBorderColor);
            rect.setOutlineThickness(1);
            window.draw(rect);
        }
    }
    
    float normalPulseTime = foodTimer.getElapsedTime().asSeconds();
    float normalScale = 1.0f + 0.15f * sin(normalPulseTime * 4.0f);

    CircleShape foodCircle(GRID_SIZE / 2 - 2);
    foodCircle.setScale(normalScale, normalScale);
    foodCircle.setOrigin(GRID_SIZE / 2 - 2, GRID_SIZE / 2 - 2);
    foodCircle.setPosition(food.x * GRID_SIZE + GRID_SIZE / 2, food.y * GRID_SIZE + GRID_SIZE / 2);
    foodCircle.setFillColor(Color::Red);
    window.draw(foodCircle);

    // Draw bonus food (yellow)
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

    // Draw speed boost food (purple)
    if(!isFreePlay){
        if (speedBoostFoodActive) {
            float pulseTime = speedBoostFoodTimer.getElapsedTime().asSeconds();
            float scale = 1.0f + 0.2f * sin(pulseTime * 6.0f);
    
            CircleShape speedBoostCircle(GRID_SIZE / 2 + 2);
            speedBoostCircle.setScale(scale, scale);
            speedBoostCircle.setFillColor(Color(128, 0, 128));  // Purple
            speedBoostCircle.setOrigin(GRID_SIZE / 2 + 2, GRID_SIZE / 2 + 2);
            speedBoostCircle.setPosition(speedBoostFood.x * GRID_SIZE + GRID_SIZE / 2, 
                                 speedBoostFood.y * GRID_SIZE + GRID_SIZE / 2);
            window.draw(speedBoostCircle);
        }
    }

    // Draw shrink food (green)
    if(!isFreePlay){
        if (shrinkFoodActive) {
            float pulseTime = shrinkFoodTimer.getElapsedTime().asSeconds();
            float scale = 1.0f + 0.2f * sin(pulseTime * 6.0f);
    
            CircleShape shrinkCircle(GRID_SIZE / 2 + 2);
            shrinkCircle.setScale(scale, scale);
            shrinkCircle.setFillColor(Color(0, 255, 100));  // Bright green
            shrinkCircle.setOrigin(GRID_SIZE / 2 + 2, GRID_SIZE / 2 + 2);
            shrinkCircle.setPosition(shrinkFood.x * GRID_SIZE + GRID_SIZE / 2, 
                            shrinkFood.y * GRID_SIZE + GRID_SIZE / 2);
            window.draw(shrinkCircle);
        }
    }

    // Draw slow down food (black)
    if(!isFreePlay){
        if (slowDownFoodActive) {
            float pulseTime = slowDownFoodTimer.getElapsedTime().asSeconds();
            float scale = 1.0f + 0.2f * sin(pulseTime * 6.0f);
    
            CircleShape slowDownCircle(GRID_SIZE / 2 + 2);
            slowDownCircle.setScale(scale, scale);
            slowDownCircle.setFillColor(Color(50, 50, 50));  // Dark gray
            slowDownCircle.setOutlineColor(Color::White);
            slowDownCircle.setOutlineThickness(1);
            slowDownCircle.setOrigin(GRID_SIZE / 2 + 2, GRID_SIZE / 2 + 2);
            slowDownCircle.setPosition(slowDownFood.x * GRID_SIZE + GRID_SIZE / 2, 
                               slowDownFood.y * GRID_SIZE + GRID_SIZE / 2);
            window.draw(slowDownCircle);
        }
    }
    
    for (size_t i = 0; i < snake.size(); i++) {
        RectangleShape rect(Vector2f(GRID_SIZE - 2, GRID_SIZE - 2));
        rect.setPosition(snake[i].x * GRID_SIZE + 1, snake[i].y * GRID_SIZE + 1);
        rect.setFillColor(i == 0 ? Color(43, 69, 45) : Color(70, 114, 70));
        rect.setOutlineColor(Color(28, 46, 29));
        rect.setOutlineThickness(1);
        window.draw(rect);

        if (i == 0) {
            float headX = snake[i].x * GRID_SIZE;
            float headY = snake[i].y * GRID_SIZE;
            float eyeSize = 3.0f;
            float leftEyeX, leftEyeY, rightEyeX, rightEyeY;
            
            if (direction == UP) {
                leftEyeX = headX + 6; leftEyeY = headY + 6;
                rightEyeX = headX + 14; rightEyeY = headY + 6;
            } else if (direction == DOWN) {
                leftEyeX = headX + 6; leftEyeY = headY + 14;
                rightEyeX = headX + 14; rightEyeY = headY + 14;
            } else if (direction == LEFT) {
                leftEyeX = headX + 6; leftEyeY = headY + 6;
                rightEyeX = headX + 6; rightEyeY = headY + 14;
            } else {
                leftEyeX = headX + 14; leftEyeY = headY + 6;
                rightEyeX = headX + 14; rightEyeY = headY + 14;
            }
            
            CircleShape leftEye(eyeSize);
            leftEye.setPosition(leftEyeX, leftEyeY);
            leftEye.setFillColor(Color::White);
            window.draw(leftEye);
            
            CircleShape leftPupil(eyeSize / 2);
            leftPupil.setPosition(leftEyeX + eyeSize/2, leftEyeY + eyeSize/2);
            leftPupil.setFillColor(Color::Black);
            window.draw(leftPupil);
            
            CircleShape rightEye(eyeSize);
            rightEye.setPosition(rightEyeX, rightEyeY);
            rightEye.setFillColor(Color::White);
            window.draw(rightEye);
            
            CircleShape rightPupil(eyeSize / 2);
            rightPupil.setPosition(rightEyeX + eyeSize/2, rightEyeY + eyeSize/2);
            rightPupil.setFillColor(Color::Black);
            window.draw(rightPupil);
        }
    }
    
    Text scoreText("Score: " + std::to_string(score), font, 20);
    scoreText.setFillColor(Color::White);
    scoreText.setOutlineThickness(1);
    scoreText.setOutlineColor(Color::Black);
    scoreText.setPosition(10, 10);
    window.draw(scoreText);
    
    Text multText("x" + std::to_string(scoreMultiplier).substr(0, 4), font, 20);
    multText.setFillColor(Color::Yellow);
    multText.setOutlineThickness(1);
    multText.setOutlineColor(Color::Black);
    multText.setPosition(10, 35);
    window.draw(multText);
    
    if (!isFreePlay) {
        Text levelText("Level: " + std::to_string(getCurrentLevel().levelNumber), font, 20);
        levelText.setFillColor(Color::White);
        levelText.setOutlineThickness(1);
        levelText.setOutlineColor(Color::Black);
        levelText.setPosition(WINDOW_WIDTH - 120, 10);
        window.draw(levelText);
    } else {
        Text modeText("Survival", font, 20);
        modeText.setFillColor(Color::White);
        modeText.setOutlineThickness(1);
        modeText.setOutlineColor(Color::Black);
        modeText.setPosition(WINDOW_WIDTH - 120, 10);
        window.draw(modeText);
    }

    int timerYPos = 35;
    if (speedBoostActive) {
        int remainingTime = (int)(speedBoostEffectDuration - speedBoostEffectTimer.getElapsedTime().asSeconds());
        if (remainingTime < 0) remainingTime = 0;
        
        Text speedBoostTimer("Speed Boost: " + std::to_string(remainingTime) + " sec", font, 18);
        speedBoostTimer.setFillColor(Color::White);
        speedBoostTimer.setOutlineThickness(1);
        speedBoostTimer.setOutlineColor(Color::Black);
        speedBoostTimer.setPosition(WINDOW_WIDTH - 160, timerYPos);
        window.draw(speedBoostTimer);
        timerYPos += 25;
    }
    
    if (slowDownActive) {
        int remainingTime = (int)(slowDownEffectDuration - slowDownEffectTimer.getElapsedTime().asSeconds());
        if (remainingTime < 0) remainingTime = 0;
        
        Text slowDownTimer("Slow Down: " + std::to_string(remainingTime) + " sec", font, 18);
        slowDownTimer.setFillColor(Color::White);
        slowDownTimer.setOutlineThickness(1);
        slowDownTimer.setOutlineColor(Color::Black);
        slowDownTimer.setPosition(WINDOW_WIDTH - 160, timerYPos);
        window.draw(slowDownTimer);
    }

    if (showEffectText) {
        
        Text effectPopup(activeEffectText, font, 15);
        effectPopup.setFillColor(Color::Yellow);
        effectPopup.setOutlineColor(Color::Black);
        effectPopup.setOutlineThickness(1);
        effectPopup.setStyle(Text::Bold);  
        
        // Center the text
        FloatRect textBounds = effectPopup.getLocalBounds();
        effectPopup.setPosition((WINDOW_WIDTH - textBounds.width) / 2, 
                               WINDOW_HEIGHT / 2 - 125);
        window.draw(effectPopup);
    }

}

void SnakeGame::renderLevelTransition() {
    if (transitionBgTexture.getSize().x > 0) window.draw(transitionBg);
    
    Text title("LEVEL " + std::to_string(levels[currentLevelIndex].levelNumber) + " COMPLETE!", font, 50);
    title.setFillColor(Color::Green);
    title.setStyle(Text::Bold);
    title.setOutlineThickness(1);
    title.setOutlineColor(Color::Black);
    title.setPosition(WINDOW_WIDTH / 2 - 250, 125);
    window.draw(title);
    
    Text scoreText("Score: " + std::to_string(score), font, 40);
    scoreText.setFillColor(Color::White);
    scoreText.setOutlineThickness(1);
    scoreText.setOutlineColor(Color::Black);
    scoreText.setPosition(WINDOW_WIDTH / 2 - 100, 250);
    window.draw(scoreText);
    
    // Text nextText("Advancing to Level " + std::to_string(levels[nextLevelIndex].levelNumber), font, 35);
    // nextText.setFillColor(Color::Yellow);
    // nextText.setPosition(WINDOW_WIDTH / 2 - 180, 350);
    // window.draw(nextText);
    
    // Text resetInfo("Snake reset to default size and speed", font, 20);
    // resetInfo.setFillColor(Color(150, 200, 150));
    // resetInfo.setPosition(WINDOW_WIDTH / 2 - 180, 400);
    // window.draw(resetInfo);
    
    Text hint("Press Space or Enter to continue...", font, 22);
    hint.setFillColor(Color(48,48,48));
    hint.setPosition(WINDOW_WIDTH / 2 - 150, 480);
    window.draw(hint);
}

void SnakeGame::renderPauseOverlay() {
    RectangleShape overlay(Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    overlay.setFillColor(Color(0, 0, 0, 180));
    window.draw(overlay);
    
    if (pauseBgTexture.getSize().x > 0) {
        pauseBg.setColor(Color(255, 255, 255, 200));
        window.draw(pauseBg);
    }
    
    Text title("GAME PAUSED", font, 60);
    title.setFillColor(Color::Yellow);
    title.setStyle(Text::Bold);
    title.setOutlineThickness(1);
    title.setOutlineColor(Color::Black);
    title.setPosition(WINDOW_WIDTH / 2 - 180, 200);
    window.draw(title);
    
    Text hint("Press ESC/P/Space to continue \n Press Q to return to Main Menu", font, 25);
    hint.setFillColor(Color::White);
    hint.setPosition(WINDOW_WIDTH / 2 - 150, 320);
    window.draw(hint);
    
    Text controls("Controls: Arrow Keys or WASD", font, 20);
    controls.setFillColor(Color(200, 200, 200));
    controls.setPosition(WINDOW_WIDTH / 2 - 140, 400);
    window.draw(controls);
    
    Text pauseKey("Pause: ESC/P/Space", font, 20);
    pauseKey.setFillColor(Color(200, 200, 200));
    pauseKey.setPosition(WINDOW_WIDTH / 2 - 80, 430);
    window.draw(pauseKey);
}

void SnakeGame::renderGameOver() {

    if (gameoverBgTexture.getSize().x > 0) {
        window.draw(gameoverBg);
    }

    Text title("GAME OVER", font, 50);
    title.setFillColor(Color::Red);
    title.setStyle(Text::Bold);
    title.setOutlineThickness(1);
    title.setOutlineColor(Color::Black);
    title.setPosition(WINDOW_WIDTH / 2 - 150, 200);
    window.draw(title);
    
    Text scoreText("Final Score: " + std::to_string(score), font, 30);
    scoreText.setFillColor(Color::White);
    scoreText.setPosition(WINDOW_WIDTH / 2 - 120, 280);
    window.draw(scoreText);
    
    if (enteringName) {
        Text prompt("Enter your name:", font, 25);
        prompt.setFillColor(Color::Yellow);
        prompt.setPosition(WINDOW_WIDTH / 2 - 120, 330);
        window.draw(prompt);
        
        Text nameText(playerName + "_", font, 30);
        nameText.setFillColor(Color::White);
        nameText.setPosition(WINDOW_WIDTH / 2 - 100, 370);
        window.draw(nameText);
    } else {
        Text back("Press ESC to return to menu", font, 20);
        back.setFillColor(Color(150, 150, 150));
        back.setPosition(WINDOW_WIDTH / 2 - 150, 500);
        window.draw(back);
    }
}

void SnakeGame::renderLeaderboard() {
    if (leaderboardBgTexture.getSize().x > 0) window.draw(leaderboardBg);
    
    Text title("LEADERBOARD", font, 50);
    title.setFillColor(Color::Yellow);
    title.setStyle(Text::Bold);
    title.setOutlineThickness(1);
    title.setOutlineColor(Color::Black);
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
    
    Text normalText("Quest", font, 20);
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
    
    Text freeText("Survival", font, 20);
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

void SnakeGame::renderSettings() {
    if (settingsBgTexture.getSize().x > 0) window.draw(settingsBg);
    
    Text title("SETTINGS", font, 50);
    title.setFillColor(Color::Cyan);
    title.setStyle(Text::Bold);
    title.setOutlineThickness(1);
    title.setOutlineColor(Color::Black);
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

void SnakeGame::loadSettings() {
    std::ifstream file("settings.json");
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            file.close();
            if (j.contains("sound")) {
                soundEnabled = j["sound"].get<bool>();
            } else {
                soundEnabled = true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading settings: " << e.what() << std::endl;
            soundEnabled = true;
            saveSettings();
        }
    } else {
        soundEnabled = true;
        saveSettings();
    }
}

void SnakeGame::saveSettings() {
    json j;
    j["sound"] = soundEnabled;
    std::ofstream file("settings.json");
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    }
}

void SnakeGame::loadLeaderboard() {
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

void SnakeGame::saveLeaderboard() {
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

void SnakeGame::addToLeaderboard(const std::string& name, int newScore, bool isFreePlay) {
    auto& targetLeaderboard = isFreePlay ? freePlayLeaderboard : leaderboard;
    targetLeaderboard.push_back({name, newScore});
    std::sort(targetLeaderboard.begin(), targetLeaderboard.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    if (targetLeaderboard.size() > 5) {
        targetLeaderboard.resize(5);
    }
}

void SnakeGame::run() {
    while (window.isOpen()) {
        handleInput();
        update();
        render();
    }
}