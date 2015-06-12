typedef struct WordTree
{
	struct WordTree *sub_nodes[26 + 1];
} WordTree;

typedef struct AnagramsResult
{
	char *anagrams;
	size_t count;
} AnagramsResult;

WordTree *word_list_to_tree(char **word_list);
AnagramsResult find_all_anagrams(WordTree *tree, char *word);