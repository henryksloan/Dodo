#include <iostream>
#include <optional>
#include <string>

#include "SDL.h"
#include "gameboy.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <GB ROM file>" << std::endl;
    return 1;
  }

  Gameboy gameboy;

  std::optional<std::string> error_msg = gameboy.loadCartridge(argv[1]);
  if (error_msg) {
    std::cerr << *error_msg << std::endl;
    return 1;
  }

  // for (int i = 0; i < 8000000; i++) {
  //   gameboy.step();
  // }

  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window = SDL_CreateWindow("Dodo", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555,
                                           SDL_TEXTUREACCESS_TARGET, 160, 144);

  SDL_Event event;
  while (true) {
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) break;

    Uint64 start = SDL_GetPerformanceCounter();

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    uint8_t action_keys = (!key_state[SDL_SCANCODE_RETURN] << 3) |
                          (!key_state[SDL_SCANCODE_RSHIFT] << 2) |
                          (!key_state[SDL_SCANCODE_Z] << 1) |
                          (!key_state[SDL_SCANCODE_X]);
    uint8_t dir_keys = (!key_state[SDL_SCANCODE_DOWN] << 3) |
                       (!key_state[SDL_SCANCODE_UP] << 2) |
                       (!key_state[SDL_SCANCODE_LEFT] << 1) |
                       (!key_state[SDL_SCANCODE_RIGHT]);
    gameboy.setButtonsPressed(action_keys, dir_keys);

    for (int i = 0; i < 10000; i++) {
      gameboy.step();
    }
    auto frame = gameboy.frameTest();
    SDL_UpdateTexture(texture, nullptr, frame.data(), 160 * 2);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    Uint64 end = SDL_GetPerformanceCounter();

    float elapsed_ms =
        (end - start) / (float)SDL_GetPerformanceFrequency() * 1000.0f;

    // Cap to 60 FPS
    if (elapsed_ms < 16.666f) {
      SDL_Delay(floor(16.666f - elapsed_ms));
    }
  }

  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();

  return 0;
}
