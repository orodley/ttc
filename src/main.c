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
	for (int i = 0; i < count * (MAX_WORD_LENGTH + 1); i += (MAX_WORD_LENGTH + 1)) {
		if (strncmp(word_list + i, word, len + 1) == 0)
			return i;
	}

	return -1;
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

	char curr_input[MAX_WORD_LENGTH + 1];
	char remaining_chars[MAX_WORD_LENGTH + 1];
	size_t chars_entered = 0;

	memset(curr_input, 0, MAX_WORD_LENGTH + 1);
	memcpy(remaining_chars, word, MAX_WORD_LENGTH + 1);
	
	for (;;) {
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
			size_t end = MAX_WORD_LENGTH - chars_entered - 1;
			remaining_chars[removed_char_index] = remaining_chars[end];
			remaining_chars[end] = '\0';

			curr_input[chars_entered++] = vk;

			printf("remaining_chars = %s\n", remaining_chars);
			printf("curr_input = %s\n", curr_input);
		} else if (vk == SDLK_BACKSPACE) {
			if (chars_entered != 0) {
				chars_entered--;
				char removed = curr_input[chars_entered];
				remaining_chars[MAX_WORD_LENGTH - chars_entered - 1] = removed;
				curr_input[chars_entered] = '\0';

				printf("remaining_chars = %s\n", remaining_chars);
				printf("curr_input = %s\n", curr_input);
			}
		} else if (vk == SDLK_RETURN) {
			if (word_exists(result.anagrams, result.count, curr_input) != -1)
				printf("yep, %s is correct\n", curr_input);
			else
				printf("no, %s is wrong\n", curr_input);

			memcpy((remaining_chars + MAX_WORD_LENGTH) - chars_entered, curr_input, chars_entered);
			memset(curr_input, 0, chars_entered);
			chars_entered = 0;
			printf("remaining_chars = %s\n", remaining_chars);
			printf("curr_input = %s\n", curr_input);
		}
	}

	return 0;
}
