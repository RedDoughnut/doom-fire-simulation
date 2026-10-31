#include <SDL3/SDL_events.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <bits/stdc++.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdint>

uint32_t lerp(uint32_t A, uint32_t B, double t){
    uint32_t aA = (A >> 24) & 0xFF;
    uint32_t aB = (A >> 16) & 0xFF;
    uint32_t aG = (A >> 8) & 0xFF;
    uint32_t aR =  A & 0xFF;

    uint32_t bA = (B >> 24) & 0xFF;
    uint32_t bB = (B >> 16) & 0xFF;
    uint32_t bG = (B >> 8) & 0xFF;
    uint32_t bR =  B & 0xFF;

    uint8_t outA = static_cast<uint8_t>(aA + t * (bA - aA));
    uint8_t outR = static_cast<uint8_t>(aR + t * (bR - aR));
    uint8_t outG = static_cast<uint8_t>(aG + t * (bG - aG));
    uint8_t outB = static_cast<uint8_t>(aB + t * (bB - aB));

    return (outA << 24) | (outB << 16) | (outG << 8) | outR;
}
int main() {
    const Uint64 TARGET_FPS = 30;
    const Uint64 NS_PER_FRAME = 1000000000 / TARGET_FPS;
    const int MAX_COLOR = 37;
    const int MAX_DROP = 1;
    const int MAX_DRIFT = 3;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
        return -1;
    }
    const char* title = "Fire";
    int width = 800;
    int height = 600;
    SDL_Window* window = SDL_CreateWindow(title, width, height, SDL_WINDOW_BORDERLESS);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    std::vector<std::vector<uint8_t>> values(height, std::vector<uint8_t>(width));

    for(int i=0;i<width;i++){
        values[height-1][i] = MAX_COLOR - 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);


    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, MAX_DROP);
    std::uniform_int_distribution<int> distDrift(0, MAX_DRIFT);

    uint32_t P_1, P_2, P_3, P_4;
    int o1, o2;

    o1 = MAX_COLOR / 3;
    o2 = 2 * MAX_COLOR / 3;

    std::vector<uint32_t> palette1(MAX_COLOR);
    P_1 = 0xFF<<24;
    P_2 = (0xFF<<24) | (7<<16) | (31<<8) | 231;
    P_3 = (0xFF<<24) | (7<<16) | (223<<8) | 255;
    P_4 = 0xFFFFFFFF;

    for(int i=0;i<MAX_COLOR;i++){
        if(i<o1){
            double t = static_cast<double>(i) / o1;
            palette1[i] = lerp(P_1, P_2, t);
        } else if(i<o2){
            double t = static_cast<double>(i - o1) / (o2 - o1);
            palette1[i] = lerp(P_2, P_3, t);
        }
        else{
            double t = static_cast<double>(i - o2) / (MAX_COLOR - o2);
            palette1[i] = lerp(P_3, P_4, t);
        }
    }
    std::vector<uint32_t> palette2(MAX_COLOR);
    // ABGR
    P_1 = 0xFF<<24;
    P_2 = (0xFF<<24) | (231<<16) | (31<<8) | 7;
    P_3 = (0xFF<<24) | (249<<16) | (247<<8) | 183;
    P_4 = 0xFFFFFFFF;

    for(int i=0;i<MAX_COLOR;i++){
        if(i<o1){
            double t = static_cast<double>(i) / o1;
            palette2[i] = lerp(P_1, P_2, t);
        } else if(i<o2){
            double t = static_cast<double>(i - o1) / (o2 - o1);
            palette2[i] = lerp(P_2, P_3, t);
        } else{
            double t = static_cast<double>(i - o2) / (MAX_COLOR - o2);
            palette2[i] = lerp(P_3, P_4, t);
        }
    }

    std::vector<std::vector<uint32_t>> palettes = {palette1, palette2};
    int curPalette = 0;

    bool running = true;
    SDL_Event event;

    while (running) {
        Uint64 frame_start_perf = SDL_GetPerformanceCounter();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN){
                switch(event.key.key){
                    case SDLK_1:
                        curPalette = 0; break;
                    case SDLK_2:
                        curPalette = 1; break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        void* mPixels;
        int mPitch;

        SDL_LockTexture(texture, NULL, &mPixels, &mPitch);

        Uint32* pixels = static_cast<Uint32*>(mPixels);
        int rowStride = mPitch / sizeof(Uint32);

        for (int i = 0; i < height-1; i++) {
            for (int j = 0; j < width; j++) {
                int decay = dist(gen);
                int drift = distDrift(gen);
                int nx = (j + drift) % width;
                int next_val = values[i + 1][nx] - decay;
                values[i][j] = std::max(next_val, 0);
                pixels[i * rowStride + j] = palettes[curPalette][values[i][j]];
            }
        }

        SDL_UnlockTexture(texture);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        Uint64 frame_end_perf = SDL_GetPerformanceCounter();
        Uint64 elapsed_perf = frame_end_perf - frame_start_perf;
        Uint64 elapsed_ns = (elapsed_perf * 1000000000) / SDL_GetPerformanceFrequency();

        if (elapsed_ns < NS_PER_FRAME) {
            SDL_DelayNS(NS_PER_FRAME - elapsed_ns);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
