#pragma once

// Lowercases Latin and Cyrillic letters, turns punctuation (? ! , ; : " ( ) and the quotes « ») and sentence dots into spaces,
// and collapses runs of spaces. Characters such as + - * / = ' and the dot inside a number stay.
// The output never needs more room than the source. Returns the output length.
long NormalizeQuestion(const unsigned char *source, long length, unsigned char *output);
