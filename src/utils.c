#include <utils.h>
#include <vec.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stddef.h>

char *build_path(char *path, char *path_env)
{
	struct stat sinfo;
	if (!stat(path, &sinfo))
		return strdup(path);
	char **path_list = split(path_env, ':');
	for (char **traverser = path_list; *traverser; traverser++) {
		size_t total_length = strlen(*traverser) + strlen(path);
		char *joined = malloc(
			(total_length + 1 + 1) *
			sizeof(char)); // 1 for null byte and 1 for the '/' character
		snprintf(joined, total_length + 2, "%s/%s", *traverser, path);
		if (!stat(joined, &sinfo)) {
			freevec(path_list);
			return joined;
		}
		free(joined);
	}
	freevec(path_list);
	return strdup(path);
}
