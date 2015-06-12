#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "anagram.h"
#include "words.h"

int main(int argc, char *argv[])
{
	WordTree *tree = word_list_to_tree(words);

	if (argc == 1) {
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

		return 0;
	} else if (argc != 2) {
		return 1;
	}
	char *word = argv[1];
	if (strlen(word) > 6)
		return 2;

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
	SDL_Delay(2000);

	return 0;
}
