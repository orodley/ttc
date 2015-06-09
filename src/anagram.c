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

#define MAX_ANAGRAMS 256
// TODO: change this if we have modes with words longer than 6 characters
#define MAX_WORD_LENGTH 6

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

AnagramsResult find_all_anagrams(WordTree *tree, char *word)
{
	AnagramsResult result;

	char word_copy[MAX_WORD_LENGTH];
	size_t word_len = strlen(word);

	for (size_t i = 0; i < word_len; i++)
		assert((word[i] >= 'a') && (word[i] <= 'z'));

	char *anagrams = calloc(MAX_ANAGRAMS + 1, MAX_WORD_LENGTH + 1);
	char char_trail[MAX_WORD_LENGTH];
	size_t anagram_index = 0;
	strncpy(word_copy, word, word_len);
	find_all_anagrams_aux(anagrams, &anagram_index, tree,
			word_copy, word_len, char_trail, 0);

	result.anagrams = anagrams;
	result.count = anagram_index;

	return result;
}
