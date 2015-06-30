#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "anagram.h"
#include "game.h"

#define WORDS_START_X 10
#define WORDS_START_Y 10
#define LETTER_SPACING 2
#define LETTER_HEIGHT 20
#define BOX_SPACING 3
#define BOX_HEIGHT (LETTER_HEIGHT + LETTER_SPACING * 2)
#define BOX_WIDTH (BOX_HEIGHT * 0.75)
#define ROW_SPACING BOX_SPACING
#define COLUMN_SPACING 6

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

static void render_letters(SDL_Renderer *renderer, SDL_Texture **letter_textures,
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

static void render_text(SDL_Renderer *renderer, TTF_Font *font, char *text,
		int x, int y)
{
	SDL_Color black = { 0, 0, 0, 255 };
	SDL_Surface *surface = TTF_RenderText_Blended(font, text, black);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_Rect pos = { .x = x, .y = y };
	SDL_QueryTexture(texture, NULL, NULL, &pos.w, &pos.h);
	SDL_RenderCopy(renderer, texture, NULL, &pos);

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void render_guessed_word(Game *game, char *word)
{
	size_t length = strlen(word);
	size_t position = word_position(game->anagrams_by_length[length - 3], word);

	int x = WORDS_START_X;
	int y = WORDS_START_Y;

	int *cols = game->column_sizes[length - 3];
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
		y += game->column_sizes[i][0] * (BOX_HEIGHT + BOX_SPACING);

	y += row * (BOX_HEIGHT + BOX_SPACING);

	// Hack to get letters placed slightly better. We should center
	// glyphs when we render them to do this properly.
	x += 2; 
	y += 2;

	SDL_Texture *original_render_target =
		SDL_GetRenderTarget(game->renderer);
	SDL_SetRenderTarget(game->renderer, game->guessed_words_texture);
	render_letters(game->renderer, game->small_letter_textures,
			word, x, y, BOX_WIDTH + BOX_SPACING);

	SDL_SetRenderTarget(game->renderer, original_render_target);
}

static int distance(int x1, int y1, int x2, int y2)
{
	int xdiff = x1 - x2;
	int ydiff = y1 - y2;
	return (int)sqrtf(xdiff * xdiff + ydiff * ydiff);
}

static SDL_Color color_lerp(SDL_Color c1, float proportion, SDL_Color c2)
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

int **compute_layout(Game *game)
{
	int box_height = LETTER_HEIGHT + LETTER_SPACING * 2;
	int max_rows = (game->window_height - 20) / (box_height + BOX_SPACING);

	int *row_sizes[MAX_WORD_LENGTH - 2];
	int total_words = 0;
	for (int i = 0; i < MAX_WORD_LENGTH - 2; i++) {
		size_t count = game->anagrams_by_length[i]->count;
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

void render_game(Game *game)
{
	SDL_SetRenderDrawColor(game->renderer, 255, 0, 255, 255);
	SDL_RenderClear(game->renderer);
	SDL_RenderCopy(game->renderer, game->background, NULL, NULL);
	SDL_RenderCopy(game->renderer, game->guessed_words_texture, NULL, NULL);

	render_letters(game->renderer, game->large_letter_textures, game->curr_input,
			game->window_width / 2, game->window_height / 2, 24 * 1.5);
	render_letters(game->renderer, game->large_letter_textures, game->remaining_chars,
			game->window_width / 2, game->window_height / 2 + 24 * 2, 24 * 1.5);

	int minutes = game->time_left / 60;
	int seconds = game->time_left % 60;

	size_t time_str_len = sizeof "3:00";
	char time_str[time_str_len];
	snprintf(time_str, time_str_len, "%d:%02d", minutes, seconds);
	render_text(game->renderer, game->font, time_str, game->window_width / 2, game->window_height / 2 + 24 * 5);

	size_t points_str_len = sizeof "Points: 100000000";
	char points_str[points_str_len];
	snprintf(points_str, points_str_len, "Points: %d", game->points);
	render_text(game->renderer, game->font, points_str, game->window_width / 2, game->window_height / 2 + 24 * 7);

	SDL_RenderPresent(game->renderer);
	SDL_UpdateWindowSurface(game->window);
}
