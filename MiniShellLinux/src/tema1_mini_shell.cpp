/*!
 * @author Dascalu Laurentiu, 335CA
 * @brief Mini shell, Tema 1 Sisteme de operare.
 */
#include "include/tema1_mini_shell.hpp"

void parse_error(const char * str, const int where)
{
	fprintf(stderr, "Parse error near %d: %s\n", where, str);
}

int main()
{
	TreeTraveler worker;

	CommandsHandler::init();

	for (;;)
	{
		cout << PROMPT_STRING;
		cout.flush();

		string line;
		getline(cin, line);

		if (line == "exit" || line == "quit")
			break;

		if ((line.length() == 0) && !cin.good())
		{
			// end of file reached
			cerr << "End of file!" << endl;
			return EXIT_SUCCESS;
		}

		command_t* root = NULL;
		if (parse_line(line.c_str(), &root))
		{
			//cout << "Command successfully read!" << endl;
			if (root == NULL)
			{
				cout << "Command is empty!" << endl;
			}
			else
			{
				//root points to a valid command tree that we can use
				worker.set_root(root);
				worker.start_tree_travel();
			}
		}
		else
		{
			//there was an error parsing the command
		}
		free_parse_memory();
	}

	CommandsHandler::finish();

	return EXIT_SUCCESS;
}

TreeTraveler::TreeTraveler()
{
	exe = new CommandExecutor();
}

TreeTraveler::~TreeTraveler()
{
	delete exe;
}

void TreeTraveler::set_root(command_t *root)
{
	m_root = root;
}

void TreeTraveler::start_tree_travel()
{
	display_command(m_root, NULL);
}

int TreeTraveler::display_command(command_t * c, command_t * father)
{
	int result = 0;
	assert(c != NULL);
	assert(c->up == father);

	if (c->op == OP_NONE)
	{
		assert(c->cmd1 == NULL);
		assert(c->cmd2 == NULL);
		display_simple(c->scmd, c);

		/*
		 * Acum pot executa comanda.
		 */
		MyInternalCommand *command = CommandsHandler::get_stack()->top();
		result = exe->execute_simple_command(command);
		CommandsHandler::get_stack()->pop();

		return result;
	}
	else
	{
		assert(c->scmd == NULL);

		switch (c->op)
		{
		case OP_SEQUENTIAL:
			display_command(c->cmd1, c);
			return display_command(c->cmd2, c);
		case OP_PARALLEL:
		{
			int cpid1 = -1, cpid2 = -1, status1 = 0, status2 = 0;

			switch (cpid1 = fork())
			{
			/*
			 * Atat in cazul executiei comenzilor in paralel cat si in
			 * cazul comenzilor redirectate ( pipe ) numarul de fork-uri este mai mare
			 * decat numarul de exec-uri. Acest lucru imi permite executia comenzii c1 | c2
			 * astfel : c1 in copilul copilui si c2 in copil, urmand ca procesul curent
			 * sa primeasca ca si rezultat : rezultat(c1) | rezultat(c2)
			 * */
			case -1:
				printf("fork() failed !\n");
				exit(-1);
			case 0:
			{
				switch (cpid2 = fork())
				{
				case -1:
					printf("fork() failed !\n");
					exit(-1);
				case 0:
					exit(display_command(c->cmd1, c));
				default:
					result = display_command(c->cmd2, c);
					waitpid(cpid2, &status2, 0);
					exit(result | status2);
				}
			}
			default:
				waitpid(cpid1, &status1, 0);
			}
			return status1;
		}
		case OP_CONDITIONAL_ZERO:
			result = display_command(c->cmd1, c);

			if (!result)
				result = display_command(c->cmd2, c);

			return result;
		case OP_CONDITIONAL_NZERO:
			result = display_command(c->cmd1, c);

			if (result)
				result = display_command(c->cmd2, c);

			return result;
		case OP_PIPE:
		{
			int cpid1 = -1, cpid2 = -1, status1 = 0;
			int fds[2];

			switch (cpid1 = fork())
			{
			case -1:
				printf("fork() failed !\n");
				exit(-1);
			case 0:
			{
				if (pipe(fds) == -1)
				{
					fprintf(stderr, "pipe() failed !\n");
					exit(1);
				}
				// Vezi explicatiile de la pipe referitoare la numarul de fork-uri.
				switch (cpid2 = fork())
				{
				case -1:
					printf("fork() failed !\n");
					exit(-1);
				case 0:
					close(fds[0]);
					CommandExecutor::redirect(fds[1], STDOUT_FILENO);
					close(fds[1]);
					exit(display_command(c->cmd1, c));
				default:
					close(fds[1]);
					CommandExecutor::redirect(fds[0], STDIN_FILENO);
					close(fds[0]);
					result = display_command(c->cmd2, c);
					exit(result);
				}
			}
			default:
				waitpid(cpid1, &status1, 0);
			}
			return status1;
		}
		default:
			assert(false);
		}
	}
	return 0;
}

vector<string> *TreeTraveler::display_word(word_t * w)
{
	assert(w != NULL);
	word_t * crt = w;
	char *aux;
	vector<string> *word = new vector<string> ();

	while (crt != NULL)
	{
		if (crt->expand)
		{
			if ((aux = getenv(crt->string)) == NULL)
				word->push_back("");
			else
				word->push_back(aux);
		}
		else
			word->push_back(crt->string);

		crt = crt->next_part;
		if (crt != NULL)
			assert(crt->next_word == NULL);
	}
	return word;
}

vector<vector<string> *> *TreeTraveler::display_list(word_t * w)
{
	assert(w != NULL);
	word_t * crt = w;
	vector<vector<string> *> *list = new vector<vector<string> *> ();

	while (crt != NULL)
	{
		list->push_back(display_word(crt));
		crt = crt->next_word;
	}

	return list;
}

void TreeTraveler::display_simple(simple_command_t * s, command_t * father)
{
	assert(s != NULL);
	assert(father == s->up);
	vector<vector<string> *> *params = NULL, *in = NULL, *out = NULL, *err =
			NULL, *command = NULL;

	if (s->params != NULL)
		params = display_list(s->params);

	if (s->in != NULL)
		in = display_list(s->in);

	if (s->out != NULL)
		out = display_list(s->out);

	if (s->err != NULL)
		err = display_list(s->err);

	command = display_list(s->verb);

	if (params == NULL)
		params = new vector<vector<string> *> ();
	if (in == NULL)
		in = new vector<vector<string> *> ();
	if (out == NULL)
		out = new vector<vector<string> *> ();
	if (err == NULL)
		err = new vector<vector<string> *> ();

	CommandsHandler::add_command_stack(command, params, in, out, err,
			s->io_flags);
}

CommandExecutor::CommandExecutor()
{

}

CommandExecutor::~CommandExecutor()
{

}

void CommandExecutor::redirect(int oldfd, int newfd)
{
	/*
	 * Ignor cererea de redirectare pentru un descriptor invalid.
	 */
	if (newfd < 0)
		return;

	if (dup2(oldfd, newfd) == -1)
	{
		printf("Dup2 failed !");
		exit(1);
	}
}

vector<string> *CommandExecutor::flatten(vector<vector<string> *> *data)
{
	vector<string> *str = new vector<string> ();

	for (unsigned int i = 0; i < data->size(); i++)
	{
		vector<string> *v = (*data)[i];
		string aux;

		for (unsigned int j = 0; j < v->size(); j++)
			aux += (*v)[j];
		str->push_back(aux);
		delete v;
	}

	return str;
}

string *CommandExecutor::flatten(vector<string> *data)
{
	string *str = new string();

	for (unsigned int i = 0; i < data->size(); i++)
		(*str) += (*data)[i];

	return str;
}

int CommandExecutor::execute_simple_command(MyInternalCommand *com)
{
	int status, append_flag_out = 0, append_flag_err = 0, open_flag = 0;
	pid_t cpid;
	vector<string> *__aux = flatten(com->command);
	string *load = flatten(__aux);
	vector<string> *params = flatten(com->params);
	delete __aux;
	unsigned int pos;

	if (*load == "cd")
	{
		string *_params = flatten(params);
		int result = chdir(_params->data());
		delete load;
		delete params;
		delete _params;
		return result;
	}
	else if (*load == "true")
	{
		delete params;
		delete load;
		return 0;
	}
	else if (*load == "false")
	{
		delete params;
		delete load;
		return 1;
	}
	else if ((pos = load->find('=', 0)) != string::npos)
	{
		string value(load->substr(pos + 1));
		string name(load->substr(0, pos));
		delete params;
		delete load;
		return setenv(name.data(), value.data(), 1);
	}
	else
	{
		if (com->io_flags & IO_OUT_APPEND)
			append_flag_out = 1;
		if (com->io_flags & IO_ERR_APPEND)
			append_flag_err = 1;

		switch (cpid = fork())
		{
		case -1:
			fprintf(stderr, "fork failed!\n");
			exit(-1);
		case 0:
		{
			const char *args[100] =
			{ NULL};

			for (unsigned int i = 0; i < params->size(); i++)
				args[i + 1] = strdup((*params)[i].data());

			/*
			 * Redirectari :
			 * stdin
			 * stdout
			 * stderr
			 */
			string *in = flatten(flatten(com->in));
			string *out = flatten(flatten(com->out));
			string *err = flatten(flatten(com->err));

			if (*in != "")
				redirect(open(in->data(), O_RDONLY, 0644), STDIN_FILENO);

			int fdout = -1, fderr = -1;

			/*
			 * Deschid fisierele de I/O cu argumente corespunzatoare : append, read, write etc.
			 */
			if (*out != "")
			{
				open_flag = O_CREAT | O_WRONLY;
				if (!append_flag_out)
					open_flag |= O_TRUNC;
				else
					open_flag |= O_APPEND;
				fdout = open(out->data(), open_flag, 0644);
				redirect(fdout, STDOUT_FILENO);
			}

			if (*err != "")
			{
				open_flag = O_CREAT | O_WRONLY;
				if (!append_flag_err)
					open_flag |= O_TRUNC;
				else
					open_flag |= O_APPEND;
				if (*err == *out)
					fderr = fdout;
				else
					fderr = open(err->data(), open_flag, 0644);
				redirect(fderr, STDERR_FILENO);
			}

			args[0] = strdup(load->data());

			execvp(load->data(), (char* const *) args);
			fprintf(stderr, "Execution failed for '%s'\n", load->data());
			delete in;
			delete out;
			delete err;
			exit(-1);
		}
		default:
			waitpid(cpid, &status, 0);
			delete com;
			delete load;
			delete params;
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			return -1;
		}
	}

}

MyInternalCommand::MyInternalCommand(vector<vector<string> *> *command, vector<
		vector<string> *> *params, vector<vector<string> *> *in, vector<vector<
		string> *> *out, vector<vector<string> *> *err, int io_flags)
{
	this->command = command;
	this->params = params;
	this->in = in;
	this->out = out;
	this->err = err;
	this->io_flags = io_flags;
}

MyInternalCommand::~MyInternalCommand()
{
	delete command;
	delete params;
	delete in;
	delete out;
	delete err;
}

stack<MyInternalCommand *> *CommandsHandler::sc;

void CommandsHandler::init()
{
	sc = new stack<MyInternalCommand *> ();
}

void CommandsHandler::finish()
{
	while (sc->size() > 0)
	{
		delete sc->top();
		sc->pop();
	}

	delete sc;
}

void CommandsHandler::add_command_stack(vector<vector<string> *> *command,
		vector<vector<string> *> *params, vector<vector<string> *> *in, vector<
				vector<string> *> *out, vector<vector<string> *> *err,
		int io_flags)
{
	sc->push(new MyInternalCommand(command, params, in, out, err, io_flags));
}

stack<MyInternalCommand *> *CommandsHandler::get_stack()
{
	return sc;
}
