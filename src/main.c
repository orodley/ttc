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
	Game game = {};

	game.all_words_array = words;
	game.all_words_tree = word_list_to_tree(words);

	struct timeval t;
	gettimeofday(&t, NULL);
	srand(t.tv_usec * t.tv_sec);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		return 3;
	if (TTF_Init() == -1)
		return 4;

	game.window_width = 720;
	game.window_height = 540;

	game.window = SDL_CreateWindow(
			"ttc",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			game.window_width, game.window_height,
			SDL_WINDOW_SHOWN);
	if (game.window == NULL)
		return 5;

	game.renderer = SDL_CreateRenderer(game.window, -1, 0);
	if (game.renderer == NULL)
		return 6;

	int font_size = 24;
	if (!prerender_letters(game.large_letter_textures, game.renderer, "font.ttf", font_size))
		return 7;
	if (!prerender_letters(game.small_letter_textures, game.renderer, "font.ttf", 18))
		return 7;
	game.font = TTF_OpenFont("font.ttf", font_size);
	if (game.font == NULL)
		return 7;

	SDL_Color center_color = {0, 0xDB, 0xEB, 0xFF};
	SDL_Color corner_color = {0, 0x9A, 0xEB, 0xFF};
	game.background = render_radial_gradient(game.renderer,
			game.window_width, game.window_height, center_color, corner_color);
	game.letter_circle = render_letter_circle(game.renderer, 30);

	new_level(&game);
	game.points = 0;
	
	for (;;) {
		render_game(&game);

		SDL_Event event;
		SDL_WaitEvent(&event);

		if (!handle_event(&game, &event))
			break;
	}

	return 0;
}
