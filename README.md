# shell-like-program
This shell-like C program displays the command line prompt **myshell>** and waits for the user's command. The supported commands are as follows:

### Built-in commands directly implemented in the program:
- `cd <directory>` : Changes the current directory to \<directory>. 

- `pwd` : Prints the current working directory.

- `history` : Prints the 10 most recently entered commands in the shell in current session including itself.

- `exit` : Terminates the shell process.

This shell-like program considers other built-in commands as system commands. For them, creates a child process and the child process executes the command by using `execvp()` function.

### Other supported functionalities:
- **Background Processes** : When a process runs in background, the program displays the process id of the background process and prompts for another command. A background process is indicated by placing an ampersand character `&` at the end of an input line. 

    Example usage:

    ```
    gedit &
    ```

- **Pipe Operator** : Pipe operator `|` connects the `stdout` of the first command to `stdin` of the command after pipe.

    Example usage:

    ```
    ls -l | wc -l
    ```
- **Logical AND Operator** : Handles conditionally chained processes uding logical AND operator `&&`. The second command runs if and only if the first command runs successfully.

    Example usage:
    ```
    gcc main.c && ./a.out
    ```