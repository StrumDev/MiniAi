#pragma once

#include "network/Model.h"
#include "network/Workspace.h"

// Runs one sequence (question, separator, answer, end mark) through the network.
// Adds the cross-entropy loss to the workspace and, when backward is set,
// the gradient of that summed loss with respect to every parameter.
void BackpropSequence(const Model *model, Workspace *space, const unsigned char *tokens, int length, int separator, float questionWeight, int backward);
