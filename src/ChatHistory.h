#pragma once

enum
{
    ChatHistoryLimit = 256
};

// The exchanges of the current chat, oldest first. Only the newest ones are given to the model.
typedef struct ChatHistory
{
    int count;
    char **questions;
    char **answers;
} ChatHistory;

void ChatHistoryClear(ChatHistory *history);

// Copies the exchange into the history; the oldest exchange is dropped when the history is full. Returns 0 when memory runs out.
int ChatHistoryAdd(ChatHistory *history, const char *question, const char *answer);

// Writes the newest exchanges (at most maxTurns, at most budget bytes) in the form the model was trained on:
// question, Separator, answer, EndMark for every exchange, oldest first. output needs room for budget bytes.
// Returns how many bytes were written.
long ChatHistoryContext(const ChatHistory *history, int maxTurns, long budget, unsigned char *output);
