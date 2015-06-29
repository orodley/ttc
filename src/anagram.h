#ifndef ANAGRAM_H_
#define ANAGRAM_H_

// TODO: change this if we have modes with words longer than 6 characters
#define MAX_WORD_LENGTH 6

typedef struct WordTree
{
	struct WordTree *sub_nodes[26 + 1];
} WordTree;

typedef struct WordList
{
	size_t count;
	size_t elem_size;
	char words[];
} WordList;

#define GET_WORD(word_list, i) \
	((word_list)->words + ((i) * (word_list)->elem_size))

WordTree *word_list_to_tree(char **word_list);
WordList *make_word_list(size_t count, size_t elem_size);
WordList *find_all_anagrams(WordTree *tree, char *word);
int word_position(WordList *word_list, const char *word);
void shuffle(char *word);

#endif
