#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "anagram.h"
#include "words.h"

void most_and_fewest(WordTree *tree)
{
	size_t max_count = 0;
	char *max_word;
	size_t min_count = (size_t)-1;
	char *min_word;
	for (size_t i = 0; words[i] != NULL; i++) {
		if (strlen(words[i]) != MAX_WORD_LENGTH)
			continue;

		AnagramsResult anagrams = find_all_anagrams(tree, words[i]);
		if (anagrams.count > max_count) {
			max_count = anagrams.count;
			max_word = words[i];
		} else if (anagrams.count < min_count) {
			min_count = anagrams.count;
			min_word = words[i];
		}
	}

	printf("Fewest anagrams: \"%s\": %zd\n", min_word, min_count);
	printf("Most anagrams: \"%s\": %zd\n", max_word, max_count);
}

char *random_word(char **word_list, unsigned int length)
{
	char *selected_word;
	int valid_words = 0;
	for (int i = 1; word_list[i] != NULL; i++) {
		char *word = word_list[i];
		if (strlen(word) == length) {
			valid_words++;
			float probability = (float)rand() / RAND_MAX;
			if (valid_words == 1 || probability < (1.0 / valid_words))
				selected_word = word;
		}
	}

	return selected_word;
}

void shuffle(char *word)
{
	size_t length = strlen(word);
	for (size_t i = 0; i < length; i++) {
		float proportion = (float)rand() / RAND_MAX;
		size_t swap_index = ((length - i) * proportion) + i;

		char tmp = word[i];
		word[i] = word[swap_index];
		word[swap_index] = tmp;
	}
}

int word_exists(char *word_list, int count, char *word)
{
	size_t len = strlen(word);
	for (int i = 0;
			i < count * (MAX_WORD_LENGTH + 1);
			i += (MAX_WORD_LENGTH + 1)) {
		if (strncmp(word_list + i, word, len + 1) == 0)
			return i;
	}

	return -1;
}

void render_letters(SDL_Renderer *renderer, SDL_Texture **letter_textures,
		char *letters, int x, int y, int step)
{
	SDL_Rect letter_pos = {x, y, 0, 0};
	for (int i = 0; letters[i] != '\0'; i++) {
		SDL_Texture *letter_texture = letter_textures[letters[i] - 'a'];
		SDL_QueryTexture(letter_texture, NULL, NULL,
				&letter_pos.w, &letter_pos.h);
		SDL_RenderCopy(renderer, letter_texture, NULL, &letter_pos);

		letter_pos.x += step;
	}
}

int main(int argc, char *argv[])
{
	WordTree *tree = word_list_to_tree(words);

	struct timeval t;
	gettimeofday(&t, NULL);
	srand(t.tv_usec * t.tv_sec);

	char *word;
	if (argc == 1)
		word = strdup(random_word(words, MAX_WORD_LENGTH));
	else if (argc == 2)
		word = argv[1];
	else
		return 1;

	if (strlen(word) > MAX_WORD_LENGTH)
		return 2;

	printf("word is %s\n", word);
	shuffle(word);
	printf("word is %s\n", word);

	AnagramsResult result = find_all_anagrams(tree, word);
	for (size_t i = 0; i < result.count; i++)
		printf("%s\n", result.anagrams + i * 7);


	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 3;
	if (TTF_Init() == -1)
		return 4;

	int width = 720;
	int height = 540;

	SDL_Window *window = SDL_CreateWindow(
			"ttc",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			width, height,
			SDL_WINDOW_SHOWN);
	if (window == NULL)
		return 5;

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	if (renderer == NULL)
		return 6;

	int font_size = 24;
	TTF_Font* font = TTF_OpenFont("font.ttf", font_size);
	if (font == NULL)
		return 7;

	SDL_Texture *letter_textures[26];
	for (int i = 0; i < 26; i++) {
		SDL_Color black = {0, 0, 0, 255};
		SDL_Surface* letter_surface =
			TTF_RenderGlyph_Blended(font, 'A' + i, black);
		SDL_Texture* letter_texture =
			SDL_CreateTextureFromSurface(renderer, letter_surface);
		SDL_SetTextureBlendMode(letter_texture, SDL_BLENDMODE_BLEND);

		letter_textures[i] = letter_texture;
	}

	char curr_input[MAX_WORD_LENGTH + 1];
	char remaining_chars[MAX_WORD_LENGTH + 1];
	size_t chars_entered = 0;

	memset(curr_input, 0, MAX_WORD_LENGTH + 1);
	memcpy(remaining_chars, word, MAX_WORD_LENGTH + 1);
	
	for (;;) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

		render_letters(renderer, letter_textures, curr_input,
				width / 2, height / 2, font_size * 1.5);
		render_letters(renderer, letter_textures, remaining_chars,
				width / 2, height / 2 + font_size * 2, font_size * 1.5);

        SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);


		SDL_Event event;
		SDL_WaitEvent(&event);

		if (event.type != SDL_KEYDOWN)
			continue;

		SDL_KeyboardEvent kbe = event.key;
		SDL_Keycode vk = kbe.keysym.sym;
		if (vk == SDLK_ESCAPE) {
			break;
		} else if ((vk >= 'a') && (vk <= 'z')) {
			int removed_char_index = -1;
			for (size_t i = 0; i < MAX_WORD_LENGTH - chars_entered; i++) {
				char c = remaining_chars[i];

				if (c == vk) {
					removed_char_index = i;
					break;
				}
			}

			if (removed_char_index == -1)
				continue;
			for (int i = removed_char_index; i < MAX_WORD_LENGTH; i++)
				remaining_chars[i] = remaining_chars[i + 1];

			curr_input[chars_entered++] = vk;

		} else if (vk == SDLK_BACKSPACE) {
			if (chars_entered != 0) {
				chars_entered--;
				char removed = curr_input[chars_entered];
				remaining_chars[MAX_WORD_LENGTH - chars_entered - 1] = removed;
				curr_input[chars_entered] = '\0';
			}
		} else if (vk == SDLK_RETURN) {
			if (word_exists(result.anagrams, result.count, curr_input) != -1)
				printf("yep, %s is correct\n", curr_input);
			else
				printf("no, %s is wrong\n", curr_input);

			memcpy((remaining_chars + MAX_WORD_LENGTH) - chars_entered,
					curr_input, chars_entered);
			memset(curr_input, 0, chars_entered);
			chars_entered = 0;
		}

		printf("remaining_chars = %s\n", remaining_chars);
		printf("curr_input = %s\n", curr_input);
	}

	return 0;
}
