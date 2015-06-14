// TODO: change this if we have modes with words longer than 6 characters
#define MAX_WORD_LENGTH 6

typedef struct WordTree
{
	struct WordTree *sub_nodes[26 + 1];
} WordTree;

typedef struct WordList
{
	size_t count;
	char words[];
} WordList;

WordTree *word_list_to_tree(char **word_list);
WordList *find_all_anagrams(WordTree *tree, char *word);
