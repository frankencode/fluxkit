/*
 * AbnfCompiler.cpp -- ABNF compiler according to RFC5434 (former RFC4234, RFC2234)
 *
 * Copyright (c) 2007-2010, Frank Mertens
 *
 * See ../LICENSE for the license.
 */

#include "SyntaxDebugger.hpp"
#include "AbnfCompiler.hpp"

namespace ftl
{

AbnfCompiler::AbnfCompiler()
{
	OPTION("caseSensitive", false);
	
	DEFINE_VOID("ALPHA",
		CHOICE(
			RANGE('a', 'z'), // 0x61 - 0x7A
			RANGE('A', 'Z')  // 0x41 - 0x5A
		)
	);
	
	DEFINE_VOID("DIGIT",
		RANGE('0', '9') // 0x30 - 0x39
	);
	
	DEFINE_VOID("HEXDIG",
		CHOICE(
			INLINE("DIGIT"),
			CHOICE(
				RANGE('a', 'f'), // 0x61 - 0x66
				RANGE('A', 'F')  // 0x41 - 0x46
			)
		)
	);
	
	DEFINE_VOID("BIT", RANGE("01"));  // 0x30 or 0x31
	DEFINE_VOID("WSP", RANGE(" \t")); // 0x20 or 0x09, whitespace
	
	DEFINE_VOID("CHAR",  RANGE(0x01, 0x7F)); // any 7-bit US-ASCII character
	DEFINE_VOID("VCHAR", RANGE(0x21, 0x7E)); // visible (printing) characters
	
	DEFINE_VOID("CR",     CHAR('\r')); // 0x0D
	DEFINE_VOID("LF",     CHAR('\n')); // 0x0A
	DEFINE_VOID("HTAB",   CHAR('\t')); // 0x09
	DEFINE_VOID("SP",     CHAR(' '));  // 0x20
	DEFINE_VOID("DQUOTE", CHAR('"'));  // 0x22
	
	DEFINE_VOID("CRLF",
		GLUE(
			REPEAT(0, 1, INLINE("CR")), // minor deviation from RFC5234
			INLINE("LF")
		)
	);
	
	DEFINE_VOID("CTL",
		CHOICE(
			RANGE(0x00, 0x1F),
			CHAR(0x7F)
		)
	);
	
	DEFINE_VOID("LWSP",
		REPEAT(
			CHOICE(
				INLINE("WSP"),
				GLUE(
					INLINE("CRLF"),
					INLINE("WSP")
				)
			)
		)
	);
	
	DEFINE_VOID("comment",
		GLUE(
			CHAR(';'),
			REPEAT(
				CHOICE(
					INLINE("WSP"),
					INLINE("VCHAR")
				)
			),
			INLINE("CRLF")
		)
	);
	
	DEFINE_VOID("c-nl",
		CHOICE(
			INLINE("comment"),
			INLINE("CRLF")
		)
	);
	
	DEFINE_VOID("c-wsp",
		GLUE(
			REPEAT(0, 1, INLINE("c-nl")),
			INLINE("WSP")
		)
	);
	
	numVal_ =
		DEFINE("num-val",
			GLUE(
				CHAR('%'),
				CHOICE(
					GLUE(RANGE("bB"), defineValue("BIT")),
					GLUE(RANGE("dD"), defineValue("DIGIT")),
					GLUE(RANGE("xX"), defineValue("HEXDIG"))
				)
			)
		);
	
	charVal_ =
		DEFINE("char-val",
			GLUE(
				CHAR('"'),
				REPEAT(OTHER('"')), // deviation from RFC5234
				CHAR('"')
			)
		);
	
	proseVal_ =
		DEFINE("prose-val",
			GLUE(
				CHAR('<'),
				REPEAT(OTHER('>')), // deviation from RFC5234
				CHAR('>')
			)
		);
	
	group_ =
		DEFINE("group",
			GLUE(
				CHAR('('),
				REPEAT(INLINE("c-wsp")),
				REF("alternation"),
				REPEAT(INLINE("c-wsp")),
				CHAR(')')
			)
		);
	
	option_ =
		DEFINE("option",
			GLUE(
				CHAR('['),
				REPEAT(INLINE("c-wsp")),
				REF("alternation"),
				REPEAT(INLINE("c-wsp")),
				CHAR(']')
			)
		);
	
	element_ =
		DEFINE("element",
			CHOICE(
				REF("rulename"),
				REF("group"),
				// REF("option"), // correction over RFC5234
				REF("num-val"),
				REF("char-val"),
				REF("prose-val")
			)
		);
	
	repeat_ =
		DEFINE("repeat",
			CHOICE(
				GLUE(
					REPEAT(INLINE("DIGIT")),
					CHAR('*'),
					REPEAT(INLINE("DIGIT"))
				),
				REPEAT(1, INLINE("DIGIT"))
			)
		);
	
	repetition_ =
		DEFINE("repetition",
			CHOICE(
				GLUE(
					REPEAT(0, 1, REF("repeat")),
					REF("element")
				),
				REF("option")
			)
		);
	
	concatenation_ =
		DEFINE("concatenation",
			GLUE(
				REF("repetition"),
				REPEAT(
					GLUE(
						REPEAT(1, INLINE("c-wsp")),
						REF("repetition")
					)
				)
			)
		);
	
	alternation_ =
		DEFINE("alternation",
			GLUE(
				REF("concatenation"),
				REPEAT(
					GLUE(
						REPEAT(INLINE("c-wsp")),
						CHAR('/'),
						REPEAT(INLINE("c-wsp")),
						REF("concatenation")
					)
				)
			)
		);
	
	ruleName_ =
		DEFINE("rulename",
			GLUE(
				INLINE("ALPHA"),
				REPEAT(
					CHOICE(
						INLINE("ALPHA"),
						INLINE("DIGIT"),
						CHAR('-')
					)
				)
			)
		);
	
	definedAs_ =
		DEFINE("defined-as",
			KEYWORD("= ~")
				// deviation from RFC5234, no support for redefinition ("=/")
				// and added differentiation between matching ('~') and production rules ('=')
		);
	
	rule_ =
		DEFINE("rule",
			GLUE(
				REF("rulename"),
				REPEAT(INLINE("c-wsp")),
				REF("defined-as"),
				REPEAT(INLINE("c-wsp")),
				REF("alternation"),
					// correction of RFC5234, redundant rule "elements" substituted
				REPEAT(INLINE("c-wsp")),
				INLINE("c-nl")
			)
		);
	
	rulelist_ =
		DEFINE("rulelist",
			REPEAT(1,
				GLUE(
					NOT(EOI()),
					CHOICE(
						REF("rule"),
						GLUE(
							REPEAT(INLINE("c-wsp")),
							INLINE("c-nl")
						),
						ERROR()
					)
				)
			)
		);
	
	ENTRY("rulelist");
	LINK();
}

AbnfCompiler::NODE AbnfCompiler::defineValue(const char* digitRule)
{
	return
		GLUE(
			REPEAT(1, INLINE(digitRule)),
			REPEAT(0, 1,
				CHOICE(
					REPEAT(1,
						GLUE(
							CHAR('.'),
							REPEAT(1, INLINE(digitRule))
						)
					),
					GLUE(
						CHAR('-'),
						REPEAT(1, INLINE(digitRule))
					)
				)
			)
		);
}

Ref<AbnfCompiler::Definition, Owner> AbnfCompiler::compile(Ref<ByteArray> text, bool printDefinition)
{
	Ref<Definition, Owner> definition = new Definition;
	Ref<Token, Owner> ruleList = match(text);
	check(ruleList);
	
	Ref<Debugger, Owner> debugger;
	if (printDefinition)
		debugger = new Debugger(definition);
	
	definition->OPTION("caseSensitive", false);
	
	compileRuleList(text, ruleList, definition);
	compileEntry(text, ruleList, definition);
	
	if (debugger)
		debugger->printDefinition();
	
	return definition;
}

void AbnfCompiler::compileRuleList(Ref<ByteArray> text, Ref<Token> ruleList, Ref<Definition> definition)
{
	check(ruleList->rule() == rulelist_);
	
	Ref<Token> rule = ruleList->firstChild();
	while (rule) {
		check(rule->rule() == rule_);
		
		Ref<Token> ruleName = rule->firstChild();
		Ref<Token> definedAs = ruleName->nextSibling();
		Ref<Token> alternation = definedAs->nextSibling();
		
		check(ruleName->rule() == ruleName_);
		check(definedAs->rule() == definedAs_);
		check(alternation->rule() == alternation_);
		check(definedAs->length() == 1); // HACK, disallow "=/" for starters (would require REDEFINE)
		
		if (text->at(definedAs->i0()) == '=')
			definition->DEFINE(str(text, ruleName), compileAlternation(text, alternation, definition));
		else if (text->at(definedAs->i0()) == '~')
			definition->DEFINE_VOID(str(text, ruleName), compileAlternation(text, alternation, definition));
		else
			check(false);
		
		rule = rule->nextSibling();
	}
}

void AbnfCompiler::compileEntry(Ref<ByteArray> text, Ref<Token> ruleList, Ref<Definition> definition)
{
	Ref<Token> rule = ruleList->firstChild();
	check(rule);
	check(rule->rule() == rule_);
	Ref<Token> ruleName = rule->firstChild();
	check(ruleName->rule() == ruleName_);
	
	definition->ENTRY(str(text, ruleName));
}

AbnfCompiler::NODE AbnfCompiler::compileAlternation(Ref<ByteArray> text, Ref<Token> alternation, Ref<Definition> definition)
{
	check(alternation->rule() == alternation_);
	return compileAlternationCascade(text, alternation->firstChild(), definition);
}

AbnfCompiler::NODE AbnfCompiler::compileAlternationCascade(Ref<ByteArray> text, Ref<Token> concatenation, Ref<Definition> definition)
{
	check(concatenation->rule() == concatenation_);
	if (!concatenation->nextSibling())
		return compileConcatenation(text, concatenation, definition);
	return optimizedCHOICE(definition,
		compileConcatenation(text, concatenation, definition),
		compileAlternationCascade(text, concatenation->nextSibling(), definition)
	);
}

AbnfCompiler::NODE AbnfCompiler::compileConcatenation(Ref<ByteArray> text, Ref<Token> concatenation, Ref<Definition> definition)
{
	check(concatenation->rule() == concatenation_);
	return compileConcatenationCascade(text, concatenation->firstChild(), definition);
}

AbnfCompiler::NODE AbnfCompiler::compileConcatenationCascade(Ref<ByteArray> text, Ref<Token> repetition, Ref<Definition> definition)
{
	check(repetition->rule() == repetition_);
	if (!repetition->nextSibling())
		return compileRepetition(text, repetition, definition);
	return definition->GLUE(
		compileRepetition(text, repetition, definition),
		compileConcatenationCascade(text, repetition->nextSibling(), definition)
	);
}

AbnfCompiler::NODE AbnfCompiler::compileRepetition(Ref<ByteArray> text, Ref<Token> repetition, Ref<Definition> definition)
{
	NODE node = 0;
	Ref<Token> token = repetition->firstChild();
	if (token->rule() == repeat_) {
		int i = token->i0();
		while (i < token->i1()) {
			if (text->at(i) == '*') break;
			++i;
		}
		int repeatMin = 0;
		int repeatMax = intMax;
		if (i < token->i1()) {
			if (i > 0) repeatMin = strToInt(*text, token->i0(), i);
			if (i + 1 < token->i1()) repeatMax = strToInt(*text, i + 1, token->i1());
		}
		else {
			repeatMin = repeatMax = strToInt(*text, token->i0(), token->i1());
		}
		if ((repeatMin == 0) && (repeatMax == 0))
			node = definition->PASS();
		else
			node = definition->REPEAT(repeatMin, repeatMax, compileElement(text, token->nextSibling(), definition));
	}
	else if (token->rule() == element_) {
		node = compileElement(text, token, definition);
	}
	else if (token->rule() == option_) {
		node = compileOption(text, token, definition);
	}
	else {
		check(false);
	}
	return node;
}

AbnfCompiler::NODE AbnfCompiler::compileOption(Ref<ByteArray> text, Ref<Token> option, Ref<Definition> definition)
{
	check(option->rule() == option_);
	Ref<Token> alternation = option->firstChild();
	check(alternation);
	return definition->REPEAT(0, 1, compileAlternation(text, alternation, definition));
}

AbnfCompiler::NODE AbnfCompiler::compileElement(Ref<ByteArray> text, Ref<Token> element, Ref<Definition> definition)
{
	NODE node = 0;
	check(element->rule() == element_);
	Ref<Token> token = element->firstChild();
	if (token->rule() == ruleName_) {
		node = definition->REF(str(text, token));
	}
	else if (token->rule() == group_) {
		node = compileAlternation(text, token->firstChild(), definition);
	}
	else if (token->rule() == numVal_) {
		node = compileNumVal(text, token, definition);
	}
	else if (token->rule() == charVal_) {
		node = compileCharVal(text, token, definition);
	}
	else if (token->rule() == proseVal_) {
		node = compileProseVal(text, token, definition);
	}
	else {
		check(false);
	}
	return node;
}

AbnfCompiler::NODE AbnfCompiler::compileNumVal(Ref<ByteArray> text, Ref<Token> numVal, Ref<Definition> definition)
{
	NODE node = 0;
	
	check(numVal->rule() == numVal_);
	check(text->at(numVal->i0()) == '%');
	char prefix = toLower(text->at(numVal->i0() + 1));
	int base;
	if (prefix == 'x')
		base = 16;
	else if (prefix == 'b')
		base = 2;
	else // if (prefix == 'd');
		base = 10;
	
	int i = numVal->i0() + 2;
	while ((i < numVal->i1()) && (text->at(i) != '-') && (text->at(i) != '.')) ++i;
	
	if (i < numVal->i1()) {
		if (text->at(i) == '-') {
			int a = strToInt(*text, numVal->i0() + 2, i, base);
			int b = strToInt(*text, i + 1, numVal->i1(), base);
			node = definition->RANGE(a, b);
		}
		else if (text->at(i) == '.') {
			int n = 1;
			for (int i = numVal->i0() + 2; i < numVal->i1(); ++i) {
				n += text->at(i) == '.';
				++i;
			}
			ByteArray s(n);
			int i0 = numVal->i0() + 2;
			int i = i0;
			int j = 0;
			while (i < numVal->i1()) {
				if (text->at(i) == '.') {
					s.set(j, strToInt(*text, i0, i, base));
					++j;
					i0 = i + 1;
				}
				++i;
			}
			check(j == s.size() - 1);
			s.set(j, strToInt(*text, i0, i, base));
			node = (s.size() > 1) ? definition->STRING(s.constData()) : definition->CHAR(s.at(0));
		}
		else {
			check(false);
		}
	}
	else {
		Char ch = strToInt(*text, numVal->i0() + 2, numVal->i1(), base);
		node = definition->CHAR(ch);
	}
	
	return node;
}

AbnfCompiler::NODE AbnfCompiler::compileCharVal(Ref<ByteArray> text, Ref<Token> charVal, Ref<Definition> definition)
{
	return
		(charVal->length() - 2 > 1) ?
			definition->STRING(text->copy(charVal->i0() + 1, charVal->i1() - 1)->constData()) :
			definition->CHAR(text->at(charVal->i0() + 1));
}

AbnfCompiler::NODE AbnfCompiler::compileProseVal(Ref<ByteArray> text, Ref<Token> proseVal, Ref<Definition> definition)
{
	return compileCharVal(text, proseVal, definition);
}

AbnfCompiler::NODE AbnfCompiler::optimizedCHOICE(Ref<Definition> definition, NODE choice0, NODE choice1)
{
	NODE node = 0;
	
	typedef Debugger::DebugNode DebugNode;
	
	Ref<DebugNode> debugNode0 = choice0;
	Ref<DebugNode> debugNode1 = choice1;
	Ref<CharNode> charNode0 = (debugNode0) ? debugNode0->entry() : Ref<Node>(choice0);
	Ref<CharNode> charNode1 = (debugNode1) ? debugNode1->entry() : Ref<Node>(choice1);
	Ref<RangeExplicitNode> rangeExplicitNode1 = (debugNode1) ? debugNode1->entry() : Ref<Node>(choice1);
	
	if ((charNode0) && (charNode1)) {
		ByteArray s(2);
		s.set(0, charNode0->ch());
		s.set(1, charNode1->ch());
		node = definition->RANGE(s.constData());
	}
	else if ((charNode0) && (rangeExplicitNode1)) {
		int n = rangeExplicitNode1->s().size() + 1;
		ByteArray s(n);
		s.set(0, charNode0->ch());
		for (int i = 1; i < n; ++i)
			s.set(i, rangeExplicitNode1->s().at(i - 1));
		node = definition->RANGE(s.constData());
	}
	else {
		node = definition->CHOICE(choice0, choice1);
	}
	return node;
}

} // namespace ftl