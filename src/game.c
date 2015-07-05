#define _XOPEN_SOURCE 500
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "anagram.h"
#include "game.h"
#include "render.h"

typedef enum UserEvent
{
	SECOND_ELAPSED_EVENT,
} UserEvent;

static uint32_t timer_handler(uint32_t interval, void *param)
{
	Game *game = (Game *)param;
	if (game->state == IN_LEVEL) {
		SDL_Event event;
		SDL_UserEvent userevent;

		userevent.type = SDL_USEREVENT;
		userevent.code = SECOND_ELAPSED_EVENT;

		event.type = SDL_USEREVENT;
		event.user = userevent;

		SDL_PushEvent(&event);
		return interval;
	} else {
		return 0;
	}
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


void new_level(Game *game)
{
	char *word = strdup(random_word(game->all_words_array, MAX_WORD_LENGTH));

	printf("word is %s\n", word);
	shuffle(word);
	printf("word is %s\n", word);

	game->anagrams = find_all_anagrams(game->all_words_tree, word);
	printf("%zu anagrams:\n", game->anagrams->count);

	WordList **anagrams_by_length = malloc(sizeof(*anagrams_by_length) *
			(MAX_WORD_LENGTH - 2));
	sort_words(anagrams_by_length, game->anagrams);
	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		printf("%zu: %zu words\n", i + 3, anagrams_by_length[i]->count);

		for (size_t j = 0; j < anagrams_by_length[i]->count; j++)
			printf("\t%s\n", GET_WORD(anagrams_by_length[i], j));
	}

	game->anagrams_by_length = anagrams_by_length;

	game->column_sizes = compute_layout(game);

	game->guessed_words =
		make_word_list(game->anagrams->count, MAX_WORD_LENGTH + 1);

	SDL_Color white = { 0xFF, 0xFF, 0xFF, 0xFF };
	game->guessed_words_texture = render_empty_words(game, white);
	SDL_DestroyTexture(game->message_box);
	game->message_box = NULL;

	game->chars_entered = 0;

	memset(game->curr_input, 0, MAX_WORD_LENGTH + 1);
	memcpy(game->remaining_chars, word, MAX_WORD_LENGTH + 1);

	if (game->second_timer != 0)
		SDL_RemoveTimer(game->second_timer);
	game->second_timer = SDL_AddTimer(1000, timer_handler, game);
	game->time_left = 180;

	game->state = IN_LEVEL;
}

static bool handle_event_in_level(Game *game, SDL_Event *event);
static bool handle_event_won_level(Game *game, SDL_Event *event);
static bool handle_event_game_over(Game *game, SDL_Event *event);

bool handle_event(Game *game, SDL_Event *event)
{
	switch (game->state) {
	case IN_LEVEL: return handle_event_in_level(game, event);
	case WON_LEVEL: return handle_event_won_level(game, event);
	case GAME_OVER: return handle_event_game_over(game, event);
	default: assert(!"invalid state");
	}
}

static bool handle_event_in_level(Game *game, SDL_Event *event)
{
	if (event->type == SDL_USEREVENT) {
		switch (event->user.code) {
		case SECOND_ELAPSED_EVENT:
			game->time_left--;

			if (game->time_left == 0) {
				puts("Time's up!");

				SDL_Color blue = { 0, 0xDB, 0xEB, 0xFF };
				SDL_Texture *missed_words = render_empty_words(game, blue);
				SDL_Texture *original_render_target =
					SDL_GetRenderTarget(game->renderer);
				SDL_SetRenderTarget(game->renderer, game->guessed_words_texture);
				SDL_RenderCopy(game->renderer, missed_words, NULL, NULL);
				SDL_SetRenderTarget(game->renderer, original_render_target);

				// TODO: This could be made more efficient by keeping an index
				// into guessed_words too.
				for (size_t i = 0; i < game->anagrams->count; i++) {
					char *word = GET_WORD(game->anagrams, i);
					if (word_position(game->guessed_words, word) == -1)
						render_guessed_word(game, word);
				}

				bool guessed_a_six_letter_word = false;
				for (size_t i = 0; i < game->guessed_words->count; i++) {
					if (strlen(GET_WORD(game->guessed_words, i)) == 6) {
						guessed_a_six_letter_word = true;
						break;
					}
				}

				if (guessed_a_six_letter_word) {
					char *message = "You qualify for the next round!";
					puts(message);
					game->state = WON_LEVEL;
					game->message_box = render_message_box(game, message);

					return true;
				} else {
					printf("You suck, you failed. No new level for you\n"
							"Your final score was %d\n", game->points);
					game->state = GAME_OVER;
					game->message_box = render_message_box(game, "Game over, loser");
					return true;
				}
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
		for (size_t i = 0; i < MAX_WORD_LENGTH - game->chars_entered; i++) {
			char c = game->remaining_chars[i];

			if (c == vk) {
				removed_char_index = i;
				break;
			}
		}

		if (removed_char_index == -1)
			return true;
		for (int i = removed_char_index; i < MAX_WORD_LENGTH; i++)
			game->remaining_chars[i] = game->remaining_chars[i + 1];

		game->curr_input[game->chars_entered++] = vk;

	} else if (vk == SDLK_BACKSPACE) {
		if (game->chars_entered != 0) {
			game->chars_entered--;
			char removed = game->curr_input[game->chars_entered];
			game->remaining_chars[
				MAX_WORD_LENGTH - game->chars_entered - 1] = removed;
			game->curr_input[game->chars_entered] = '\0';
		}
	} else if (vk == SDLK_SPACE) {
		shuffle(game->remaining_chars);
	} else if (vk == SDLK_RETURN) {
		size_t length = strlen(game->curr_input);
		int position;
		if (length < 3 || length > 6) {
			printf("%s is not the right length\n", game->curr_input);
		} else if (word_position(game->guessed_words, game->curr_input) != -1) {
			printf("you've already guessed %s\n", game->curr_input);
		} else if ((position = word_position(
						game->anagrams_by_length[length - 3],
						game->curr_input))
				!= -1) {
			printf("yep, %s is correct\n", game->curr_input);

			memcpy(GET_WORD(game->guessed_words, game->guessed_words->count),
					game->curr_input, length);
			game->guessed_words->count++;

			render_guessed_word(game, game->curr_input);

			game->points += 10 * length * length;

			if (game->guessed_words->count == game->anagrams->count)
				puts("Congratulations, you guessed all the words!");
		} else {
			printf("no, %s is wrong\n", game->curr_input);
		}

		memcpy((game->remaining_chars + MAX_WORD_LENGTH) - game->chars_entered,
				game->curr_input, game->chars_entered);
		memset(game->curr_input, 0, game->chars_entered);
		game->chars_entered = 0;
	}

	return true;
}

static bool handle_event_won_level(Game *game, SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN) {
		switch (event->key.keysym.sym) {
		case SDLK_RETURN:
			new_level(game);
			break;
		case SDLK_ESCAPE:
			return false;
		}
	} 

	return true;
}

static bool handle_event_game_over(Game *game, SDL_Event *event)
{
	(void)game;
	return !(event->type == SDL_KEYDOWN);
}
