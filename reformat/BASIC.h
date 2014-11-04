/*
 * CiderPress
 * Copyright (C) 2007, 2008 by faddenSoft, LLC.  All Rights Reserved.
 * See the file LICENSE for distribution terms.
 */
/*
 * Reformat BASIC programs.
 */
#ifndef __LR_BASIC__
#define __LR_BASIC__

#include "ReformatBase.h"

/*
 * Reformat an Applesoft BASIC program into readable text.
 */
class ReformatApplesoft : public ReformatText {
public:
    ReformatApplesoft(void) {}
    virtual ~ReformatApplesoft(void) {}

    virtual void Examine(ReformatHolder* pHolder);
    virtual int Process(const ReformatHolder* pHolder,
        ReformatHolder::ReformatID id, ReformatHolder::ReformatPart part,
        ReformatOutput* pOutput);

    /* share our token list with others */
    enum { kTokenLen = 8, kTokenCount = 107 };
    static const char* GetApplesoftTokens(void);
};

/*
 * Reformat an Integer BASIC program into readable text.
 */
class ReformatInteger : public ReformatText {
public:
    ReformatInteger(void) {}
    virtual ~ReformatInteger(void) {}

    virtual void Examine(ReformatHolder* pHolder);
    virtual int Process(const ReformatHolder* pHolder,
        ReformatHolder::ReformatID id, ReformatHolder::ReformatPart part,
        ReformatOutput* pOutput);
};

/*
 * Reformat an Apple /// Business BASIC program into readable text.
 */
class ReformatBusiness : public ReformatText {
public:
    ReformatBusiness(void) {}
    virtual ~ReformatBusiness(void) {}

    virtual void Examine(ReformatHolder* pHolder);
    virtual int Process(const ReformatHolder* pHolder,
        ReformatHolder::ReformatID id, ReformatHolder::ReformatPart part,
        ReformatOutput* pOutput);

    /* share our token list with others - but this won't really work in its current form... */
    enum { kTokenLen = 10, kTokenCount = 107 };
    static const char* GetBusinessTokens(void);
};

#endif /*__LR_BASIC__*/