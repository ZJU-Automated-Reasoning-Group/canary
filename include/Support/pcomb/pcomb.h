#ifndef PCOMB_MAIN_HEADER_H
#define PCOMB_MAIN_HEADER_H

// This is a header that pulls in all the headers for parsers and combinators
#include "Support/pcomb/Parser/PredicateCharParser.h"
#include "Support/pcomb/Parser/RegexParser.h"
#include "Support/pcomb/Parser/StringParser.h"

#include "Support/pcomb/Combinator/AltParser.h"
#include "Support/pcomb/Combinator/SeqParser.h"
#include "Support/pcomb/Combinator/ManyParser.h"
#include "Support/pcomb/Combinator/TokenParser.h"
#include "Support/pcomb/Combinator/ParserAdapter.h"
#include "Support/pcomb/Combinator/LazyParser.h"
#include "Support/pcomb/Combinator/LexemeParser.h"

#endif
