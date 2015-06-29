#ifndef GAME_H_
#define GAME_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "anagram.h"

typedef struct GameState
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *guessed_words_texture;
	SDL_Texture *large_letter_textures[26];
	SDL_Texture *small_letter_textures[26];
	SDL_Texture *background;
	TTF_Font *font;

	SDL_TimerID second_timer;

	int window_height;
	int window_width;

	WordList *anagrams;
	WordList **anagrams_by_length;
	int **column_sizes;

	WordList *guessed_words;

	char curr_input[MAX_WORD_LENGTH + 1];
	size_t chars_entered;
	char remaining_chars[MAX_WORD_LENGTH + 1];

	int time_left;
	int points;
} GameState;

void new_level(GameState *game_state, WordTree *tree, char **words);
bool handle_event(GameState *game_state, SDL_Event *event);

#endif
