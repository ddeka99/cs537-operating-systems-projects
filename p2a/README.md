# CS 537 Project 2A: Shell (Linux)

Project description can be found [here](https://pages.cs.wisc.edu/~remzi/Classes/537/Fall2021/Projects/p2a.html)

## Building and Testing

- Run `make` to build the project.
- Results from running the testsuite can be found in `tests-out` directory.

## Intro

- `fork()`, `exec()` and `wait()` system calls are used to handle non built-in commands and `loop` built-in command.
- `dup2()` system call is used to handle redirection by modifying the file descriptors.
- Only input redirection is supported in the implemented shell.
- Piping is not supported.

## Function descriptions

- Functions created to implement the various functionalities of the above mentioned shell are described below.

### int main(int argc, char **argv)

- Driver function.
- Identify mode of operation from command line arguments.
- Initialize `PATH`.
- Start executing input lines if batch mode.
- Prompt message after every execution if interactive mode.
- Handle whitespaces in input lines.
- Split input line into command and arguments.
- Check if command is built-in.
- If built-in command, run directly.
- If non built-in command
    - Check if command present in `PATH`. Continue if present.
    - Check for input redirection (if any).
    - Run non built-in command in a child process creating a separate environment.
    - Handle redirection using `dup2` system call.
    - Return to parent when execution of child process is over.
- Repeat

### void PrintError()

- Prints error message into `STDERR`

### char *ReadLine()

- Reserve memory for input string.
- Read line from `STDIN`

### char **SplitLine(char *line)

- Extract arguments from input line by tokenizing it.
- Handle input redirection.
- Keep track of number of tokens.

### char *TrimWhiteSpace(char *str)

- Trim leading space in input string.
- Trim trailing space in input string.
- Handle case if all spaces.

### int IsBuiltInCommand(char *cmd)

- Check if command is built-in from the list of built-in commands.
- Return the corresponding built-in command number.

### int ExecuteBuiltInCommand(char *p_cmd, char **dp_args, char **dp_pathDir, int cmdNo)

- Implement `exit`
- Implement `cd`
- Implement `path`
- Implement `loop`
- `loop` is implemented using multiple child processes.