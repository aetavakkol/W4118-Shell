#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct node {
	char *str;
	struct node *last;
};

char *cwd;
const int inputSize = 512;
struct node *dirStack = NULL, *pathStack = NULL;


int cmd_exec(char *argArr[], int argNum)
{
	int i, *status;
	char *args[argNum];
	for (i = 0; i < argNum; i++)
		args[i] = argArr[i];

	pid_t id = fork();
	if (id == 0) {
		/* Try to execute file in current dir */
		execv(args[0], args);

		/* Otherwise iterate over paths in path stack */
		struct node *iter;
		char *path, *cmd;
		iter = (struct node *) malloc(sizeof(struct node));
		path = (char *) malloc(sizeof(int) * inputSize * 2);
		cmd = (char *) malloc(sizeof(char) * strlen(args[0]));
		cmd = args[0];
		iter = pathStack;
		while (iter != NULL) {
			strcpy(path, iter->str);
			strcat(path, cmd);
			args[0] = path;
			execv(path, args);
			iter = iter->last;
		}
		free(iter);
		free(path);
		free(cmd);

		fprintf(stderr, "ERROR: unknown command\n");
		exit(EXIT_FAILURE);
	} else {
		wait(status);
		return 0;
	}
};


int cmd_cd(char *path)
{
	if (path != NULL) {
		if (chdir(path) == 0) {
			getcwd(cwd, sizeof(char) * inputSize);
			printf(" > %s\n", cwd);
			return 0;
		} else {
			fprintf(stderr, "ERROR: invalid path\n");
			return -1;
		}
		return 0;
	} else {
		fprintf(stderr, "ERROR: path required\n");
		return -1;
	}
};


struct node *stackPush(struct node *stk, char *path)
{
	if (path != NULL) {
		struct node *newN;
		newN = (struct node *) malloc(sizeof(struct node));
		newN->str = (char *) malloc(sizeof(char) * (strlen(path) + 1));
		strcpy(newN->str, path);
		newN->last = stk;
		stk = newN;
		return stk;
	} else {
		fprintf(stderr, "ERROR: path required\n");
		return stk;
	}
};


struct node *stackPop(struct node *stk)
{
	struct node *temp = stk;
	char *tempStr = (char *) malloc(sizeof(char) * inputSize);
	strcpy(tempStr, stk->str);
	stk = stk->last;
	free(temp);
	return stk;
};


void stackPrint(struct node *stk)
{
	struct node *iter = (struct node *) malloc(sizeof(struct node));
	printf("cwd: %s\n", cwd);
	iter = stk;
	while (iter != NULL) {
		printf(" > %s\n", iter->str);
		iter = iter->last;
	}
	free(iter);
};


/* Print in format required for path command */
void listPrint(struct node *stk)
{
	struct node *iter = (struct node *) malloc(sizeof(struct node));
	iter = stk;
	printf(" > ");
	if (iter != NULL) {
		printf(iter->str);
		iter = iter->last;
	}
	while (iter != NULL) {
		printf(":%s", iter->str);
		iter = iter->last;
	}
	printf("\n");
	free(iter);
};


/*
* Iterates over the stack of paths to either:
* Add the specified path if it does not exist
* Delete the specified path if it does exist
* Paths should be absolute to function
*/
int pathFind(int flag, char *path)
{
	struct node *curr = pathStack, *prev = NULL;
	int found = 0;

	if (path == NULL) {
		fprintf(stderr, "ERROR: path required\n");
		return -1;
	}

	/* Make last character a / */
	if (strcmp(path + strlen(path) - 1, "/") != 0)
		strcat(path, "/");

	/* Find path if in list */
	while (curr != NULL) {
		if (strcmp(curr->str, path) == 0) {
			found = 1;
			break;
		}
		prev = curr;
		curr = curr->last;
	}

	/* Add or delete path according to flag */
	if (found == 1) {
		if (flag == 1) {
			fprintf(stderr, "ERROR: path already present\n");
			return -1;
		} else if (flag == -1 && prev == NULL) {
			pathStack = stackPop(pathStack);
			return 0;
		} else if (flag == -1) {
			prev->last = curr->last;
			return 0;
		}
	} else {
		if (flag == 1) {
			pathStack = stackPush(pathStack, path);
			return 0;
		} else if (flag == -1) {
			fprintf(stderr, "ERROR: path not found\n");
			return -1;
		}
	}
	fprintf(stderr, "ERROR: path find failed\n");
	return -1;
}


/*
* Parses the input into commands and arguments using whitespace
* Then calls the associated function or error
* Maximum number of arguments is 5
*/
void parse(char *in)
{
	int argNum = 1;
	char *arg, *argArr[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
	arg = (char *) malloc(sizeof(char) * (strlen(in) + 1));

	/* Parse command */
	arg = strtok(in, " \r\n\t");
	argArr[0] = arg;
	if (argArr[0] == NULL) {
		fprintf(stderr, "ERROR: unknown command\n");
		return;
	}

	/* Parse arguments */
	arg = strtok(NULL, " \r\n\t");
	while (arg != NULL && argNum < 6) {
		if (strcmp(arg, " ") && strcmp(arg, "\r")
			&& strcmp(arg, "\n")) {
			argArr[argNum] = arg;
			argNum++;
		}
		arg = strtok(NULL, " \r\n\t");
	}

	/* Execute according to command */
	if (strcmp(argArr[0], "exit") == 0) {
		printf("EXITING SHELL...\n");
		exit(EXIT_SUCCESS);
	} else if (strcmp(argArr[0], "cd") == 0) {
		cmd_cd(argArr[1]);
	} else if (strcmp(argArr[0], "pushd") == 0) {
		char *oldwd = (char *) malloc(sizeof(char) * strlen(cwd));
		strcpy(oldwd, cwd);
		if (cmd_cd(argArr[1]) == 0)
			dirStack = stackPush(dirStack, oldwd);
	} else if (strcmp(argArr[0], "popd") == 0) {
		if (dirStack != NULL) {
			cmd_cd(dirStack->str);
			dirStack = stackPop(dirStack);
		} else {
			fprintf(stderr, "ERROR: stack is empty\n");
		}
	} else if (strcmp(argArr[0], "dirs") == 0) {
		stackPrint(dirStack);
	} else if (strcmp(argArr[0], "path") == 0) {
		if (argArr[1] == NULL && argArr[1] == NULL)
			listPrint(pathStack);
		else if (strcmp(argArr[1], "+") == 0)
			pathFind(1, argArr[2]);
		else if (strcmp(argArr[1], "-") == 0)
			pathFind(-1, argArr[2]);
		else
			fprintf(stderr, "ERROR: bad flag: %s\n", argArr[1]);
	} else {
		cmd_exec(argArr, argNum);
	}
};


/*
* Main shell fucntion,
* Currently can only accept input of a fixed number of characters
*/
int main(int argc, char **argv)
{
	char *in = (char *) malloc(sizeof(char) * inputSize);
	cwd = (char *) malloc(sizeof(char) * inputSize);
	getcwd(cwd, sizeof(char) * inputSize);
	printf("SHELL STARTED:\n");
	while (1) {
		printf(">> ");
		fgets(in, inputSize, stdin);
		if (in != NULL && strcmp(in, "\r") && strcmp(in, "\n"))
			parse(in);
	}
	return EXIT_SUCCESS;
};


