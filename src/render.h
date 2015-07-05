#ifndef RENDER_H_
#define RENDER_H_

#include <stdbool.h>
#include <SDL2/SDL.h>

bool prerender_letters(SDL_Texture **textures, SDL_Renderer *renderer,
		char *font_name, int size);
void render_guessed_word(Game *game, char *word);
SDL_Texture *render_radial_gradient(SDL_Renderer *renderer,
		int width, int height, SDL_Color center_color, SDL_Color corner_color);
SDL_Texture *render_empty_words(Game *game, SDL_Color fill_color);
SDL_Texture *render_message_box(Game *game, char *text);
int **compute_layout(Game *game);
void render_game(Game *game);

#endif
