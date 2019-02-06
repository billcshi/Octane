#include <iostream>

#include "stage1main.h"
#include "../Router/Router.h"

int primary_router_port;

int stage1main(task_config *taskConfig)
{
    std::cout<<"Welcome to stage1main"<<std::endl;
    Router primary_router;
    primary_router_port=primary_router.portNumber;
    primary_router.open_log("stage1.r0.out");
    char buf[40];
    sprintf(buf,"primary port: %d\n",primary_router_port);
    primary_router.write_to_log(buf);
    return 0;
}