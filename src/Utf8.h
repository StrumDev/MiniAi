#pragma once

// Replaces every byte that is not part of a valid UTF-8 sequence with '?'.
void Utf8Sanitize(char *text);
