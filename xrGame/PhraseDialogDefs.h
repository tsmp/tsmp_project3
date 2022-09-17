#pragma once

class CPhraseDialog;
typedef intrusive_ptr<CPhraseDialog> DIALOG_SHARED_PTR;

DEFINE_VECTOR(shared_str, DIALOG_ID_VECTOR, DIALOG_ID_IT);

#include "PhraseDialog.h"
