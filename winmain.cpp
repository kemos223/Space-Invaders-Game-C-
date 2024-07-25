#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

enum GameState {
    BEGINNING_STATE,
    PLAY_STATE,
    PAUSE_STATE,
    DEFEAT_STATE,
    NEXT_WAVE_STATE,
    WINNER_STATE,
};

enum Direction {
    Left,
    Right,
    Down
};

struct Ship {
    sf::Sprite sprite;
    bool isDying = false;
    int currentFrame = 0; 
    sf::Clock deathClock; 
};

struct Alien {
    sf::Sprite sprite;
    bool isDying = false;
    int currentFrame = 0;
    sf::Clock deathClock; 
    sf::Clock movementClock;
};

struct AlienBolt {
    sf::RectangleShape shape;
    bool active = false;
};

void beginState(sf::RenderWindow& window) {
    sf::Font font;
    if (!font.loadFromFile("Arcade.ttf")) {
    
    }

    sf::Text text("Press 'S' to Start", font, 50);

    text.setFillColor(sf::Color::White);
    text.setPosition(200, 250);
    window.clear();
    window.draw(text);
    window.display();

}

sf::Texture loadTexture(const std::string& filename) { 
    sf::Texture texture; 
    if (!texture.loadFromFile(filename)) { 
    }
    return texture; 
}

std::vector<std::vector<Alien>> initializeAliens(sf::Texture& alienTexture, const std::vector<sf::IntRect>& movementFrames) {
    std::vector<std::vector<Alien>> aliens; 
    for (int i = 0; i < 3; i++) { 
        std::vector<Alien> row; 
        for (int j = 0; j < 10; j++) {  
            sf::Sprite alienSprite(alienTexture); 
            alienSprite.setPosition(100 + j * 60, i * 60 + 50); 
            alienSprite.setTextureRect(movementFrames[0]); 
            row.push_back({ alienSprite });  
        }
        aliens.push_back(row);
    }
    return aliens;
}

Ship initializeShip(sf::Texture& shipTexture) {
    Ship ship;
    ship.sprite.setTexture(shipTexture);
    ship.sprite.setPosition(400, 500);
    return ship;
}

void loadFrames(std::vector<sf::IntRect>& frames, int frameWidth, int frameHeight, int startX, int startY, int count, int columns) {
    for (int i = 0; i < count; i++) {
        int x = startX + (i % columns) * frameWidth;
        int y = startY + (i / columns) * frameHeight;
        frames.push_back(sf::IntRect(x, y, frameWidth, frameHeight));
    } 
}

sf::RectangleShape initializeBarrier(sf::RenderWindow& window) {
    sf::RectangleShape barrier(sf::Vector2f(window.getSize().x, 1));
    barrier.setPosition(0, 5 * window.getSize().y / 7);
    barrier.setFillColor(sf::Color::White);
    return barrier;
}

void shipMovement(sf::RenderWindow& window, Ship& shipsprite, float time) {
    float shipSpeed = 200.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        shipsprite.sprite.move(0, -shipSpeed * time);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        shipsprite.sprite.move(-shipSpeed * time, 0);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        shipsprite.sprite.move(0, shipSpeed * time);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        shipsprite.sprite.move(shipSpeed * time, 0);
    }

    if (shipsprite.sprite.getPosition().x < 0) {
        shipsprite.sprite.setPosition(0, shipsprite.sprite.getPosition().y);
    }
    if (shipsprite.sprite.getPosition().x + shipsprite.sprite.getGlobalBounds().width > window.getSize().x) {
        shipsprite.sprite.setPosition(window.getSize().x - shipsprite.sprite.getGlobalBounds().width, shipsprite.sprite.getPosition().y);
    }
    if (shipsprite.sprite.getPosition().y < 5 * window.getSize().y / 7) {
        shipsprite.sprite.setPosition(shipsprite.sprite.getPosition().x, 5 * window.getSize().y / 7);
    }
    if (shipsprite.sprite.getPosition().y + shipsprite.sprite.getGlobalBounds().height > window.getSize().y) {
        shipsprite.sprite.setPosition(shipsprite.sprite.getPosition().x, window.getSize().y - shipsprite.sprite.getGlobalBounds().height);
    }
    window.draw(shipsprite.sprite);
}

void moveAliens(sf::RenderWindow& window, std::vector<std::vector<Alien>>& aliens, float& time, Direction& direction, bool& moveDown, std::vector<sf::IntRect>& movementFrames, std::vector<sf::IntRect>& deathFrames, float& alienSpeed, GameState& gamestate, sf::RectangleShape& barrier) {
    bool changeDirection = false;
    for (auto& row : aliens) {
        for (auto& alien : row) {
            if (alien.isDying) continue;
            if (alien.movementClock.getElapsedTime().asSeconds() > 0.5f) {
                alien.currentFrame = (alien.currentFrame + 1) % movementFrames.size(); 
                alien.sprite.setTextureRect(movementFrames[alien.currentFrame]);
                alien.movementClock.restart();
            }
            if (direction == Right) {
                alien.sprite.move(alienSpeed * time, 0);
                if (alien.sprite.getPosition().x + alien.sprite.getGlobalBounds().width > window.getSize().x) {
                    changeDirection = true;
                }
            }
            else if (direction == Left) {
                alien.sprite.move(-alienSpeed * time, 0);
                if (alien.sprite.getPosition().x < 0) {
                    changeDirection = true;
                }
            }
            if (alien.sprite.getPosition().y + alien.sprite.getGlobalBounds().height >= barrier.getPosition().y) {
                gamestate = DEFEAT_STATE;
            }
        }
    }

    if (changeDirection) { 
        direction = (direction == Right) ? Left : Right;
        moveDown = true;
    }

    if (moveDown) {
        for (auto& row : aliens) {
            for (auto& alien : row) {
                alien.sprite.move(0, 20);
            }
        }
        moveDown = false;
    }
}

void fireBolt(sf::RenderWindow& window, Ship& shipsprite, std::vector<sf::RectangleShape>& bolts, sf::Clock& fire, float& boltSpeed, float& time, sf::Sound& boltNoise) {
    if (shipsprite.isDying) return;
    
    static bool isSpacePressed = false; 
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) { 
        if (!isSpacePressed && bolts.size() < 3 && fire.getElapsedTime().asSeconds() >= 0.5f) { 
            sf::RectangleShape bolt(sf::Vector2f(8, 30)); 
            bolt.setFillColor(sf::Color::Blue); 
            bolt.setPosition(shipsprite.sprite.getPosition().x + shipsprite.sprite.getGlobalBounds().width / 2 - bolt.getSize().x / 2, shipsprite.sprite.getPosition().y);
            bolts.push_back(bolt); 
            boltNoise.play();
            isSpacePressed = true; 
            fire.restart(); 
        }
        else {
            isSpacePressed = false; 
        }
    }

    for (auto& bolt : bolts) {
        bolt.move(0, -boltSpeed * time);
    }

    for (const auto& bolt : bolts) {
        window.draw(bolt);
    }
}

bool areAliensRemaining(const std::vector<std::vector<Alien>>& aliens) {
    for (const auto& row : aliens) {
        if (!row.empty()) {
            return true;
        }
    }
    return false;
}

void alienBoltCollisons(sf::RenderWindow& window, std::vector<std::vector<Alien>>& aliens, std::vector<sf::RectangleShape>& bolts, std::vector<sf::IntRect>& deathFrames,sf::Sound& alienDestroyedSound) {
    for (auto& bolt : bolts) {
        for (auto& row : aliens) {
            for (auto& alien : row) {
                if (!alien.isDying && bolt.getGlobalBounds().intersects(alien.sprite.getGlobalBounds())) {
                    bolt.setPosition(-100, -100);
                    alien.isDying = true;
                    alienDestroyedSound.play();
                    alien.currentFrame = 0;
                    alien.sprite.setTextureRect(deathFrames[0]);
                    alien.deathClock.restart();
                }
            }
        }
    }

    for (auto& row : aliens) {
        for (auto& alien : row) {
            if (alien.isDying) {
                if (alien.deathClock.getElapsedTime().asSeconds() > 0.05f) {
                    alien.currentFrame++;

                    if (alien.currentFrame < deathFrames.size()) {
                        alien.sprite.setTextureRect(deathFrames[alien.currentFrame]);
                    }
                    else {
                        alien.sprite.setPosition(-100, -100);
                    }
                    alien.deathClock.restart();
                }
            }
            window.draw(alien.sprite);


        }
    }

    bolts.erase(std::remove_if(bolts.begin(), bolts.end(), [](const sf::RectangleShape& bolt) {
        return bolt.getPosition().y + bolt.getSize().y < 0 || bolt.getPosition().x == -100;
        }), bolts.end());

    for (auto& row : aliens) {
        row.erase(std::remove_if(row.begin(), row.end(), [](const Alien& alien) {
            return alien.sprite.getPosition().x == -100;
            }), row.end());
    }
}

void alienShootBolts(sf::RenderWindow& window, std::vector<std::vector<Alien>>& aliens, sf::Clock alienFireClock, AlienBolt& alienBolt, float time, float& alienBoltSpeed) {

    if (!alienBolt.active && alienFireClock.getElapsedTime().asSeconds() >= 6.0f) {
        std::vector<Alien*> activeAliens;
        for (auto& row : aliens) {
            for (auto& alien : row) {
                if (!alien.isDying) {
                    activeAliens.push_back(&alien);
                }
            }
        }

        if (!activeAliens.empty()) {
            Alien* shootingAlien = activeAliens[rand() % activeAliens.size()];
            alienBolt.shape.setPosition(shootingAlien->sprite.getPosition().x + shootingAlien->sprite.getGlobalBounds().width / 2, shootingAlien->sprite.getPosition().y + shootingAlien->sprite.getGlobalBounds().height);
            alienBolt.active = true;
            alienFireClock.restart();
        }
    }

    if (alienBolt.active) {
        alienBolt.shape.move(0, alienBoltSpeed * time);
        if (alienBolt.shape.getPosition().y > window.getSize().y) {
            alienBolt.active = false;
            alienBolt.shape.setPosition(-100, -100);
        }
        window.draw(alienBolt.shape);
    }
}

void shipBoltCollisions(AlienBolt& alienBolt, Ship& ship, int& lives, GameState& gameState, const std::vector<sf::IntRect>& shipDeathFrames, sf::Sound& shipDamage) {
    if (alienBolt.active && alienBolt.shape.getGlobalBounds().intersects(ship.sprite.getGlobalBounds()) && !ship.isDying) {
        lives--;
        alienBolt.active = false;
        shipDamage.play();
        alienBolt.shape.setPosition(-100, -100);
        if (lives <= 0) {
            ship.isDying = true;
        }
        ship.currentFrame = 0;
        ship.sprite.setTextureRect(shipDeathFrames[0]);
        ship.deathClock.restart();
    }
}

void boltCollisions(std::vector<sf::RectangleShape>& bolts, AlienBolt& alienBolt, sf::Sound& boltDestroyedSound) {
    if (alienBolt.active) {
        bolts.erase(std::remove_if(bolts.begin(), bolts.end(), [&alienBolt, &boltDestroyedSound](const sf::RectangleShape& bolt) {
            if (bolt.getGlobalBounds().intersects(alienBolt.shape.getGlobalBounds())) {
                alienBolt.active = false;
                boltDestroyedSound.play();
                alienBolt.shape.setPosition(-100, -100);
                return true;
            }
            return false;
            }), bolts.end());
    }
}  

bool isShipDeathAnimationDone(const Ship& ship, const std::vector<sf::IntRect>& deathFrames) {
    return ship.currentFrame >= deathFrames.size();
}


void shipDeathAnimation(sf::RenderWindow& window, Ship& ship, const std::vector<sf::IntRect>& deathFrames) {
    if (ship.isDying) {
        if (ship.deathClock.getElapsedTime().asSeconds() > 0.3f) {
            ship.currentFrame++;
            if (ship.currentFrame < deathFrames.size()) {
                ship.sprite.setTextureRect(deathFrames[ship.currentFrame]);
            }
            else {
                ship.sprite.setPosition(-100, -100); 
            }
            ship.deathClock.restart();
        }
        window.draw(ship.sprite);
    }
}

void handleBarrierCollisions(sf::RectangleShape& barrier, std::vector<sf::RectangleShape>& bolts, AlienBolt& alienBolt) {
    bolts.erase(std::remove_if(bolts.begin(), bolts.end(), [&barrier](const sf::RectangleShape& bolt) {
        return bolt.getGlobalBounds().intersects(barrier.getGlobalBounds());
        }), bolts.end());

    if (alienBolt.shape.getGlobalBounds().intersects(barrier.getGlobalBounds())) {
        alienBolt.active = false;
        alienBolt.shape.setPosition(-100, -100);
    }
}

void playState(sf::RenderWindow& window, std::vector<std::vector<Alien>>& aliens, Ship& shipsprite,float time, std::vector<sf::RectangleShape>& bolts,sf::Clock& fire, Direction& direction,bool& moveDown, std::vector<sf::IntRect>& movementFrames, std::vector<sf::IntRect>& deathFrames, sf::Texture& alienTexture, int& lives, GameState& gamestate, sf::Clock alienFire, AlienBolt& alienBolt, const std::vector<sf::IntRect>& shipDeathFrames, float& alienSpeed, float& alienBoltSpeed, int& wave, sf::RectangleShape& barrier, sf::Sound& shipBoltNoise, sf::Sound& alienDestoryedSound, sf::Sound& boltDestoryedSound, sf::Sound& shipDamage) {
    
    float shipSpeed = 200.0f;
    float boltSpeed = 300.0f;

    sf::Font font;
    if (!font.loadFromFile("ComicSans.ttf")) { 

    }

    sf::Text text("Lives: " + std::to_string(lives), font, 20);
    sf::Text waveText("Wave: " + std::to_string(wave), font, 20);

    waveText.setFillColor(sf::Color::Yellow);
    waveText.setPosition(20, 20);

    text.setFillColor(sf::Color::Yellow); 
    text.setPosition(700, 20);  
    window.clear();  
    window.draw(waveText);
    window.draw(barrier);
    window.draw(text); 

    moveAliens(window, aliens, time, direction, moveDown, movementFrames, deathFrames, alienSpeed, gamestate, barrier); 

    alienBoltCollisons(window, aliens, bolts, deathFrames, alienDestoryedSound);

    alienShootBolts(window, aliens, alienFire, alienBolt, time, alienBoltSpeed);
    
    shipBoltCollisions(alienBolt, shipsprite, lives, gamestate, shipDeathFrames, shipDamage);

    boltCollisions(bolts, alienBolt, boltDestoryedSound);

    shipDeathAnimation(window, shipsprite, shipDeathFrames);

    fireBolt(window, shipsprite, bolts, fire, boltSpeed, time, shipBoltNoise);
    
    shipMovement(window, shipsprite, time);

    if (shipsprite.isDying && isShipDeathAnimationDone(shipsprite, shipDeathFrames)) {
        if (lives <= 0) {
            gamestate = DEFEAT_STATE;
        }
        else {
            shipsprite.isDying = false;
            shipsprite.sprite.setPosition(400, 500);
        }
    }

    window.display();
}

void pauseState(sf::RenderWindow& window) {
    sf::Font font;
    if (!font.loadFromFile("RetroGame.ttf")) {

    }

    sf::Text text("Game Paused", font, 50);
    sf::Text text1("Press 'P' to Resume", font, 50);

    text.setFillColor(sf::Color::White);
    text.setPosition(190, 200);
    text1.setFillColor(sf::Color::White);
    text1.setPosition(75, 250);
    window.clear();
    window.draw(text);
    window.draw(text1);
    window.display();
}

void winnerState(sf::RenderWindow& window) { 
    sf::Font font;
    if (!font.loadFromFile("RetroGame.ttf")) {

    }

    sf::Text text("You Won!", font, 50);
    sf::Text text1("Press 'S' to Restart", font, 50);

    text.setFillColor(sf::Color::White);
    text.setPosition(260, 200);
    text1.setFillColor(sf::Color::White);
    text1.setPosition(50, 250);
    window.clear();
    window.draw(text);
    window.draw(text1);
    window.display();

}

void defeatState(sf::RenderWindow& window) {
    sf::Font font;
    if (!font.loadFromFile("RetroGame.ttf")) {

    }

    sf::Text text("You Lost!", font, 50);
    sf::Text text1("Press 'S' to Restart", font ,50);

    text.setFillColor(sf::Color::White);
    text.setPosition(230, 200);
    text1.setFillColor(sf::Color::White);
    text1.setPosition(50, 250);
    window.clear();
    window.draw(text); 
    window.draw(text1);
    window.display(); 
    
}

void nextWaveState(sf::RenderWindow& window, int& wave) {
    sf::Font font;
    if (!font.loadFromFile("RetroGame.ttf")) {

    }
    sf::Text text("Wave Complete", font ,50); 
    sf::Text text1("Press 'S' to Continue", font, 50);

    text.setFillColor(sf::Color::White);
    text.setPosition(150, 200);
    text1.setFillColor(sf::Color::White);
    text1.setPosition(30, 250);
    window.clear();
    window.draw(text);
    window.draw(text1);
    window.display();
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Alien Invaders");
    window.setFramerateLimit(60);

    GameState gameState = BEGINNING_STATE; 


  
    sf::Texture alienTexture = loadTexture("alien-strip1.png"); 
    sf::Texture shipTexture = loadTexture("ship.png"); 
    sf::Texture shipDeathTexture = loadTexture("ship-strip.png");

    std::vector<sf::IntRect> movementFrames; 
    std::vector<sf::IntRect> deathFrames; 
    std::vector<sf::IntRect> shipDeathFrames;  
    std::vector<sf::IntRect> wallDeathFrames;
    int movementFrameWidth = 36; 
    int movementFrameHeight = 36; 
    int deathFrameWidth = 36; 
    int deathFrameHeight = 36; 
    int shipDeathFrameWidth = 44; 
    int shipDeathFrameHeight = 44; 


    loadFrames(movementFrames, movementFrameWidth, movementFrameHeight, 0, 0, 2, 2);
    loadFrames(deathFrames, deathFrameWidth, deathFrameHeight, 0, 36, 4, 2);
    loadFrames(shipDeathFrames, shipDeathFrameWidth, shipDeathFrameHeight, 0, 0, 6, 6); 

     
    std::vector<std::vector<Alien>> aliens = initializeAliens(alienTexture, movementFrames); 
    Ship ship = initializeShip(shipTexture);
    ship.sprite.setTexture(shipDeathTexture);  

    sf::RectangleShape barrier = initializeBarrier(window);


    std::vector<sf::RectangleShape> bolts;
    Direction direction = Right;
    bool moveDown = false;
    int lives = 3;
    float alienSpeed = 50.0f;
    float alienBoltSpeed = 100.0f;
    int wave = 1;

    sf::Clock clock;
    sf::Clock fire;
    sf::Clock alienFire;

    AlienBolt alienBolt;
    alienBolt.shape.setSize(sf::Vector2f(8, 30)); 
    alienBolt.shape.setFillColor(sf::Color::Red); 
    alienBolt.shape.setPosition(-100, -100);  

    srand(static_cast<unsigned>(time(0)));

    sf::SoundBuffer shipBoltSound;
    sf::SoundBuffer alienDestroyedSound;
    sf::SoundBuffer boltDestoryedSound;
    sf::SoundBuffer shipTakesDamageSound;
    if (!shipBoltSound.loadFromFile("pew2.wav")) {
    }
    if (!alienDestroyedSound.loadFromFile("blast1.wav")) {
    }
    if (!boltDestoryedSound.loadFromFile("pop1.wav")) {
    }
    if (!shipTakesDamageSound.loadFromFile("blast2.wav")) {
    }
    sf::Sound shipSound(shipBoltSound); 
    sf::Sound alienDestroyed(alienDestroyedSound);
    sf::Sound boltDestoryed(boltDestoryedSound); 
    sf::Sound shipDamage(shipTakesDamageSound);

    

    while (window.isOpen())
    {
        sf::Event event;
        
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();


            if (event.type == sf::Event::KeyPressed) { 
                if (gameState == BEGINNING_STATE && event.key.code == sf::Keyboard::S) {
                    gameState = PLAY_STATE;
                }

                else if (gameState == PLAY_STATE && event.key.code == sf::Keyboard::P) {
                    gameState = PAUSE_STATE;
                }
                else if (gameState == PAUSE_STATE && event.key.code == sf::Keyboard::P) { 
                    gameState = PLAY_STATE;
                }
                else if (gameState == NEXT_WAVE_STATE && event.key.code == sf::Keyboard::S) {
                    aliens = initializeAliens(alienTexture, movementFrames);
                    barrier = initializeBarrier(window); 
                    ship.sprite.setPosition(400, 500);   
                    bolts.clear(); 
                    alienBolt.active = false; 
                    alienBolt.shape.setPosition(-100, -100);  
                    direction = Right;
                    moveDown = false;
                    gameState = PLAY_STATE; 
                }
                else if (gameState == WINNER_STATE && event.key.code == sf::Keyboard::S) {
                    aliens = initializeAliens(alienTexture, movementFrames); 
                    ship.sprite.setPosition(400, 500);  
                    bolts.clear(); 
                    lives = 3; 
                    alienSpeed = 50.0f;
                    alienBoltSpeed = 100.0f; 
                    wave = 1;
                    alienBolt.active = false;
                    alienBolt.shape.setPosition(-100, -100);
                    direction = Right; 
                    moveDown = false; 
                    gameState = PLAY_STATE;  
                }
                else if (gameState == DEFEAT_STATE && event.key.code == sf::Keyboard::S) {
                    aliens = initializeAliens(alienTexture, movementFrames); 
                    ship = initializeShip(shipTexture); 
                    ship.sprite.setTexture(shipDeathTexture); 
                    ship.sprite.setPosition(400, 500);  
                    bolts.clear(); 
                    lives = 3; 
                    alienSpeed = 50.0f;
                    alienBoltSpeed = 100.0f; 
                    wave = 1;
                    alienBolt.active = false; 
                    alienBolt.shape.setPosition(-100, -100); 
                    direction = Right;
                    moveDown = false; 
                    gameState = PLAY_STATE;
                }
            }
        }

        float deltaTime = clock.restart().asSeconds(); 

        switch (gameState) {
            case BEGINNING_STATE:
                beginState(window);
                break;
            case PLAY_STATE: {
                playState(window, aliens, ship, deltaTime, bolts, fire, direction,moveDown, movementFrames, deathFrames, alienTexture, lives, gameState, alienFire, alienBolt, shipDeathFrames, alienSpeed, alienBoltSpeed, wave, barrier,shipSound, alienDestroyed, boltDestoryed, shipDamage);
                if (!areAliensRemaining(aliens)) {
                    wave++;
                    alienSpeed += 10.0f;
                    alienBoltSpeed += 5.0f;
                    gameState = NEXT_WAVE_STATE;
                    direction = Right; 
                }
                if (!areAliensRemaining(aliens) && wave == 12) {
                    gameState = WINNER_STATE;
                }
            }
                break;
            case PAUSE_STATE:
                pauseState(window);
                break;
            case NEXT_WAVE_STATE:
                nextWaveState(window, wave);
                break;
            case WINNER_STATE:
                winnerState(window);
                break;
            case DEFEAT_STATE:
                defeatState(window);
                
        }  
    }
    return 0;
}