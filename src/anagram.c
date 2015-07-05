#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "anagram.h"

#define WORD_TERMINATOR_INDEX 26
#define TERMINATOR ((void *)1)

WordTree *word_list_to_tree(char **word_list)
{
	WordTree *tree = calloc(1, sizeof *tree);

	for (int word_index = 0; word_list[word_index] != NULL; word_index++) {
		char *word = word_list[word_index];
		WordTree *node = tree;

		for (int word_index = 0; word[word_index] != '\0'; word_index++) {
			char c = word[word_index];
			assert((c >= 'a') && (c <= 'z'));
			size_t index = c - 'a';

			WordTree *next_node = node->sub_nodes[index];
			if (next_node == NULL) {
				next_node = calloc(1, sizeof *next_node);
				node->sub_nodes[index] = next_node;
			}

			node = next_node;
		}

		node->sub_nodes[WORD_TERMINATOR_INDEX] = TERMINATOR;
	}

	return tree;
}

WordList *make_word_list(size_t count, size_t elem_size)
{
	WordList *word_list = calloc(1, sizeof(*word_list) + count * elem_size);
	word_list->elem_size = elem_size;
	word_list->count = 0;

	return word_list;
}

#define MAX_ANAGRAMS 256

// TODO: Use char ***anagrams and remove anagram index?
static void find_all_anagrams_aux(char *anagrams, size_t *anagram_index,
		WordTree *tree, char *chars_to_use, size_t chars_to_use_length,
		char *char_trail, size_t char_trail_length)
{
	if (tree->sub_nodes[WORD_TERMINATOR_INDEX] == TERMINATOR) {
		strncpy(anagrams + (*anagram_index) * (MAX_WORD_LENGTH + 1),
				char_trail,
				char_trail_length);
		anagrams[((*anagram_index) * (MAX_WORD_LENGTH + 1)) - 1] = '\0';
		*anagram_index += 1;
	}

	if (chars_to_use_length == 0)
		return;

	char original[MAX_WORD_LENGTH];
	memcpy(original, chars_to_use, chars_to_use_length);

	char already_examined[MAX_WORD_LENGTH];
	size_t already_examined_length = 0;
	char using = chars_to_use[chars_to_use_length - 1];
	for (int i = (int)chars_to_use_length - 2;; i--) {
		bool skip = false;
		for (size_t j = 0; j < already_examined_length; j++) {
			if (already_examined[j] == using) {
				skip = true;
				break;
			}
		}

		WordTree *subtree = tree->sub_nodes[using - 'a'];
		if ((subtree != NULL) && !skip) {
			char_trail[char_trail_length] = using;
			find_all_anagrams_aux(anagrams, anagram_index, subtree,
					chars_to_use, chars_to_use_length - 1,
					char_trail, char_trail_length + 1);
		}

		if (i < 0)
			break;

		already_examined[already_examined_length++] = using;
		char next = chars_to_use[i];
		chars_to_use[i] = using;
		using = next;
	}

	memcpy(chars_to_use, original, chars_to_use_length);
}

WordList *find_all_anagrams(WordTree *tree, char *word)
{
	WordList *result = make_word_list(MAX_ANAGRAMS, MAX_WORD_LENGTH + 1);

	char word_copy[MAX_WORD_LENGTH];
	size_t word_len = strlen(word);

	for (size_t i = 0; i < word_len; i++)
		assert((word[i] >= 'a') && (word[i] <= 'z'));

	char char_trail[MAX_WORD_LENGTH];
	size_t anagram_index = 0;
	strncpy(word_copy, word, word_len);
	find_all_anagrams_aux(result->words, &anagram_index, tree,
			word_copy, word_len, char_trail, 0);

	result->count = anagram_index;
	result->elem_size = MAX_WORD_LENGTH + 1;

	return result;
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
