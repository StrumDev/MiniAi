#include "Commands.h"
#include <stdio.h>
#include <string.h>
#include "ChatHistory.h"
#include "Responder.h"

#ifdef _WIN32
#include <windows.h>
#endif

enum
{
    AnswerSize = 8192
};

void CommandHelp(void)
{
    printf("Commands:\n");
    printf("  /teach                   pretrain on the .pre files of the training folder, then fine-tune on its .fin files\n");
    printf("                           (continues the saved model if the architecture is the same)\n");
    printf("  /teach pretrain          only the pretraining phase (plain text)\n");
    printf("  /teach finetune          only the fine-tuning phase (the .fin files)\n");
    printf("  /teach mixed             learn text windows and in:/out: pairs shuffled together in one pass\n");
    printf("  /teach new               train from scratch (can be combined: /teach pretrain new)\n");
    printf("  /continue <text>         let the model continue a plain text (shows what pretraining learned)\n");
    printf("  /history                 show the chat history (needs ChatHistory: 1)\n");
    printf("  /history clear           forget the chat history\n");
    printf("  /clear                   clear the screen\n");
    printf("  /config                  show the settings\n");
    printf("  /set <key> <value>       change a setting (saved to config.yml)\n");
    printf("  /help                    show this help\n");
    printf("  /exit                    quit\n");
    printf("Any other text is a question for the model.\n");
}

// The Windows console understands the escape codes only after the virtual terminal mode is switched on.
static void CommandEnableEscapeCodes(void)
{
#ifdef _WIN32
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode))
    {
        SetConsoleMode(output, mode | 0x0004);
    }
#endif
}

void CommandClear(void)
{
    CommandEnableEscapeCodes();
    printf("\033[2J\033[3J\033[H");
    fflush(stdout);
}

void CommandConfig(const Context *context)
{
    printf("Settings file: %s\n", context->configPath);
    ConfigPrint(&context->config);
    printf("Change with: /set <key> <value>. The Layers, Hidden and Embedding parameters apply after /teach new.\n");
}

void CommandSet(Context *context, const char *name, const char *value)
{
    int status = ConfigSet(&context->config, name, value);
    
    if (status == 0)
    {
        printf("Unknown parameter \"%s\". Full list: /config\n", name);
        return;
    }
    
    if (status < 0)
    {
        printf("Value \"%s\" is not allowed for parameter %s. Ranges and descriptions: /config\n", value, name);
        return;
    }
    
    printf("%s = %s\n", name, value);
    
    if (!ConfigSave(&context->config, context->configPath))
        printf("Could not write %s\n", context->configPath);
    
    if (ConfigNameEquals(name, "Layers") || ConfigNameEquals(name, "Hidden") || ConfigNameEquals(name, "Embedding"))
        printf("The new architecture will be applied on the next /teach (from scratch).\n");
   
    if (ConfigNameEquals(name, "ChatHistory"))
    {
        ChatHistoryClear(&context->history);
        
        if (context->config.chatHistory)
            printf("Chat history is on: the last %d exchanges are given to the model as context. Run /teach finetune so the model learns to use context.\n", context->config.chatHistoryTurns);
        else
            printf("Chat history is off and was cleared.\n");

    }
}

void CommandAsk(Context *context, const char *question)
{
    if (!context->modelReady)
    {
        printf("The model is not trained yet. Type /teach\n");
        return;
    }
    
    char answer[AnswerSize];
    
    if (!ResponderAnswer(&context->model, &context->config, &context->random, &context->history, question, answer, AnswerSize))
    {
        printf("Not enough memory for the answer.\n");
        return;
    }
    
    printf("MiniAi> %s\n", answer[0] != 0 ? answer : "(empty answer)");
}

void CommandHistoryShow(const Context *context)
{
    if (!context->config.chatHistory)
    {
        printf("Chat history is off. Set ChatHistory: 1 in config.yml (or /set ChatHistory 1) to turn it on.\n");
        return;
    }
   
    if (context->history.count == 0)
    {
        printf("The history is empty.\n");
        return;
    }
    
    for (int i = 0; i < context->history.count; i++)
        printf("  %d. You: %s\n     MiniAi: %s\n", i + 1, context->history.questions[i], context->history.answers[i]);
}

void CommandHistoryClear(Context *context)
{
    ChatHistoryClear(&context->history);
    printf("History cleared.\n");
}

void CommandContinue(Context *context, const char *text)
{
    if (!context->modelReady)
    {
        printf("The model is not trained yet. Type /teach\n");
        return;
    }

    char continuation[AnswerSize];
    
    if (!ResponderContinue(&context->model, &context->config, &context->random, text, continuation, AnswerSize))
    {
        printf("Not enough memory for the answer.\n");
        return;
    }
    
    printf("MiniAi> %s%s\n", text, continuation);
}