#pragma once

#include "Context.h"

enum
{
    PhasePretrain = 1,
    PhaseFinetune = 2,
    PhaseAll = 3,
    PhaseMixed = 4
};

// Runs the chosen training: pretraining on plain text, then fine-tuning on in:/out: pairs,
// or (PhaseMixed) both kinds of examples shuffled together in one pass.
// With fresh set, the model is created from scratch instead of continuing the saved one.
void CommandTeach(Context *context, int phases, int fresh);
