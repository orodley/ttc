#define _XOPEN_SOURCE 500
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "anagram.h"
#include "words.h"

static SDL_Texture *large_letter_textures[26];
static SDL_Texture *small_letter_textures[26];
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

int word_position(WordList *word_list, const char *word)
{
	size_t length = strlen(word) + 1;
	for (size_t i = 0; i < word_list->count; i++) {
		if (strncmp(GET_WORD(word_list, i), word, length) == 0)
			return i;
	}

	return -1;
}

void separate_words(WordList **separated_words, const WordList *words)
{
	int length_counts[MAX_WORD_LENGTH - 2] = { 0 };
	for (size_t i = 0; i < words->count; i++) {
		const char *word = GET_WORD(words, i);
		size_t length = strlen(word);
		assert((length >= 3) && (length <= MAX_WORD_LENGTH));
		length_counts[length - 3]++;
	}

	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		WordList *word_list = make_word_list(length_counts[i], i + 4);
		word_list->count = 0;
		separated_words[i] = word_list;
	}

	for (size_t i = 0; i < words->count; i++) {
		const char *word = GET_WORD(words, i);
		size_t length = strlen(word);
		WordList *word_list = separated_words[length - 3];
		memcpy(GET_WORD(word_list, word_list->count), word, length);
		word_list->count++;
	}

	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		WordList *word_list = separated_words[i];

		qsort(word_list->words, word_list->count, word_list->elem_size,
				(int (*)(const void *, const void *))strcmp);
	}
}

int **compute_layout(WordList **separated_words, int max_rows)
{
	int *row_sizes[MAX_WORD_LENGTH - 2];
	int total_words = 0;
	for (int i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		size_t count = separated_words[i]->count;
		total_words += count;
		row_sizes[i] = malloc(sizeof(int) * (count + 1));

		for (size_t j = 0; j < count; j++)
			row_sizes[i][j] = 1;
		row_sizes[i][count] = 0;
	}

	int total_rows = total_words;
	int max_cols = 1;
	size_t curr_group = 0;
	size_t distribution_index = 0;
	while (total_rows > max_rows) {
		int *row_group = row_sizes[curr_group];
		// TODO: We could cache the number of rows in each group instead of recomputing it
		size_t i = 0;
		while (row_group[i] != 0)
			i++;
		if (i == 0)
			continue;
		i--;

		int to_distribute = row_group[i];

		if (i < distribution_index + to_distribute) {
			if (curr_group == 0)
				max_cols++;

			curr_group = (curr_group + 1) % (MAX_WORD_LENGTH - 2);
			distribution_index = 0;
			continue;
		}

		for (size_t k = distribution_index; k < distribution_index + to_distribute; k++)
			row_group[k]++;

		distribution_index += to_distribute;
		row_group[i] = 0;
		total_rows--;
	}

	// TODO: maybe it's just as easy to compute it column-wise instead of
	// row-wise in the first place?
	int **col_sizes = malloc(sizeof(*col_sizes) * (MAX_WORD_LENGTH - 2));
	for (int i = 0; i < MAX_WORD_LENGTH - 2; i++)
		col_sizes[i] = calloc(max_cols, sizeof(int));

	for (int i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		int *col_group = col_sizes[i];
		int *row_group = row_sizes[i];

		int curr_col = row_group[0];
		int j;
		for (j = 0; row_group[j] != 0; j++) {
			int row_size = row_group[j];

			if (row_size < curr_col) {
				for (int k = row_size; k < curr_col; k++)
					col_group[k] = j;
				curr_col = row_size;
			}
		}
		for (; curr_col > 0; curr_col--)
			col_group[curr_col - 1] = j;
	}

	for (int i = 0; i < MAX_WORD_LENGTH - 2; i++)
		free(row_sizes[i]);

	return col_sizes;
}

bool prerender_letters(SDL_Texture **textures, SDL_Renderer *renderer,
		char *font_name, int size)
{
	TTF_Font *font = TTF_OpenFont(font_name, size);
	if (font == NULL)
		return false;
	int minx;
	int maxx;
	int miny;
	int maxy;
	int advance;
	TTF_GlyphMetrics(font, 'I', &minx, &maxx, &miny, &maxy, &advance);
	printf("at size %d, minx = %d, maxx = %d, miny = %d, maxy = %d, advance = %d\n",
			size, minx, maxx, miny, maxy, advance);

	for (int i = 0; i < 26; i++) {
		SDL_Color black = {0, 0, 0, 255};
		SDL_Surface* letter_surface =
			TTF_RenderGlyph_Blended(font, 'A' + i, black);
		SDL_Texture* letter_texture =
			SDL_CreateTextureFromSurface(renderer, letter_surface);
		SDL_SetTextureBlendMode(letter_texture, SDL_BLENDMODE_BLEND);

		textures[i] = letter_texture;

		SDL_FreeSurface(letter_surface);
	}

	return true;
}

void render_word(SDL_Renderer *renderer, SDL_Texture **letter_textures,
		const char *letters, int x, int y, int step)
{
	SDL_Rect letter_box = {
		.x = x,
		.y = y,
		.h = 0,
		.w = 0,
	};

	for (int i = 0; letters[i] != '\0'; i++) {
		SDL_Texture *letter_texture = letter_textures[letters[i] - 'a'];
		SDL_QueryTexture(letter_texture, NULL, NULL, &letter_box.w, &letter_box.h);

		SDL_RenderCopy(renderer, letter_texture, NULL, &letter_box);

		letter_box.x += step;
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

#define WORDS_START_X 10
#define WORDS_START_Y 10
#define LETTER_SPACING 2
#define LETTER_HEIGHT 20
#define BOX_SPACING 3
#define BOX_HEIGHT (LETTER_HEIGHT + LETTER_SPACING * 2)
#define BOX_WIDTH (BOX_HEIGHT * 0.75)
#define ROW_SPACING BOX_SPACING
#define COLUMN_SPACING 6

SDL_Texture *render_empty_words(SDL_Renderer *renderer, WordList **word_lists,
		int **col_sizes, int width, int height)
{
	SDL_Texture *t = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_UNKNOWN,
			SDL_TEXTUREACCESS_TARGET, width, height);
	SDL_Texture *original_render_target = SDL_GetRenderTarget(renderer);
	SDL_SetRenderTarget(renderer, t);

	SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	SDL_Rect box = { 
		.x = WORDS_START_X,
		.y = WORDS_START_Y,
		.h = BOX_HEIGHT,
		.w = BOX_WIDTH,
	};

	for (int i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		int group_start_y = box.y;
		int *group_col_sizes = col_sizes[i];
		WordList *word_list = word_lists[i];
		int word_length = i + 3;
		int curr_col_index = 0;
		int words_left_in_column = group_col_sizes[curr_col_index];

		for (size_t j = 0; j < word_list->count; j++) {
			int word_start_x = box.x;
			for (int n = 0; n < word_length; n++) {
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				SDL_RenderFillRect(renderer, &box);
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderDrawRect(renderer, &box);

				box.x += box.w + BOX_SPACING;
			}

			words_left_in_column--;
			if (words_left_in_column == 0) {
				curr_col_index++;
				words_left_in_column = group_col_sizes[curr_col_index];

				box.y = group_start_y;
				box.x += COLUMN_SPACING;
			} else {
				box.x = word_start_x;
				box.y += box.h + ROW_SPACING;
			}
		}

		box.x = WORDS_START_X;
		box.y += group_col_sizes[0] * (box.h + ROW_SPACING);
	}

	SDL_SetRenderTarget(renderer, original_render_target);
	return t;
}

#define SECOND_ELAPSED 0

uint32_t push_time_event(uint32_t interval, void *param)
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

int main(void)
{
	WordTree *tree = word_list_to_tree(words);

	struct timeval t;
	gettimeofday(&t, NULL);
	srand(t.tv_usec * t.tv_sec);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
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
	if (!prerender_letters(large_letter_textures, renderer, "font.ttf", font_size))
		return 7;
	if (!prerender_letters(small_letter_textures, renderer, "font.ttf", 18))
		return 7;
	TTF_Font *font = TTF_OpenFont("font.ttf", font_size);
	if (font == NULL)
		return 7;

	SDL_Color center_color = {0, 239, 235, 255};
	SDL_Color corner_color = {0, 124, 235, 255};
	background = render_radial_gradient(renderer, width, height,
			center_color, corner_color);

	char *word = strdup(random_word(words, MAX_WORD_LENGTH));

	printf("word is %s\n", word);
	shuffle(word);
	printf("word is %s\n", word);

	WordList *anagrams = find_all_anagrams(tree, word);
	printf("%zu anagrams:\n", anagrams->count);

	WordList *words[MAX_WORD_LENGTH - 2];
	separate_words(words, anagrams);
	for (size_t i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		printf("%zu: %zu words\n", i + 3, words[i]->count);

		for (size_t j = 0; j < words[i]->count; j++)
			printf("\t%s\n", GET_WORD(words[i], j));
	}

#define MAX_COLS 5
	int box_height = LETTER_HEIGHT + LETTER_SPACING * 2;
	int max_rows = (height - 20) / (box_height + BOX_SPACING);
	int **col_sizes = compute_layout(words, max_rows);

	SDL_Texture *guessed_words_texture = render_empty_words(renderer, words,
			col_sizes, width, height);

	WordList *guessed_words =
		make_word_list(anagrams->count, MAX_WORD_LENGTH + 1);
	guessed_words->count = 0;

	char curr_input[MAX_WORD_LENGTH + 1];
	char remaining_chars[MAX_WORD_LENGTH + 1];
	size_t chars_entered = 0;

	memset(curr_input, 0, MAX_WORD_LENGTH + 1);
	memcpy(remaining_chars, word, MAX_WORD_LENGTH + 1);

	SDL_AddTimer(1000, push_time_event, NULL);
	int time_left = 180;
	
	for (;;) {
		SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, background, NULL, NULL);
		SDL_RenderCopy(renderer, guessed_words_texture, NULL, NULL);

		render_word(renderer, large_letter_textures, curr_input,
				width / 2, height / 2, font_size * 1.5);
		render_word(renderer, large_letter_textures, remaining_chars,
				width / 2, height / 2 + font_size * 2, font_size * 1.5);

		int minutes = time_left / 60;
		int seconds = time_left % 60;

		size_t str_len = sizeof "3:00";
		char time_str[str_len];
		snprintf(time_str, str_len, "%d:%02d", minutes, seconds);
		SDL_Color black = { 0, 0, 0, 255 };
		SDL_Surface *time_surface = TTF_RenderText_Blended(font, time_str, black);
		SDL_Texture *time_texture = SDL_CreateTextureFromSurface(renderer, time_surface);

		SDL_Rect time_pos = {
			.x = width / 2,
			.y = height / 2 + font_size * 5,
		};
		SDL_QueryTexture(time_texture, NULL, NULL, &time_pos.w, &time_pos.h);
		SDL_RenderCopy(renderer, time_texture, NULL, &time_pos);

        SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);

		SDL_FreeSurface(time_surface);
		SDL_DestroyTexture(time_texture);

		SDL_Event event;
		SDL_WaitEvent(&event);

		if (event.type == SDL_USEREVENT) {
			switch (event.user.code) {
			case SECOND_ELAPSED:
				time_left--;

				if (time_left == 0) {
					puts("Time's up!");
					return 0;
				}

				break;
			}

			continue;
		} else if (event.type != SDL_KEYDOWN) {
			continue;
		}

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
			size_t length = strlen(curr_input);
			int position;
			if (length < 3 || length > 6) {
				printf("%s is not the right length\n", curr_input);
			} else if (word_position(guessed_words, curr_input) != -1) {
				printf("you've already guessed %s\n", curr_input);
			} else if ((position = word_position(words[length - 3],
							curr_input)) != -1) {
				printf("yep, %s is correct\n", curr_input);

				memcpy(GET_WORD(guessed_words, guessed_words->count),
						curr_input, length);
				guessed_words->count++;

				int x = WORDS_START_X;
				int y = WORDS_START_Y;

				int *cols = col_sizes[length - 3];
				int column = 0;
				int row = position;
				int column_size = cols[column];
				while (row >= column_size) {
					column++;
					row -= column_size;
					column_size = cols[column];
				}

				x += column * length * (BOX_WIDTH + BOX_SPACING);
				x += column * COLUMN_SPACING;

				for (size_t i = 0; i < length - 3; i++)
					y += col_sizes[i][0] * (BOX_HEIGHT + BOX_SPACING);

				y += row * (BOX_HEIGHT + BOX_SPACING);

				// Hack to get letters placed slightly better. We should center
				// glyphs when we render them to do this properly.
				x += 2; 
				y += 2;

				SDL_Texture *original_render_target =
					SDL_GetRenderTarget(renderer);
				SDL_SetRenderTarget(renderer, guessed_words_texture);
				render_word(renderer, small_letter_textures,
						curr_input, x, y, BOX_WIDTH + BOX_SPACING);

				SDL_SetRenderTarget(renderer, original_render_target);

				if (guessed_words->count == anagrams->count)
					puts("Congratulations, you guessed all the words!");
			} else {
				printf("no, %s is wrong\n", curr_input);
			}

			memcpy((remaining_chars + MAX_WORD_LENGTH) - chars_entered,
					curr_input, chars_entered);
			memset(curr_input, 0, chars_entered);
			chars_entered = 0;
		}
	}

	return 0;
}
