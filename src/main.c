#define _XOPEN_SOURCE 500
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "anagram.h"
#include "words.h"

static TTF_Font *font;
static SDL_Texture *letter_textures[26];
static SDL_Texture *background;

void most_and_fewest(WordTree *tree)
{
	size_t max_count = 0;
	char *max_word;
	size_t min_count = (size_t)-1;
	char *min_word;
	for (size_t i = 0; words[i] != NULL; i++) {
		if (strlen(words[i]) != MAX_WORD_LENGTH)
			continue;

		WordList *anagrams = find_all_anagrams(tree, words[i]);
		if (anagrams->count > max_count) {
			max_count = anagrams->count;
			max_word = words[i];
		} else if (anagrams->count < min_count) {
			min_count = anagrams->count;
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

void separate_words(WordList **separated_words, WordList *words)
{
	int length_counts[MAX_WORD_LENGTH - 2] = { 0 };
	for (size_t i = 0; i < words->count; i++) {
		char *word = words->words + i * (MAX_WORD_LENGTH + 1);
		size_t length = strlen(word);
		assert((length >= 3) && (length <= MAX_WORD_LENGTH));
		length_counts[length - 3]++;
	}

	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		WordList *word_list = calloc(1, sizeof(*word_list) +
				(length_counts[i] * (i + 4)));
		separated_words[i] = word_list;
	}

	for (size_t i = 0; i < words->count; i++) {
		char *word = words->words + i * (MAX_WORD_LENGTH + 1);
		size_t length = strlen(word);
		WordList *word_list = separated_words[length - 3];
		memcpy(word_list->words + ((length + 1) * word_list->count), word, length);
		word_list->count++;
	}

	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		WordList *word_list = separated_words[i];

		qsort(word_list->words, word_list->count, i + 4,
				(int (*)(const void *, const void *))strcmp);
	}
}

void render_letters(SDL_Renderer *renderer, char *letters, int x, int y, int step)
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

int distance(int x1, int y1, int x2, int y2)
{
	int xdiff = x1 - x2;
	int ydiff = y1 - y2;
	return (int)sqrtf(xdiff * xdiff + ydiff * ydiff);
}

SDL_Color color_lerp(SDL_Color c1, float proportion, SDL_Color c2)
{
	int r_diff = c2.r - c1.r;
	int g_diff = c2.g - c1.g;
	int b_diff = c2.b - c1.b;
	int a_diff = c2.a - c1.a;

	int r = (int)(r_diff * proportion) + c1.r;
	int g = (int)(g_diff * proportion) + c1.g;
	int b = (int)(b_diff * proportion) + c1.b;
	int a = (int)(a_diff * proportion) + c1.a;

	SDL_Color result = {r, g, b, a};
	return result;
}

SDL_Texture *render_radial_gradient(SDL_Renderer *renderer,
		int width, int height, SDL_Color center_color, SDL_Color corner_color)
{
	SDL_Texture *t = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_UNKNOWN,
			SDL_TEXTUREACCESS_TARGET, width, height);
	SDL_Texture *quarter = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_UNKNOWN,
			SDL_TEXTUREACCESS_TARGET, width / 2, height / 2);
	SDL_Texture *original_render_target = SDL_GetRenderTarget(renderer);
	SDL_SetRenderTarget(renderer, quarter);

	int max_dist = distance(0, 0, width / 2, height / 2);
	for (int x = 0; x < width / 2; x++) {
		for (int y = 0; y < height / 2; y++) {
			int dist = distance(width / 2, height / 2, x, y);
			float proportion = (float)dist / max_dist;

			SDL_Color color = color_lerp(center_color, proportion, corner_color);
			SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
			SDL_RenderDrawPoint(renderer, x, y);
		}
	}

	SDL_SetRenderTarget(renderer, t);
	SDL_Rect dest = {0, 0, width / 2, height / 2};
	SDL_RendererFlip flip = 0;
	SDL_RenderCopyEx(renderer, quarter, NULL, &dest, 0, NULL, flip);
	dest.x += width / 2;
	flip = SDL_FLIP_HORIZONTAL;
	SDL_RenderCopyEx(renderer, quarter, NULL, &dest, 0, NULL, flip);
	dest.y += height / 2;
	flip |= SDL_FLIP_VERTICAL;
	SDL_RenderCopyEx(renderer, quarter, NULL, &dest, 0, NULL, flip);
	dest.x -= width / 2;
	flip = SDL_FLIP_VERTICAL;
	SDL_RenderCopyEx(renderer, quarter, NULL, &dest, 0, NULL, flip);

	SDL_DestroyTexture(quarter);

	SDL_SetRenderTarget(renderer, original_render_target);

	return t;
}

int main(void)
{
	WordTree *tree = word_list_to_tree(words);

	struct timeval t;
	gettimeofday(&t, NULL);
	srand(t.tv_usec * t.tv_sec);

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
	font = TTF_OpenFont("font.ttf", font_size);
	if (font == NULL)
		return 7;

	for (int i = 0; i < 26; i++) {
		SDL_Color black = {0, 0, 0, 255};
		SDL_Surface* letter_surface =
			TTF_RenderGlyph_Blended(font, 'A' + i, black);
		SDL_Texture* letter_texture =
			SDL_CreateTextureFromSurface(renderer, letter_surface);
		SDL_SetTextureBlendMode(letter_texture, SDL_BLENDMODE_BLEND);

		letter_textures[i] = letter_texture;
	}

	SDL_Color center_color = {0, 239, 235, 255};
	SDL_Color corner_color = {0, 124, 235, 255};
	background = render_radial_gradient(renderer, width, height,
			center_color, corner_color);

	char *word = strdup(random_word(words, MAX_WORD_LENGTH));

	printf("word is %s\n", word);
	shuffle(word);
	printf("word is %s\n", word);

	WordList *anagrams = find_all_anagrams(tree, word);
	printf("%d anagrams:\n", anagrams->count);

	WordList *words[MAX_WORD_LENGTH - 2];
	separate_words(words, anagrams);
	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		printf("%d: %d words\n", i + 3, words[i]->count);

		for (size_t j = 0; j < words[i]->count; j++)
			printf("\t%s\n", words[i]->words + j * (i + 4));
	}

	char curr_input[MAX_WORD_LENGTH + 1];
	char remaining_chars[MAX_WORD_LENGTH + 1];
	size_t chars_entered = 0;

	memset(curr_input, 0, MAX_WORD_LENGTH + 1);
	memcpy(remaining_chars, word, MAX_WORD_LENGTH + 1);
	
	for (;;) {
		SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, background, NULL, NULL);

		render_letters(renderer, curr_input,
				width / 2, height / 2, font_size * 1.5);
		render_letters(renderer, remaining_chars,
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
			if (word_exists(anagrams->words, anagrams->count, curr_input) != -1)
				printf("yep, %s is correct\n", curr_input);
			else
				printf("no, %s is wrong\n", curr_input);

			memcpy((remaining_chars + MAX_WORD_LENGTH) - chars_entered,
					curr_input, chars_entered);
			memset(curr_input, 0, chars_entered);
			chars_entered = 0;
		}
	}

	return 0;
}
