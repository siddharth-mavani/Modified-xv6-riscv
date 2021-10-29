#include "../kernel/types.h"
#include "../kernel/param.h"
#include "../kernel/stat.h"
#include "./user.h"

int is_num(char s[])
{
    for(int i = 0; s[i] != '\0' ; i++)
    {
        if(s[i] < '0' || s[i] > '9')
        {
            return 0;
        }
    }

    return 1;
}


int main(int argc, char*argv[])
{
    char *nargv[MAXARG];

    int check;
    check = is_num(argv[1]);
    
    // checking for incorrect input
    if(argc < 3 || check == 0)
    {
        fprintf(2, "Incorrect Input !\n");
        fprintf(2, "Correct usage: strace <mask> <command>\n");
        exit(1);
    }

    int temp, temp_num;
    temp_num = atoi(argv[1]);
    temp = trace(temp_num);

    if(temp < 0)
    {
        fprintf(2, "strace: trace failed\n");
    }

    // copying the command
    for(int i = 2; i < argc && i < MAXARG; i++)
    {
        nargv[i - 2] = argv[i];
    }

    exec(nargv[0], nargv);   // executing command

    return 0;
}