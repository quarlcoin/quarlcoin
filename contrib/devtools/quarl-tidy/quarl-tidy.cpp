// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#include "nontrivial-threadlocal.h"

#include <clang-tidy/ClangTidyModule.h>

class QuarlcoinModule final : public clang::tidy::ClangTidyModule
{
public:
    void addCheckFactories(clang::tidy::ClangTidyCheckFactories& CheckFactories) override
    {
        CheckFactories.registerCheck<quarlcoin::NonTrivialThreadLocal>("quarlcoin-nontrivial-threadlocal");
    }
};

static clang::tidy::ClangTidyModuleRegistry::Add<QuarlcoinModule>
    X("quarlcoin-module", "Adds quarlcoin checks.");

volatile int QuarlcoinModuleAnchorSource = 0;
