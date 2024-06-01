#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <pspdebug.h>
#include <pspkernel.h>

PSP_MODULE_INFO("SDL-Starter", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
 
int exit_callback(int arg1, int arg2, void* common)
{
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void* argp)
{
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();

    return 0;
}

int SetupCallbacks(void)
{
    int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0)
    {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}

// Screen dimension constants
enum {
  SCREEN_WIDTH  = 480,
  SCREEN_HEIGHT = 272
};

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_GameController* controller = NULL;
SDL_Texture* sprite;

bool shouldRenderSprite = false;

bool running = true;

SDL_Rect rectangle = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 16, 16};

const int SPEED = 600; 
const int FRAME_RATE = 60; 

// Exit the game and clean up
void quitGame() {

    SDL_GameControllerClose(controller);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// Function to handle events
void handleEvents() {

    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        if (event.type == SDL_QUIT) {
            
            quitGame();
            exit(0);
        }
    }
}

void capFrameRate(Uint32 frameStartTime) {

    Uint32 frameTime = SDL_GetTicks() - frameStartTime;
    
    if (frameTime < 1000 / FRAME_RATE) {
        SDL_Delay(1000 / FRAME_RATE - frameTime);
    }
}

SDL_Texture* LoadSprite(const char* file, SDL_Renderer* renderer)
{
    SDL_Texture* texture = IMG_LoadTexture(renderer, file);
    if (texture == nullptr)
    {
        pspDebugScreenPrintf("Failed to create texture: %s\n", SDL_GetError());
        sceKernelDelayThread(3 * 1000 * 1000);
        return nullptr;
    }
    pspDebugScreenPrintf("Loaded image: %s\n", file);
    sceKernelDelayThread(3 * 1000 * 1000);
    return texture;
}

void RenderSprite(SDL_Texture* sprite, SDL_Renderer* renderer, int x, int y)
{
    SDL_Rect dest;
    dest.x = x;
    dest.y = y;
    SDL_QueryTexture(sprite, NULL, NULL, &dest.w, &dest.h);
    SDL_RenderCopy(renderer, sprite, NULL, &dest);
}

void update(float deltaTime) {

    SDL_GameControllerUpdate();

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A)) {
        shouldRenderSprite = !shouldRenderSprite;
    }

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) && rectangle.y > 0) {
        rectangle.y -= SPEED * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) && rectangle.y < SCREEN_HEIGHT - rectangle.h) {
        rectangle.y += SPEED * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) && rectangle.x > 0) {
        rectangle.x -= SPEED * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) && rectangle.x < SCREEN_WIDTH - rectangle.w) {
        rectangle.x += SPEED * deltaTime;
    }
}

void render() {

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    if(shouldRenderSprite) 
    {
        RenderSprite(sprite, renderer, 100, 100);
    }

    SDL_RenderFillRect(renderer, &rectangle);

    SDL_RenderPresent(renderer);  
}

int main()
{
    SetupCallbacks();
    SDL_SetMainReady();
    pspDebugScreenInit();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        return -1;
    }

    if ((window = SDL_CreateWindow("RedRectangle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0)) == NULL) {
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

    sprite = LoadSprite("sprites/sprite.png", renderer);

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime;
    float deltaTime;

    while (running)
    {
        currentFrameTime = SDL_GetTicks();
        deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;
        previousFrameTime = currentFrameTime;

        handleEvents();
        update(deltaTime);
        render();
        // capFrameRate(currentFrameTime);
    }

    quitGame();
}