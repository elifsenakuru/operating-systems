#ifndef eshell_H
#define eshell_H

#include "parser.h"
#include <vector>

class eshell {
public:
    void execute(parsed_input* input);
    void executeCommand(command& cmd);
    void executePipeline(parsed_input* input);
    void executeSequential(parsed_input* inputs);
    void executeParallel(parsed_input* inputs);
    void executeSubshell(char* input);
    void executePipelineInParallel(pipeline& pline);
    void executePipelineSeq(single_input* input);
    void execCom(command* cmd);

};

#endif // eshell_H

