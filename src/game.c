#define _XOPEN_SOURCE 500
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "anagram.h"
#include "game.h"
#include "render.h"

#define SECOND_ELAPSED 0

static uint32_t push_time_event(uint32_t interval, void *param)
{
	(void)param;

    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_USEREVENT;
    userevent.code = SECOND_ELAPSED;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
    return(interval);
}

static char *random_word(char **word_list, unsigned int length)
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

static void sort_words(WordList **words_by_length, const WordList *unsorted_words)
{
	int length_counts[MAX_WORD_LENGTH - 2] = { 0 };
	for (size_t i = 0; i < unsorted_words->count; i++) {
		const char *word = GET_WORD(unsorted_words, i);
		size_t length = strlen(word);
		assert((length >= 3) && (length <= MAX_WORD_LENGTH));
		length_counts[length - 3]++;
	}

	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		WordList *word_list = make_word_list(length_counts[i], i + 4);
		word_list->count = 0;
		words_by_length[i] = word_list;
	}

	for (size_t i = 0; i < unsorted_words->count; i++) {
		const char *word = GET_WORD(unsorted_words, i);
		size_t length = strlen(word);
		WordList *word_list = words_by_length[length - 3];
		memcpy(GET_WORD(word_list, word_list->count), word, length);
		word_list->count++;
	}

	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		WordList *word_list = words_by_length[i];

		qsort(word_list->words, word_list->count, word_list->elem_size,
				(int (*)(const void *, const void *))strcmp);
	}
}


void new_level(GameState *game_state, WordTree *tree, char **words)
{
	char *word = strdup(random_word(words, MAX_WORD_LENGTH));

	printf("word is %s\n", word);
	shuffle(word);
	printf("word is %s\n", word);

	game_state->anagrams = find_all_anagrams(tree, word);
	printf("%zu anagrams:\n", game_state->anagrams->count);

	WordList **anagrams_by_length = malloc(sizeof(*anagrams_by_length) *
			(MAX_WORD_LENGTH - 2));
	sort_words(anagrams_by_length, game_state->anagrams);
	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		printf("%zu: %zu words\n", i + 3, anagrams_by_length[i]->count);

		for (size_t j = 0; j < anagrams_by_length[i]->count; j++)
			printf("\t%s\n", GET_WORD(anagrams_by_length[i], j));
	}

	game_state->anagrams_by_length = anagrams_by_length;

	game_state->column_sizes = compute_layout(game_state);

	game_state->guessed_words_texture = render_empty_words(game_state->renderer, game_state->anagrams_by_length,
			game_state->column_sizes, game_state->window_width, game_state->window_height);

	game_state->guessed_words =
		make_word_list(game_state->anagrams->count, MAX_WORD_LENGTH + 1);
	game_state->guessed_words->count = 0;

	game_state->chars_entered = 0;

	memset(game_state->curr_input, 0, MAX_WORD_LENGTH + 1);
	memcpy(game_state->remaining_chars, word, MAX_WORD_LENGTH + 1);

	game_state->second_timer = SDL_AddTimer(1000, push_time_event, NULL);
	game_state->time_left = 180;
	game_state->points = 0;
}

bool handle_event(GameState *game_state, SDL_Event *event)
{
	if (event->type == SDL_USEREVENT) {
		switch (event->user.code) {
		case SECOND_ELAPSED:
			game_state->time_left--;

			if (game_state->time_left == 0) {
				puts("Time's up!");
				return 0;
			}

			break;
		}

		return true;
	} else if (event->type != SDL_KEYDOWN) {
		return true;
	}

	SDL_KeyboardEvent kbe = event->key;
	SDL_Keycode vk = kbe.keysym.sym;
	if (vk == SDLK_ESCAPE) {
		return false;
	} else if ((vk >= 'a') && (vk <= 'z')) {
		int removed_char_index = -1;
		for (size_t i = 0; i < MAX_WORD_LENGTH - game_state->chars_entered; i++) {
			char c = game_state->remaining_chars[i];

			if (c == vk) {
				removed_char_index = i;
				break;
			}
		}

		if (removed_char_index == -1)
			return true;
		for (int i = removed_char_index; i < MAX_WORD_LENGTH; i++)
			game_state->remaining_chars[i] = game_state->remaining_chars[i + 1];

		game_state->curr_input[game_state->chars_entered++] = vk;

	} else if (vk == SDLK_BACKSPACE) {
		if (game_state->chars_entered != 0) {
			game_state->chars_entered--;
			char removed = game_state->curr_input[game_state->chars_entered];
			game_state->remaining_chars[
				MAX_WORD_LENGTH - game_state->chars_entered - 1] = removed;
			game_state->curr_input[game_state->chars_entered] = '\0';
		}
	} else if (vk == SDLK_SPACE) {
		shuffle(game_state->remaining_chars);
	} else if (vk == SDLK_RETURN) {
		size_t length = strlen(game_state->curr_input);
		int position;
		if (length < 3 || length > 6) {
			printf("%s is not the right length\n", game_state->curr_input);
		} else if (word_position(game_state->guessed_words, game_state->curr_input) != -1) {
			printf("you've already guessed %s\n", game_state->curr_input);
		} else if ((position = word_position(
						game_state->anagrams_by_length[length - 3],
						game_state->curr_input))
				!= -1) {
			printf("yep, %s is correct\n", game_state->curr_input);

			memcpy(GET_WORD(game_state->guessed_words, game_state->guessed_words->count),
					game_state->curr_input, length);
			game_state->guessed_words->count++;

			render_guessed_word(game_state, game_state->curr_input);

			game_state->points += 10 * length * length;

			if (game_state->guessed_words->count == game_state->anagrams->count)
				puts("Congratulations, you guessed all the words!");
		} else {
			printf("no, %s is wrong\n", game_state->curr_input);
		}

		memcpy((game_state->remaining_chars + MAX_WORD_LENGTH) - game_state->chars_entered,
				game_state->curr_input, game_state->chars_entered);
		memset(game_state->curr_input, 0, game_state->chars_entered);
		game_state->chars_entered = 0;
	}

	return true;
}
