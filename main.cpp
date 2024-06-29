#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>

const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 272;

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_GameController* controller = nullptr;

TTF_Font *fontSquare = nullptr;

SDL_Texture *scoreTexture = nullptr;
SDL_Rect scoreBounds;

SDL_Texture *liveTexture = nullptr;
SDL_Rect liveBounds;

SDL_Color fontColor = {255, 255, 255};

Mix_Chunk *collisionSound = nullptr;
Mix_Chunk *collisionWithPlayerSound = nullptr;

SDL_Rect player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 16, 36, 8};

int playerScore;
int playerLives = 2;

SDL_Rect ball = {SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 8, 8, 8};

int playerSpeed = 400;
int ballVelocityX = 225;
int ballVelocityY = 225;

bool isAutoPlayMode = false;

typedef struct
{
    SDL_Rect bounds;
    bool isDestroyed;
    int points;
} Brick;

std::vector<Brick> createBricks()
{
    std::vector<Brick> bricks;

    int brickPoints = 8;
    int positionX;
    int positionY = 20;

    for (int row = 0; row < 8; row++)
    {
        positionX = 0;

        for (int column = 0; column < 14; column++)
        {
            Brick actualBrick = {{positionX, positionY, 32, 8}, false, brickPoints};

            bricks.push_back(actualBrick);
            positionX += 34;
        }

        brickPoints--;
        positionY += 10;
    }

    return bricks;
}

std::vector<Brick> bricks = createBricks();

Mix_Chunk *loadSound(const char *p_filePath)
{
    Mix_Chunk *sound = nullptr;

    sound = Mix_LoadWAV(p_filePath);
    if (sound == nullptr)
    {
        printf("Failed to load scratch sound effect! SDL_mixer Error: %s\n", Mix_GetError());
    }

    return sound;
}

void quitGame() {

    SDL_GameControllerClose(controller);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void handleEvents() {

    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        if (event.type == SDL_QUIT) {
            
            quitGame();
            exit(0);
        }
    }
}

void updateTextureText(SDL_Texture *&texture, const char *text) {

    if (fontSquare == nullptr) {
        printf("TTF_OpenFont fontSquare: %s\n", TTF_GetError());
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(fontSquare, text, fontColor);
    if (surface == nullptr) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create text surface! SDL Error: %s\n", SDL_GetError());
        exit(3);
    }

    SDL_DestroyTexture(texture);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture from surface! SDL Error: %s\n", SDL_GetError());
    }

    SDL_FreeSurface(surface);
}
 
void update(float deltaTime) {

    SDL_GameControllerUpdate();

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))
    {
        isAutoPlayMode = !isAutoPlayMode;
    }

    if (isAutoPlayMode && ball.x < SCREEN_WIDTH - player.w)
    {
        player.x = ball.x;
    }

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) && player.x > 0)
    {
        player.x -= playerSpeed * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) && player.x < SCREEN_WIDTH - player.w)
    {
        player.x += playerSpeed * deltaTime;
    }

    if (ball.y > SCREEN_HEIGHT + ball.h)
    {
        ball.x = SCREEN_WIDTH / 2 - ball.w;
        ball.y = SCREEN_HEIGHT / 2 - ball.h;

        ballVelocityX *= -1;

        if (playerLives > 0)
        {
            playerLives--;

            std::string livesString = std::to_string(playerLives);

            std::string completeString = "lives: " + livesString;

            char const *livesChar = completeString.c_str();

            updateTextureText(liveTexture, livesChar);
        }
    }

    if (ball.x < 0 || ball.x > SCREEN_WIDTH - ball.w)
    {
        ballVelocityX *= -1;
        Mix_PlayChannel(-1, collisionSound, 0);
    }

    if (SDL_HasIntersection(&player, &ball) || ball.y < 0)
    {
        ballVelocityY *= -1;
        Mix_PlayChannel(-1, collisionWithPlayerSound, 0);
    }

    for (Brick &brick : bricks)
    {
        if (!brick.isDestroyed && SDL_HasIntersection(&brick.bounds, &ball))
        {
            ballVelocityY *= -1;
            brick.isDestroyed = true;

            playerScore += brick.points;

            std::string scoreString = std::to_string(playerScore);

            std::string finalScoreString = "score: " + scoreString;

            char const *score = finalScoreString.c_str();

            updateTextureText(scoreTexture, score);

            Mix_PlayChannel(-1, collisionSound, 0);

            break;
        }
    }

    ball.x += ballVelocityX * deltaTime;
    ball.y += ballVelocityY * deltaTime;
}

void render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_QueryTexture(scoreTexture, NULL, NULL, &scoreBounds.w, &scoreBounds.h);
    scoreBounds.x = 100;
    scoreBounds.y = scoreBounds.h / 2 - 5;
    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreBounds);

    SDL_QueryTexture(liveTexture, NULL, NULL, &liveBounds.w, &liveBounds.h);
    liveBounds.x = 300;
    liveBounds.y = liveBounds.h / 2 - 5;
    SDL_RenderCopy(renderer, liveTexture, NULL, &liveBounds);

    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);

    for (Brick brick : bricks)
    {
        if (!brick.isDestroyed)
            SDL_RenderFillRect(renderer, &brick.bounds);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderFillRect(renderer, &player);
    SDL_RenderFillRect(renderer, &ball);

    SDL_RenderPresent(renderer);
}

int main(int argc, char *args[]) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        return -1;
    }

    if ((window = SDL_CreateWindow("breakout", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN)) == NULL) {
        return -1;
    }

    if ((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)) == NULL) {
        return -1;
    }

    if (SDL_NumJoysticks() < 1) {
        printf("No game controllers connected!\n");
        return -1;
    } 
    else {

        controller = SDL_GameControllerOpen(0);
        if (controller == nullptr) {

            printf("Unable to open game controller! SDL Error: %s\n", SDL_GetError());
            return -1;
        }
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    }

    if (TTF_Init() == -1)
    {
        return 1;
    }

    fontSquare = TTF_OpenFont("square_sans_serif_7.ttf", 16);

    updateTextureText(scoreTexture, "Score: 0");
    updateTextureText(liveTexture, "Lives: 2");

    collisionSound = loadSound("magic.wav");
    collisionWithPlayerSound = loadSound("drop.wav");

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime = previousFrameTime;
    float deltaTime = 0.0f;

    while (true)
    {
        currentFrameTime = SDL_GetTicks();
        deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;
        previousFrameTime = currentFrameTime;

        handleEvents();
        update(deltaTime);
        render();
    }

    return 0;
}