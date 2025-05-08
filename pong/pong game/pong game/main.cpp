#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <fstream>
#include <cmath>
#include <random>
#include <chrono>
#include <string>
typedef unsigned long long ull;
using namespace sf;
ContextSettings settings;
RenderWindow window(VideoMode({ 1000, 600 }), "Pong++ :3", Style::Titlebar | Style::Close, State::Windowed, settings);
RectangleShape background;

sf::View view({ 500.f, 300.f }, { 1000.f, 600.f });

RectangleShape paddle1({ 40.f, 120.f });
RectangleShape paddle2({ 40.f, 120.f });
Vector2i mousePos = Mouse::getPosition();

RectangleShape topWall({ 1000.f, 100.f });
RectangleShape bottomWall({ 1000.f, 100.f });
RectangleShape leftWall({ 0.f, 00.f });
RectangleShape rightWall({ 0.f, 00.f });

float shakeIntensity = 0.f;
float shakeDuration = 0.f;
float shakeElapsed = 0.f;

CircleShape ball(10.f);

sf::Image icon("assets/logo.bmp");

Font font("assets/fonts/PublicPixel.ttf");
Text scoreText(font);
Text HS(font);

SoundBuffer loseBuffer("assets/Sounds/lose.wav");
Sound soundLose(loseBuffer);
SoundBuffer noiceBuffer("assets/Sounds/noice.wav");
Sound soundNoice(noiceBuffer);
SoundBuffer wallHitBuffer("assets/Sounds/wall_hit.wav");
Sound wallHitSound(wallHitBuffer);
SoundBuffer hit1Buffer("assets/Sounds/hit1.wav");
Sound soundHit1(hit1Buffer);
SoundBuffer hit2Buffer("assets/Sounds/hit2.wav");
Sound soundHit2(hit2Buffer);
SoundBuffer hit3Buffer("assets/Sounds/hit3.wav");
Sound soundHit3(hit3Buffer);
SoundBuffer hit4Buffer("assets/Sounds/hit4.wav");
Sound soundHit4(hit4Buffer);

bool noicePlayed = 0;

ull score = 0, highscore = 0;

void winLoop();

void readHighscore()
{
    std::ifstream fin("assets/highscore.txt");
    std::string line;
    if (std::getline(fin, line))
    {
        try {
            highscore = std::stoi(line);
        }
        catch (const std::invalid_argument& e) {
            highscore = 0;
        }
    }
    fin.close();
}

void saveHighScore(ull hsNum) {
    std::ifstream fin("assets/highscore.txt");
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(fin, line))
        lines.push_back(line);
    fin.close();

    if (!lines.empty() && lines[0] == std::to_string(hsNum))
        return;

    if (!lines.empty())
        lines[0] = std::to_string(hsNum);
    else
        lines.push_back(std::to_string(hsNum));

    std::ofstream fout("assets/highscore.txt");
    for (const auto& l : lines)
        fout << l << '\n';
    fout.close();
}

int getRand(int min, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}
float absoluteValueAddition(float value, float increment)
{
    return (value > 0 ? increment : (value < 0 ? -increment : 0));
}
bool ballPaddleCollision()
{
    return ball.getGlobalBounds().findIntersection(paddle1.getGlobalBounds())
        ||
        ball.getGlobalBounds().findIntersection(paddle2.getGlobalBounds());
}
float clamp(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
void shakeScreen(float intensity, float duration) {
    shakeIntensity = intensity;
    shakeDuration = duration;
    shakeElapsed = 0.f; // reset elapsed time for the new shake
}

void hitSoundRandom()
{
    if (score == 69 && !noicePlayed)
        soundNoice.play(), noicePlayed = 1;
    int soundNum = getRand(1, 4);
    switch (soundNum)
    {
    case 1:
        soundHit1.play();
        return;
    case 2:
        soundHit2.play();
        return;
    case 3:
        soundHit3.play();
        return;
    case 4:
        soundHit4.play();
        return;
    }
}

void drawTitleScreen() {

    Text title(font);
    title.setString("Pong++");
    title.setCharacterSize(80);
    title.setFillColor(sf::Color(255, 180, 200));
    auto titleBounds = title.getLocalBounds();
    title.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f, titleBounds.position.x + titleBounds.size.x / 2.f});
    title.setPosition({ window.getSize().x / 2.f, window.getSize().y / 2.f });

    Text subtitle(font);
    subtitle.setString("Click anywhere to start! :D");
    subtitle.setCharacterSize(24);
    subtitle.setFillColor(sf::Color(200, 200, 255)); // soft blue
    auto subtitleBounds = subtitle.getLocalBounds();
    subtitle.setOrigin({ subtitleBounds.position.x + subtitleBounds.size.x / 2.f, subtitleBounds.position.y + subtitleBounds.size.y / 2.f });
    subtitle.setPosition({ window.getSize().x / 2.f, window.getSize().y * 2.f / 3.f });

    while (window.isOpen()) {
        Event event();
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (event->is<sf::Event::MouseButtonPressed>())
                return; // exit to restart the game
        }

        window.clear(Color(24, 24, 37, 255));
        window.draw(topWall);
        window.draw(bottomWall);
        window.draw(leftWall);
        window.draw(rightWall);

        window.draw(paddle1);
        window.draw(paddle2);
        window.draw(ball);

        window.draw(title);
        window.draw(subtitle);
        window.display();
    }
}

Vector2f ballSpeed = { 0.8f, 0.3f };
float deltaHeight = 0;
const float error = 100.f;
float paddle2Noise;
float finalY = 300.f; //this is for da simulating thingy
float H = window.getSize().y - ball.getRadius() * 2;

void finalPosY()
{
    float t = abs(paddle2.getPosition().x - ball.getPosition().x) / abs(ballSpeed.x);
    float y_naive = ball.getPosition().y + (deltaHeight)*t; //deltaHeight is the speed itself
    //^ if there were no hits by walls, and naive sounds like a cool thing to call it :p
    float y_cycles = fmod(y_naive, 2 * H);
    if (y_cycles < 0)
        y_cycles += 2 * H;
    finalY = (y_cycles <= H ? y_cycles : (2 * H - y_cycles));
}

void ballMove()
{
    Vector2f ballPos = ball.getPosition();
    Vector2f paddle1Center = paddle1.getPosition() + Vector2f(paddle1.getSize().x / 2.f, paddle1.getSize().y / 2.f);
    Vector2f paddle2Center = paddle2.getPosition() + Vector2f(paddle2.getSize().x / 2.f, paddle2.getSize().y / 2.f);

    if (ballPaddleCollision())
    {
        if (ballSpeed.x > 0)
        {
            score++;
            scoreText.setString(std::to_string(score));
            FloatRect textBounds = scoreText.getLocalBounds();
            scoreText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f });
            scoreText.setPosition({ window.getSize().x / 2.f, window.getSize().y / 2.f });
        }

        Vector2f paddle1Center = paddle1.getPosition() + Vector2f(paddle1.getSize().x / 2.f, paddle1.getSize().y / 2.f);
        Vector2f paddle2Center = paddle2.getPosition() + Vector2f(paddle2.getSize().x / 2.f, paddle2.getSize().y / 2.f);
        Vector2f ballPos = ball.getPosition();
        bool hitPaddle1 = (std::abs(ballPos.x - paddle1Center.x) < std::abs(ballPos.x - paddle2Center.x));
        if ((hitPaddle1 && ballSpeed.x > 0) || (!hitPaddle1 && ballSpeed.x < 0))
        {
            hitSoundRandom();
            HS.setString((std::max(highscore, score) == 42 ? "The answer to everything:" : "HS:") + std::to_string(std::max(highscore, score)));
            ballSpeed.x *= -1;
            float closerPaddleY = hitPaddle1 ? paddle1Center.y : paddle2Center.y;
            deltaHeight = (ballPos.y - closerPaddleY) * abs(ballSpeed.x) / error;
            deltaHeight = clamp(deltaHeight, -1.f, 1.f);
            paddle2Noise = getRand(-30, 30);
            shakeScreen(1, 0.1f);
            ballSpeed.x += absoluteValueAddition(ballSpeed.x, 0.01);
            finalPosY();
        }
    }
    if (ballPos.y + ball.getRadius() * 2 >= bottomWall.getPosition().y || ballPos.y <= 10) //WALL HIT :D
    {
        wallHitSound.play();
        deltaHeight *= -1;
        shakeScreen(1, 1);
    }
    ball.setPosition({ ballPos.x - ballSpeed.x, ballPos.y + deltaHeight });
    //if (ballSpeed.x < 0) //oh no, our table
       // finalPosY();  // it's broken </3
}

void gameOverScreen()
{
    saveHighScore(std::max(score, highscore));
    soundLose.play();
    Text retryText(font);
    retryText.setFont(font);
    retryText.setString("Retry? :P");
    retryText.setCharacterSize(48);
    // Center stuff
    retryText.setOrigin({ retryText.getCharacterSize() * retryText.getString().getSize() / 2.f, retryText.getCharacterSize() / 2.f });
    retryText.setPosition({ 500.f, 300.f });

    while (window.isOpen())
    {
        Event event();
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (event->is<sf::Event::MouseButtonPressed>())
                return; // exit to restart the game
        }
        window.clear(Color::Black);
        window.draw(topWall);
        window.draw(bottomWall);
        window.draw(leftWall);
        window.draw(rightWall);

        window.draw(paddle1);
        window.draw(paddle2);
        window.draw(ball);
        window.draw(retryText);
        window.display();
    }
}

void restartGame()
{
    readHighscore();
    score = 0;
    scoreText.setString("0");
    ball.setPosition({ 500.f, 300.f });
    ballSpeed = { 0.8f, 0.3f };
    deltaHeight = 0;
    finalY = 300.f;
    //paddle1.setPosition({ 20.f, 300.f - paddle1.getSize().y / 2.f }); //looks cleaner without these
    //paddle2.setPosition({ 940.f, 300.f - paddle2.getSize().y / 2.f }); //:P
}

int WinMain()
{
    window.setIcon(icon);
    settings.antiAliasingLevel = 8;
    window.setFramerateLimit(600);
    readHighscore();
    scoreText.setString(std::to_string(score));
    scoreText.setCharacterSize(48);
    FloatRect textBounds = scoreText.getLocalBounds();
    scoreText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f });
    scoreText.setPosition({ window.getSize().x / 2.f, window.getSize().y / 2.f });

    HS.setString((std::max(highscore, score) == 42 ? "The answer to everything:" : "HS:") + std::to_string(highscore));
    HS.setCharacterSize(16);
    FloatRect textBounds2 = HS.getLocalBounds();
    HS.setOrigin({ textBounds2.position.x + textBounds2.size.x / 2.f, textBounds2.position.y + textBounds2.size.y / 2.f });
    HS.setPosition({ window.getSize().x / 2.f, window.getSize().y / 2.f + textBounds.size.y });

    
    ball.setPosition({ 500, 300 });
    paddle2.setPosition({ 940.f, 300 });
    paddle1.setPosition({ -60.f, 300 });
    background.setSize(Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(Color::Black);

    topWall.setPosition({ 0.f, -90.f });
    bottomWall.setPosition({ 0.f, 590.f });
    leftWall.setPosition({ 0.f, 0.f });
    rightWall.setPosition({ 990.f, 0.f });

    paddle1.setFillColor(Color(38, 196, 133, 255));
    paddle2.setFillColor(Color(38, 196, 133, 255));
    ball.setFillColor(Color(102, 131, 151, 255));
    topWall.setFillColor(Color(38, 196, 133, 255));
    bottomWall.setFillColor(Color(38, 196, 133, 255));

    paddle1.setOutlineColor(Color(221, 242, 235, 255));
    paddle1.setOutlineThickness(5.f);
    paddle2.setOutlineColor(Color(221, 242, 235, 255));
    paddle2.setOutlineThickness(5.f);
    ball.setOutlineColor(Color(221, 242, 235, 255));
    ball.setOutlineThickness(2.5f);

    topWall.setOutlineColor(Color(221, 242, 235, 255));
    topWall.setOutlineThickness(4.f);
    bottomWall.setOutlineColor(Color(221, 242, 235, 255));
    bottomWall.setOutlineThickness(4.f);

    drawTitleScreen();
    winLoop();
}

void winLoop()
{
    float paddle1x = -70.f;
    float paddle1xf =  20.f;
    Vector2f originalViewPosition = view.getCenter();
    while (window.isOpen())
    {
        float dt = 1.0f / 60.0f;

        if (shakeElapsed < shakeDuration)
        {
            shakeElapsed += dt;
            float shakeX = getRand(-shakeIntensity, shakeIntensity);
            float shakeY = getRand(-shakeIntensity, shakeIntensity);
            view.setCenter(view.getCenter() + Vector2f(shakeX, shakeY));
        }
        else
        {
            shakeIntensity = 0.f;
            view.setCenter(originalViewPosition);
        }

        if (paddle1x < paddle1xf)
            paddle1x += (paddle1xf - paddle1x) * 0.01f;

        mousePos = Mouse::getPosition(window);
        paddle1.setPosition({ paddle1x, clamp(mousePos.y * 1.0f - 60.f, 15, 465) });

        Vector2f currentPos = paddle2.getPosition();
        float targetY = clamp(finalY - 60 + paddle2Noise, 15.f, 465.f);
        float smoothFactor = 0.02f; // smaller = slower movement
        float newY = currentPos.y + (targetY - currentPos.y) * smoothFactor;
        paddle2.setPosition({ 940.f, newY });

        ballMove();

        if (ball.getPosition().x < -20 || ball.getPosition().x > window.getSize().x)
        {
            gameOverScreen();
            restartGame();
        }

        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close(), saveHighScore(std::max(score, highscore));;
        }
        window.clear(Color(24, 24, 37, 255));
        window.setView(view);
        //window.draw(background);

        window.draw(topWall);
        window.draw(bottomWall);
        window.draw(leftWall);
        window.draw(rightWall);

        window.draw(scoreText);
        window.draw(HS);

        window.draw(paddle1);
        window.draw(paddle2);
        window.draw(ball);

        window.display();

    }
}