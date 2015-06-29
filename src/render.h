#ifndef RENDER_H_
#define RENDER_H_

#include <stdbool.h>
#include <SDL2/SDL.h>

bool prerender_letters(SDL_Texture **textures, SDL_Renderer *renderer,
		char *font_name, int size);
void render_guessed_word(GameState *game_state, char *word);
SDL_Texture *render_radial_gradient(SDL_Renderer *renderer,
		int width, int height, SDL_Color center_color, SDL_Color corner_color);
SDL_Texture *render_empty_words(SDL_Renderer *renderer, WordList **word_lists,
		int **col_sizes, int width, int height);
int **compute_layout(GameState *game_state);
void render_game(GameState *game_state);

#endif