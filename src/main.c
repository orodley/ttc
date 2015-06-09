#include <stdio.h>
#include <string.h>
#include "anagram.h"
#include "words.h"

int main(int argc, char *argv[])
{
	if (argc != 2)
		return 1;
	char *word = argv[1];
	if (strlen(word) > 6)
		return 2;

	WordTree *tree = word_list_to_tree(words);
	AnagramsResult result = find_all_anagrams(tree, word);
	for (size_t i = 0; i < result.count; i++)
		printf("%s\n", result.anagrams + i * 7);

	return 0;
}
