#include "chat/ChatHistory.h"
#include <stdlib.h>
#include <string.h>
#include "text/Tokens.h"

void ChatHistoryClear(ChatHistory *history)
{
    for (int i = 0; i < history->count; i++)
    {
        free(history->questions[i]);
        free(history->answers[i]);
    }

    free(history->questions);
    free(history->answers);

    memset(history, 0, sizeof(ChatHistory));
}

static char *ChatHistoryCopy(const char *text)
{
    size_t length = strlen(text);
    char *copy = (char *)malloc(length + 1);

    if (copy != 0)
        memcpy(copy, text, length + 1);

    return copy;
}

int ChatHistoryAdd(ChatHistory *history, const char *question, const char *answer)
{
    if (history->questions == 0)
    {
        history->questions = (char **)calloc(ChatHistoryLimit, sizeof(char *));
        history->answers = (char **)calloc(ChatHistoryLimit, sizeof(char *));
        
        if (history->questions == 0 || history->answers == 0)
        {
            ChatHistoryClear(history);
            return 0;
        }
    }
    
    char *questionCopy = ChatHistoryCopy(question);
    char *answerCopy = ChatHistoryCopy(answer);
    
    if (questionCopy == 0 || answerCopy == 0)
    {
        free(questionCopy);
        free(answerCopy);
        return 0;
    }

    if (history->count == ChatHistoryLimit)
    {
        free(history->questions[0]);
        free(history->answers[0]);
        
        memmove(history->questions, history->questions + 1, (ChatHistoryLimit - 1) * sizeof(char *));
        memmove(history->answers, history->answers + 1, (ChatHistoryLimit - 1) * sizeof(char *));
        
        history->count--;
    }

    history->questions[history->count] = questionCopy;
    history->answers[history->count] = answerCopy;
    history->count++;
    
    return 1;
}

long ChatHistoryContext(const ChatHistory *history, int maxTurns, long budget, unsigned char *output)
{
    int first = history->count;
    long used = 0;

    while (first > 0 && history->count - first < maxTurns)
    {
        long turn = (long)strlen(history->questions[first - 1]) + (long)strlen(history->answers[first - 1]) + 2;
        if (used + turn > budget)
            break;

        used += turn;
        first--;
    }

    long written = 0;

    for (int i = first; i < history->count; i++)
    {
        size_t questionLength = strlen(history->questions[i]);
        size_t answerLength = strlen(history->answers[i]);
        
        memcpy(output + written, history->questions[i], questionLength);
        
        written += (long)questionLength;
        output[written++] = Separator;
        
        memcpy(output + written, history->answers[i], answerLength);
        
        written += (long)answerLength;
        output[written++] = EndMark;
    }

    return written;
}