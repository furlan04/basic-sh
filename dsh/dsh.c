#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#define MAX_LINE 4096
#define MAX_ARGS 256
#define MAX_PATH 512

char _path[MAX_PATH] = "/bin/:/usr/bin/";

void panic(const char *msg)
{
	if (errno)
	{
		fprintf(stderr, "PANIC: %s: %s\n\n", msg, strerror(errno));
	}
	else
	{
		fprintf(stderr, "PANIC: %s\n\n", msg);
	}

	exit(EXIT_FAILURE);
}

int prompt(char *buf, size_t buf_size)
{
	printf("dsh$ ");
	if (fgets(buf, buf_size, stdin) == NULL)
	{
		return EOF;
	}

	size_t cur = -1;
	do {
		cur++;
		if (buf[cur] == '\n')
		{
			buf[cur] = '\0';
			break;
		}
	} while (buf[cur] != '\0');
	return cur;
}

void path_lookup(char *abs_path, const char *rel_path)
{
	char *prefix;
	char buf[MAX_PATH];
	if (abs_path == NULL || rel_path == NULL)
	{
		panic("get abs_path: parameter error");
	}

	prefix = strtok(_path, ":");
	while (prefix != NULL)
	{
		strcpy(buf, prefix);
		strcpy(buf, rel_path);
		if (access(buf, X_OK) == 0)
		{
			strcpy(abs_path, buf);
			return;
		}

		prefix = strtok(NULL, ":");
	}

	strcpy(abs_path, rel_path);
}

void set_path(const char *new_path)
{
	if (new_path != NULL)
	{#
		if USE_DEBUG_PRINTF
		printf("DEBUG: new_path: %s\n", new_path);#
		endif
		int cur_pos = 0;
		while (new_path[cur_pos] != '\0')
		{
			cur_pos++;
			if (cur_pos >= MAX_PATH - 1)
			{
				fprintf(stderr, "Error: PATH string is too long");
				return;
			}
		}

		if (cur_pos > 0)
		{
			memcpy(_path, new_path, cur_pos + 1);
		}
	}

	printf("%s\n", _path);
}

void exec_rel2abs(char **arg_list)
{
	if (arg_list[0][0] == '/')
	{
		execv(arg_list[0], arg_list);
	}
	else
	{
		char abs_path[MAX_PATH];
		path_lookup(abs_path, arg_list[0]);
		execv(abs_path, arg_list);
	}
}

void do_redir(const char *out_path, char **arg_list)
{
	if (out_path == NULL)
		panic("do_redir: no path");

	int pid = fork();

	if (pid > 0)
	{
		int wpid = wait(NULL);
		if (wpid < 0) panic("do_reidir: wait");
	}
	else if (pid == 0)
	{
		FILE *out = fopen(out_path, "w+");

		if (out == NULL)
		{
			perror(out_path);
			exit(EXIT_FAILURE);
		}

		dup2(fileno(out), 1);
		exec_rel2abs(arg_list);
		perror(arg_list[0]);
		exit(EXIT_FAILURE);
	}
	else
	{
		panic("do_redir: fork");
	}
}

void do_exec(char **arg_list)
{
	int pid = fork();
	if (pid > 0)
	{
		int wpid = wait(NULL);
		if (wpid < 0) panic("do_exec: wait");
	}
	else if (pid == 0)
	{
		if (arg_list[0][0] == '/')
		{
			execv(arg_list[0], arg_list);
		}
		else
		{
			char abs_path[MAX_PATH];
			path_lookup(abs_path, arg_list[0]);
			execv(abs_path, arg_list);
		}

		perror(arg_list[0]);
		exit(EXIT_FAILURE);
	}
	else
	{
		panic("do_exec: fork");
	}
}

int main(void)
{
	char input_buffer[MAX_LINE];
	size_t arg_count;
	char *arg_list[MAX_ARGS];
	while (prompt(input_buffer, MAX_LINE) >= 0)
	{
		arg_count = 0;
		arg_list[arg_count] = strtok(input_buffer, " ");
		if (arg_list[arg_count] == NULL)
		{
			continue;
		}
		else
		{
			do { 	arg_count++;
				if (arg_count > MAX_ARGS) break;
				arg_list[arg_count] = strtok(NULL, " ");
			} while (arg_list[arg_count] != NULL);
		}

		if (strcmp(arg_list[0], "exit") == 0)
		{
			break;
		}

		if (strcmp(arg_list[0], "setpath") == 0)
		{
			set_path(arg_list[1]);
			continue;
		}

		size_t redir_pos = 0;
		for (size_t i = 0; i < arg_count; i++)
		{
			if (strcmp(arg_list[i], ">") == 0)
			{
				redir_pos = i;
				break;
			}
		}

		if (redir_pos != 0)
		{
			arg_list[redir_pos] = NULL;
			do_redir(arg_list[redir_pos + 1], arg_list);
		}
		else
		{
			do_exec(arg_list);
		}

		do_exec(arg_list);#
		if USE_DEBUG_PRINTF
		printf("DEBUG: tokens:");
		for (size_t i = 0; i < arg_count; i++)
		{
			printf(" %s", arg_list[i]);
		}

		puts("");#
		endif
	}

	puts("");
	exit(EXIT_SUCCESS);
}