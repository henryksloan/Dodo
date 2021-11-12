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

  for (int i = 0; i < 8000000; i++) {
    gameboy.step();
  }

  auto x = gameboy.frameTest();

  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window = SDL_CreateWindow("SDL2Test", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  // SDL_Surface *screen = SDL_GetWindowSurface(window);

  // SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_BGR555);
  // screen = SDL_ConvertSurface(screen, format, 0);
  // SDL_FreeFormat(format);

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555,
                                           SDL_TEXTUREACCESS_TARGET, 160, 144);

  SDL_UpdateTexture(texture, nullptr, x.data(), 160 * 2);
  // SDL_Rect r;
  // r.w = 100;
  // r.h = 50;
  // SDL_SetRenderTarget(renderer, texture);
  // SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
  // SDL_RenderClear(renderer);
  // SDL_RenderDrawRect(renderer, &r);
  // SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0x00);
  // SDL_RenderFillRect(renderer, &r);
  // SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);

  // for (int i = 0; i < 100; i++) {
  //   for (int j = 0; j < 100; j++) {
  //     Uint16 *const target_pixel =
  //         (Uint16 *)((Uint8 *)screen->pixels + i * screen->pitch +
  //                    j * screen->format->BytesPerPixel);
  //     *target_pixel = 0x00FF00;
  //   }
  // }

  // SDL_UpdateWindowSurface(window);

  SDL_Delay(3000);

  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();

  return 0;
}
