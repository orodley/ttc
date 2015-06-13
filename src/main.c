#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include "anagram.h"
#include "words.h"

void most_and_fewest(WordTree *tree)
{
	size_t max_count = 0;
	char *max_word;
	size_t min_count = (size_t)-1;
	char *min_word;
	for (size_t i = 0; words[i] != NULL; i++) {
		if (strlen(words[i]) != 6)
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

int main(int argc, char *argv[])
{
	WordTree *tree = word_list_to_tree(words);

	struct timeval t;
	gettimeofday(&t, NULL);
	srand(t.tv_usec * t.tv_sec);

	char *word;
	if (argc == 1)
		word = strdup(random_word(words, 6));
	else if (argc == 2)
		word = argv[1];
	else
		return 1;

	if (strlen(word) > 6)
		return 2;

	printf("word is %s\n", word);
	shuffle(word);
	printf("word is %s\n", word);

	AnagramsResult result = find_all_anagrams(tree, word);
	for (size_t i = 0; i < result.count; i++)
		printf("%s\n", result.anagrams + i * 7);

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 3;

	SDL_Window *window = SDL_CreateWindow(
			"ttc",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			640, 480,
			SDL_WINDOW_SHOWN);
	if (window == NULL)
		return 4;

	SDL_Surface *surface = SDL_GetWindowSurface(window);
	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
	SDL_UpdateWindowSurface(window);

	for (;;) {
		SDL_Event event;
		SDL_WaitEvent(&event);

		if (event.type != SDL_KEYDOWN)
			continue;

		SDL_KeyboardEvent kbe = event.key;
		SDL_Keycode vk = kbe.keysym.sym;
		if (vk == SDLK_ESCAPE)
			break;
	}

	return 0;
}
