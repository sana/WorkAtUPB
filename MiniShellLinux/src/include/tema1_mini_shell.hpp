/*
 * @author Dascalu Laurentiu, 335CA.
 * @brief Mini Shell header file
 */

#ifndef TEMA1MINISHELL_HPP_
#define TEMA1MINISHELL_HPP_

#include <iostream>
#include <string>
#include <cstdlib>
#include <cassert>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <queue>
#include <string.h>
#include <stack>
#include <fstream>

#include "parser.h"

#define HEADER "Mini-Shell v1.0\nAuthor: Dascalu Laurentiu\n"

#define PROMPT_STRING	"> "
#define EVARS_DIM		":"

using namespace std;

class TreeTraveler;
class CommandExecutor;
class CommandsHandler;
class MyInternalCommand;

class TreeTraveler
{
public:
	TreeTraveler();
	~TreeTraveler();

	void set_root(command_t *root);
	void start_tree_travel();

private:
	int display_command(command_t * c, command_t * father);
	void display_simple(simple_command_t * s, command_t * father);
	vector<string> *display_word(word_t * w);
	vector<vector<string> *> *display_list(word_t * w);

	command_t *m_root;
	CommandExecutor *exe;
};

class CommandExecutor
{
private:

	vector<string> *flatten(vector<vector<string> *> *data);
	string *flatten(vector<string> *data);

public:
	CommandExecutor();
	~CommandExecutor();

	/*
	 * Executa o comanda simpla si intoarce rezultatul.
	 */
	int execute_simple_command(MyInternalCommand *com);

	/*
	 * Executa comenzile insiruite de pipe si intoarce codul de exit al ultimei comenzi.
	 */
	int execute_pipe_commands();

	static void redirect(int oldfd, int newfd);
};

class CommandsHandler
{
public:
	static void init();
	static void finish();

	static void
			add_command_stack(vector<vector<string> *> *command, vector<vector<
					string> *> *params, vector<vector<string> *> *in, vector<
					vector<string> *> *out, vector<vector<string> *> *err,
					int io_flags);

	static stack<MyInternalCommand *> *get_stack();

private:
	/*
	 * Commands stack.
	 */
	static stack<MyInternalCommand *> *sc;
};

class MyInternalCommand
{
public:
	vector<vector<string> *> *command;
	vector<vector<string> *> *params;
	vector<vector<string> *> *in;
	vector<vector<string> *> *out;
	vector<vector<string> *> *err;
	int io_flags;

			MyInternalCommand(vector<vector<string> *> *command, vector<vector<
					string> *> *params, vector<vector<string> *> *in, vector<
					vector<string> *> *out, vector<vector<string> *> *err,
					int io_flags);
	~MyInternalCommand();
};

#endif /* TEMA1MINISHELL_HPP_ */
