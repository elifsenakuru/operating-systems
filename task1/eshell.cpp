#include "eshell.h"
#include <unistd.h>
#include <vector>
#include <sys/wait.h>
#include <iostream>
#include <array>
#include <vector>
#include <array>
#include <signal.h>

void eshell::execute(parsed_input* input) {

    //if one input it is either a command or a subshell
    if (input->num_inputs == 1) {
        if (input->inputs[0].type == INPUT_TYPE_COMMAND) {
            executeCommand(input->inputs[0].data.cmd);
        }
        else if (input->inputs[0].type == INPUT_TYPE_SUBSHELL) {
            executeSubshell(input->inputs[0].data.subshell);
        }
    }
    else {
        if (input->separator == SEPARATOR_SEQ) {
            executeSequential(input);
        }
        else if (input->separator == SEPARATOR_PARA) {
            executeParallel(input);
        }
        else if (input->separator == SEPARATOR_PIPE) {
            executePipeline(input);
        }
    }
}

void eshell::executeCommand(command& cmd) {
    //printf("Executing command: %s\n", cmd.args[0]);

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(cmd.args[0], cmd.args) == -1) {
            std::cerr << "Exec failed for command: " << cmd.args[0] << std::endl;
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) { // parent process
        int status;
        waitpid(pid, &status, 0);  // wait child
    } else {
        // Fork failed
        std::cerr << "Fork failed" << std::endl;
    }
}

void eshell::executeSequential(parsed_input* input) {
    for (int i = 0; i < input->num_inputs; i++) {
        if (input->inputs[i].type == INPUT_TYPE_COMMAND) {
            executeCommand((input->inputs[i].data.cmd));
        }
        else if (input->inputs[i].type == INPUT_TYPE_PIPELINE) {
            executePipelineSeq(&(input->inputs[i]));
        }
        else {
            std::cerr << "Unsupported input" << std::endl;
        }
    }
}

void eshell::executePipelineInParallel(pipeline& pline) {
    int num_commands = pline.num_commands;
    std::vector<std::vector<int>> pipes(num_commands - 1, std::vector<int>(2));
    std::vector<pid_t> pids(num_commands);

    // create pipes
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i].data()) == -1) {
            std::cerr << "Pipe creation failed" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // fork and execute commands
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            // set up input and output redirection
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][1]);
            }
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
            }
            for (int j = 0; j < num_commands - 1; ++j) {
                if (j != i - 1) close(pipes[j][0]);
                if (j != i) close(pipes[j][1]);
            }
            if (execvp(pline.commands[i].args[0], pline.commands[i].args) == -1) {
                std::cerr << "Exec failed for command: " << pline.commands[i].args[0] << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (pids[i] < 0) {
            std::cerr << "Fork failed" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // close all pipe ends in the parent process
    for (auto& pipe : pipes) {
        close(pipe[0]);
        close(pipe[1]);
    }

    // wait for all child processes to finish
    for (auto& pid : pids) {
        waitpid(pid, nullptr, 0);
    }
}

void eshell::execCom(command *cmd) {
    if (execvp(cmd->args[0], cmd->args) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}

void eshell::executePipelineSeq(single_input *segment) {
    int num_commands = segment->data.pline.num_commands;
    int stdin_fd = 0;
    int fd[2];

    for (int i = 0; i < num_commands; i++) {
        if (i < num_commands - 1) {
            if (pipe(fd) == -1) {
                perror("pipe error");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork error");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (stdin_fd != 0) {
                dup2(stdin_fd, STDIN_FILENO);
                close(stdin_fd);
            }
            if (i < num_commands - 1) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
            } else {
                if (num_commands > 1) {
                    close(fd[1]);
                }
            }
            execCom(&(segment->data.pline.commands[i]));
            exit(EXIT_FAILURE);
        } else {
            if (stdin_fd != 0) {
                close(stdin_fd);
            }
            if (i < num_commands - 1) {
                close(fd[1]);
                stdin_fd = fd[0];
            } else {
                if (segment->data.pline.num_commands > 1) {
                    close(fd[0]);
                }
            }
        }
    }
    while (wait(NULL) > 0);
}

void eshell:: executeParallel(parsed_input *input) {
    int num_commands = input->num_inputs;
    pid_t pids[num_commands];

    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork");
        } else if (pids[i] == 0) {

            if (input->inputs[i].type == INPUT_TYPE_COMMAND) {
                execCom(&(input->inputs[i].data.cmd));
            }
            else if (input->inputs[i].type == INPUT_TYPE_PIPELINE) {
                executePipelineSeq(&(input->inputs[i]));
            }
            else {
                std::cerr << "Unsupported input type in parallel execution.\n" << std::endl;
            }
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_commands; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], NULL, 0);
        }
    }
}

void eshell::executePipeline(parsed_input *segment) {
    int num_commands = segment->num_inputs;
    std::vector<std::vector<int>> pipes(num_commands - 1, std::vector<int>(2));

    for (int i = 0; i < num_commands - 1; ++i) {
        if (pipe(pipes[i].data()) == -1) {
            std::cerr << "Pipe creation failed" << std::endl;
            return;
        }
    }

    // fork and execute commands or subshells
    std::vector<pid_t> pids;
    for (int i = 0; i < num_commands; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][1]);
            }
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
            }
            for (int j = 0; j < num_commands - 1; ++j) {
                if (j != i - 1) close(pipes[j][0]);
                if (j != i) close(pipes[j][1]);
            }

            if (segment->inputs[i].type == INPUT_TYPE_COMMAND) {
                //printf("Executing command, called from executePipeline\n");
                if (execvp(segment->inputs[i].data.cmd.args[0], segment->inputs[i].data.cmd.args) == -1) {
                    std::cerr << "Exec failed for command: " << segment->inputs[i].data.cmd.args[0] << std::endl;
                    exit(EXIT_FAILURE);
                }
            } else if (segment->inputs[i].type == INPUT_TYPE_SUBSHELL) {
                //printf("Executing subshell, called from executePipeline\n");
                executeSubshell(segment->inputs[i].data.subshell);
            }

            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            pids.push_back(pid);
        } else {
            std::cerr << "Fork failed" << std::endl;
            return;
        }
    }

    for (auto& pipe : pipes) {
        close(pipe[0]);
        close(pipe[1]);
    }

    for (auto& pid : pids) {
        waitpid(pid, nullptr, 0);
    }
}

void eshell::executeSubshell(char *subshell_cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        parsed_input subshell_input;
        if (parse_line(subshell_cmd, &subshell_input)) {
            if (subshell_input.separator == SEPARATOR_PARA) {
                //printf("Executing subshell with parallel commands\n");
                std::vector<std::array<int, 2>> pipes(subshell_input.num_inputs);
                std::vector<pid_t> child_pids;

                for (int i = 0; i < subshell_input.num_inputs; i++) {
                    if (pipe(pipes[i].data()) == -1) {
                        perror("pipe");
                        exit(EXIT_FAILURE);
                    }

                    pid_t cpid = fork();
                    if (cpid == 0) {
                        close(pipes[i][1]);
                        dup2(pipes[i][0], STDIN_FILENO);
                        close(pipes[i][0]);
                        if (subshell_input.inputs[i].type == INPUT_TYPE_COMMAND) {
                            execCom(&(subshell_input.inputs[i].data.cmd));
                        } else if (subshell_input.inputs[i].type == INPUT_TYPE_PIPELINE) {
                            executePipelineInParallel(subshell_input.inputs[i].data.pline);
                        }
                        exit(EXIT_SUCCESS);
                    } else {
                        close(pipes[i][0]);
                        child_pids.push_back(cpid);
                    }
                }

                //repeater process
                pid_t repeater_pid = fork();
                if (repeater_pid > 0) {
                    signal(SIGPIPE, SIG_IGN);
                    char buffer[256*1024];
                    ssize_t bytes_read;

                    // read from stdin and write to each pipe
                    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
                        for (int i = 0; i < subshell_input.num_inputs; i++) {
                            write(pipes[i][1], buffer, bytes_read);
                        }
                    }

                    for (int i = 0; i < subshell_input.num_inputs; i++) {
                        close(pipes[i][1]);
                    }
                    exit(EXIT_SUCCESS);
                } else {
                    child_pids.push_back(repeater_pid);
                }

                for (pid_t cpid : child_pids) {
                    waitpid(cpid, nullptr, 0);
                }
                exit(EXIT_SUCCESS);
            } else {
                execute(&subshell_input);
                free_parsed_input(&subshell_input);
            }
        } else {
            std::cerr << "Error parsing subshell input." << std::endl;
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        waitpid(pid, nullptr, 0);
    } else {
        std::cerr << "Fork failed for subshell" << std::endl;
    }
}