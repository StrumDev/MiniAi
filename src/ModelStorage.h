#pragma once

#include "Model.h"

int ModelStorageSave(const Model *model, const char *path);
int ModelStorageLoad(Model *model, const char *path);
