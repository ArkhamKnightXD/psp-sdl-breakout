#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <vector>

PSP_MODULE_INFO("SDL-BREAKOUT", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
 
int exitCallback(int arg1, int arg2, void* common) {

    sceKernelExitGame();
    return 0;
}

int callbackThread(SceSize args, void* argp) {

    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL); 
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();

    return 0;
}

int setupCallbacks(void) {

    int thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);

    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }

    return thid;
}

enum {
  SCREEN_WIDTH  = 480,
  SCREEN_HEIGHT = 272
};

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_GameController* controller = NULL;

SDL_Rect player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 16, 36, 8};
SDL_Rect ball = {SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 8, 8, 8};

int playerSpeed = 400;
int ballVelocityX = 225;
int ballVelocityY = 225;

bool isAutoPlayMode = false;

typedef struct {

    SDL_Rect bounds;
    bool isDestroyed;  
} Brick;

std::vector<Brick> createBricks() {

    std::vector<Brick> bricks;

    int positionX;
    int positionY = 20;

    for (int i = 0; i < 8; i++) {

        positionX = 2;

        for (int j = 0; j < 14; j++) {
            
            Brick actualBrick = {{positionX, positionY, 32, 8}, false};

            bricks.push_back(actualBrick);
            positionX += 34;
        }

        positionY += 10;
    }

    return bricks;
}

std::vector<Brick> bricks = createBricks();

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

bool hasCollision(SDL_Rect bounds, SDL_Rect ball) {

    return bounds.x < ball.x + ball.w && bounds.x + bounds.w > ball.x &&
           bounds.y < ball.y + ball.h && bounds.y + bounds.h > ball.y;
}
 
void update(float deltaTime) {

    SDL_GameControllerUpdate();

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START)) {
        isAutoPlayMode = !isAutoPlayMode;
    }

    if (isAutoPlayMode && ball.x < SCREEN_WIDTH - player.w) {
        player.x = ball.x;
    }

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) && player.x > 0) {
        player.x -= playerSpeed * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) && player.x < SCREEN_WIDTH - player.w) {
        player.x += playerSpeed * deltaTime;
    }

    if (ball.y > SCREEN_HEIGHT + ball.h) {

        ball.x = SCREEN_WIDTH / 2 - ball.w;
        ball.y = SCREEN_HEIGHT / 2 - ball.h;

        ballVelocityX *= -1;
    }
    
    if (ball.x < 0 || ball.x > SCREEN_WIDTH - ball.w) {
        ballVelocityX *= -1;
    }

    if (hasCollision(player, ball) || ball.y < 0) {
        ballVelocityY *= -1;
    }

    for (Brick &brick : bricks) {

        if (!brick.isDestroyed && hasCollision(brick.bounds, ball)) {

            ballVelocityY *= -1;
            brick.isDestroyed = true;
        }
    }

    ball.x += ballVelocityX * deltaTime;
    ball.y += ballVelocityY * deltaTime;
}

void render() {
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);

    for (Brick brick : bricks) {

        if (!brick.isDestroyed)
            SDL_RenderFillRect(renderer, &brick.bounds);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderFillRect(renderer, &player);
    SDL_RenderFillRect(renderer, &ball);

    SDL_RenderPresent(renderer);
}

int main() {

    setupCallbacks();
    SDL_SetMainReady();
    pspDebugScreenInit();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        return -1;
    }

    if ((window = SDL_CreateWindow("Breakout", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0)) == NULL) {
        return -1;
    }

    if ((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)) == NULL) {
        return -1;
    }

    if (SDL_NumJoysticks() < 1) {
        pspDebugScreenPrintf("no game controller");
        return -1;
    } 
    
    else {
        
        controller = SDL_GameControllerOpen(0);
        if (controller == NULL) {
            pspDebugScreenPrintf("unable to open game controller");
            return -1;
        }
    }

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime;
    float deltaTime;

    while (true) {
        currentFrameTime = SDL_GetTicks();
        deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;
        previousFrameTime = currentFrameTime;

        handleEvents();
        update(deltaTime);
        render();
    }

    quitGame();
}