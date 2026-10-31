#include "Console.h"
#include <stdio.h>
#include <string.h>
#include "Commands.h"
#include "TeachCommand.h"

enum
{
    LineSize = 4096
};

static void ConsoleTrim(char *line)
{
    size_t length = strlen(line);
    
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r' || line[length - 1] == ' ' || line[length - 1] == '\t'))
        line[--length] = 0;
}

static char *ConsoleToken(char **cursor)
{
    char *start = *cursor;
    
    while (*start == ' ' || *start == '\t')
        start++;

    if (*start == 0)
    {
        *cursor = start;
        return 0;
    }

    char *end = start;
    
    while (*end != 0 && *end != ' ' && *end != '\t')
        end++;

    if (*end != 0)
    {
        *end = 0;
        end++;
    }
    
    *cursor = end;
    return start;
}

// Understands: /teach, /teach pretrain, /teach finetune, /teach mixed, /teach new, and combinations such as /teach finetune new.
static void ConsoleTeach(Context *context, char *cursor)
{
    int phases = 0;
    int fresh = 0;
    char *argument = ConsoleToken(&cursor);

    while (argument != 0)
    {
        if (strcmp(argument, "new") == 0)
        {
            fresh = 1;
        }
        else if (strcmp(argument, "pretrain") == 0)
        {
            phases |= PhasePretrain;
        }
        else if (strcmp(argument, "finetune") == 0)
        {
            phases |= PhaseFinetune;
        }
        else if (strcmp(argument, "mixed") == 0)
        {
            phases |= PhaseMixed;
        }
        else
        {
            printf("Unknown option \"%s\". Usage: /teach [pretrain] [finetune] [mixed] [new]\n", argument);
            return;
        }

        argument = ConsoleToken(&cursor);
    }
   
    if ((phases & PhaseMixed) && (phases & PhaseAll))
    {
        printf("\"mixed\" already includes both kinds of data and cannot be combined with pretrain or finetune.\n");
        return;
    }
    
    CommandTeach(context, phases == 0 ? PhaseAll : phases, fresh);
}

// Returns 0 when the program should end.
static int ConsoleCommand(Context *context, char *line)
{
    char *cursor = line;
    char *command = ConsoleToken(&cursor);
    
    if (strcmp(command, "/exit") == 0 || strcmp(command, "/quit") == 0)
    {
        return 0;
    }
    
    if (strcmp(command, "/help") == 0)
    {
        CommandHelp();
    }
    else if (strcmp(command, "/clear") == 0)
    {
        CommandClear();
    }
    else if (strcmp(command, "/config") == 0)
    {
        CommandConfig(context);
    }
    else if (strcmp(command, "/teach") == 0)
    {
        ConsoleTeach(context, cursor);
    }
    else if (strcmp(command, "/continue") == 0)
    {
        while (*cursor == ' ' || *cursor == '\t')
            cursor++;
        
        if (*cursor == 0)
            printf("Usage: /continue <text>. Example: /continue Kyiv is\n");
        else
            CommandContinue(context, cursor);
    }
    else if (strcmp(command, "/history") == 0)
    {
        char *argument = ConsoleToken(&cursor);
        
        if (argument == 0)
            CommandHistoryShow(context);
        
        else if (strcmp(argument, "clear") == 0)
            CommandHistoryClear(context);
        
        else
            printf("Usage: /history or /history clear\n");
    }
    else if (strcmp(command, "/set") == 0)
    {
        char *name = ConsoleToken(&cursor);
        char *value = ConsoleToken(&cursor);
        
        if (name == 0 || value == 0)
            printf("Usage: /set <key> <value>. Example: /set FinetuneLearningRate 0.002\n");
        else
            CommandSet(context, name, value);
    }
    else
        printf("Unknown command %s. List of commands: /help\n", command);

    return 1;
}

void ConsoleRun(Context *context)
{
    char line[LineSize];

    while (1)
    {
        printf("You> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == 0)
            break;

        ConsoleTrim(line);

        if (line[0] == 0)
            continue;

        if (line[0] == '/')
        {
            if (!ConsoleCommand(context, line))
                break;
        }
        else
        {
            CommandAsk(context, line);
        }
    }
}
