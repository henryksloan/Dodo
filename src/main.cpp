#include <iostream>
#include <optional>
#include <string>

#include "SDL.h"
#include "SDL_audio.h"
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

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  SDL_Window *window = SDL_CreateWindow("Dodo", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555,
                                           SDL_TEXTUREACCESS_TARGET, 160, 144);

  SDL_AudioSpec fmt, fmt_real;

  fmt.freq = kSampleRate;
  fmt.format = AUDIO_F32;
  fmt.channels = 1;  // TODO: Stereo
  fmt.samples = 0;   // Automatic
  fmt.callback = NULL;
  fmt.userdata = NULL;

  SDL_AudioDeviceID audio_dev_id;
  if ((audio_dev_id = SDL_OpenAudioDevice(nullptr, 0, &fmt, &fmt_real,
                                          SDL_AUDIO_ALLOW_ANY_CHANGE)) <= 0) {
    fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
    exit(-1);
  }

  /* Start playing, "unpause" */
  SDL_PauseAudioDevice(audio_dev_id, 0);

  SDL_Event event;
  while (true) {
    Uint64 start = SDL_GetPerformanceCounter();

    size_t n_steps = 0;
    while (!gameboy.step()) {
      n_steps++;
      if (n_steps % 10000 == 0) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) break;
      }
    }

    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) break;

    {
      // SDL_ClearQueuedAudio(audio_dev_id);
      auto audio_buff = gameboy.takeAudioBuffer();
      // std::cout << audio_buff.size() << ' '
      //           << SDL_GetQueuedAudioSize(audio_dev_id) << '\n';
      while (SDL_GetQueuedAudioSize(audio_dev_id) > (4096) * 4) {
      }
      SDL_QueueAudio(audio_dev_id, audio_buff.data(),
                     static_cast<Uint32>(audio_buff.size() * sizeof(float)));
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    uint8_t action_keys =
        static_cast<uint8_t>(!key_state[SDL_SCANCODE_RETURN] << 3) |
        static_cast<uint8_t>(!key_state[SDL_SCANCODE_RSHIFT] << 2) |
        static_cast<uint8_t>(!key_state[SDL_SCANCODE_Z] << 1) |
        static_cast<uint8_t>(!key_state[SDL_SCANCODE_X]);
    uint8_t dir_keys =
        static_cast<uint8_t>(!key_state[SDL_SCANCODE_DOWN] << 3) |
        static_cast<uint8_t>(!key_state[SDL_SCANCODE_UP] << 2) |
        static_cast<uint8_t>(!key_state[SDL_SCANCODE_LEFT] << 1) |
        static_cast<uint8_t>(!key_state[SDL_SCANCODE_RIGHT]);
    gameboy.setButtonsPressed(action_keys, dir_keys);

    auto &frame = gameboy.getFrame();
    SDL_UpdateTexture(texture, NULL, frame.data(), 160 * 2);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    Uint64 end = SDL_GetPerformanceCounter();

    float elapsed_ms = static_cast<float>(end - start) /
                       static_cast<float>(SDL_GetPerformanceFrequency()) *
                       1000.0f;

    // Cap to 60 FPS
    if (elapsed_ms < 16.666f) {
      SDL_Delay(static_cast<Uint32>(16.666f - elapsed_ms));
    }
  }

  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();

  return 0;
}
