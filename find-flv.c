/*
 * find-flv.c - Simple program to look for deleted streaming flash files
 *
 * Copyright (C) 2013		Andrew Clayton <andrew@digital-domain.net>
 *
 * Released under the GNU General Public License version 2
 * See COPYING
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>

#define FLASH_STR	"/tmp/Flash"

static char (*flvs)[PATH_MAX];	/* Array of /proc/pid/fd flv file paths */
static int nr_flvs;

/*
 * Look through a plugin-container processes /proc/pid/fd directory for
 * deleted flash files.
 */
void scan_plugin_dir(const char *proc)
{
	DIR *dir;
        struct dirent64 *entry;
	char path[PATH_MAX];

	snprintf(path, PATH_MAX, "/proc/%s/fd", proc);
	dir = opendir(path);
	while ((entry = readdir64(dir)) != NULL) {
		struct stat sb;
		char file[PATH_MAX];

		snprintf(file, PATH_MAX, "%s/%s", path, entry->d_name);
		lstat(file, &sb);
		if (S_ISLNK(sb.st_mode)) {
			char target[PATH_MAX];

			memset(target, '\0', sizeof(target));
			readlink(file, target, PATH_MAX);
			if (strstr(target, FLASH_STR)) {
				int fd;

				fd = open(file, O_RDONLY | O_CLOEXEC);
				printf("[%s]\t%s\n", proc, target);
				snprintf(file, PATH_MAX, "/proc/%d/fd/%d",
						getpid(), fd);
				nr_flvs++;
				flvs = realloc(flvs, nr_flvs * sizeof(*flvs));
				strcpy(flvs[nr_flvs - 1], file);
			}
		}
	}
	closedir(dir);
}

/*
 * Look through /proc/pid directories for plugin-container processes.
 */
void scan_procfs(void)
{
	DIR *dir;
	struct dirent64 *entry;

	dir = opendir("/proc");
	while ((entry = readdir64(dir)) != NULL) {
		/*
		 * Skip entries starting with a . or don't start with a digit
		 */
		if (entry->d_name[0] == '.' ||
		    !isdigit(entry->d_name[0]))
			continue;
		char comm[PATH_MAX];
		char task[16];
		FILE *fp;

		/*
		 * /proc/pid/comm contains an at most 15 character process
		 * name.
		 */
		snprintf(comm, PATH_MAX, "/proc/%s/comm", entry->d_name);
		fp = fopen(comm, "r");
		fscanf(fp, "%15s", task);
		fclose(fp);
		if (strstr(task, "plugin-contain")) {
			printf("Process %s is a %s\n", entry->d_name, task);
			scan_plugin_dir(entry->d_name);
		}
	}
	closedir(dir);
}

int main(int argc, char **argv)
{
	scan_procfs();

	if (flvs) {
		int i;

		for (i = 0; i < nr_flvs; i++)
			printf("Opened %s\n", flvs[i]);
		free(flvs);

		pause();
	} else {
		printf("No flash videos found.\n");
	}

	exit(EXIT_SUCCESS);
}
