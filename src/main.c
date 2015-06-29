#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "anagram.h"
#include "game.h"
#include "render.h"
#include "words.h"

int main(void)
{
	WordTree *tree = word_list_to_tree(words);

	struct timeval t;
	gettimeofday(&t, NULL);
	srand(t.tv_usec * t.tv_sec);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		return 3;
	if (TTF_Init() == -1)
		return 4;

	GameState game_state;

	game_state.window_width = 720;
	game_state.window_height = 540;

	game_state.window = SDL_CreateWindow(
			"ttc",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			game_state.window_width, game_state.window_height,
			SDL_WINDOW_SHOWN);
	if (game_state.window == NULL)
		return 5;

	game_state.renderer = SDL_CreateRenderer(game_state.window, -1, 0);
	if (game_state.renderer == NULL)
		return 6;

	int font_size = 24;
	if (!prerender_letters(game_state.large_letter_textures, game_state.renderer, "font.ttf", font_size))
		return 7;
	if (!prerender_letters(game_state.small_letter_textures, game_state.renderer, "font.ttf", 18))
		return 7;
	game_state.font = TTF_OpenFont("font.ttf", font_size);
	if (game_state.font == NULL)
		return 7;

	SDL_Color center_color = {0, 239, 235, 255};
	SDL_Color corner_color = {0, 124, 235, 255};
	game_state.background = render_radial_gradient(game_state.renderer, game_state.window_width, game_state.window_height,
			center_color, corner_color);

	new_level(&game_state, tree, words);
	
	for (;;) {
		render_game(&game_state);

		SDL_Event event;
		SDL_WaitEvent(&event);

		if (!handle_event(&game_state, &event))
			break;
	}

	return 0;
}
