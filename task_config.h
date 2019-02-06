#ifndef TASK_CONFIG
#define TASK_CONFIG

#include <string>

class task_config{
public:
    int stage;
    int num_routers;

    void update_config(char*);
};
#endif
