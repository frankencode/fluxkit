/*
 * CommandLine.hpp -- command line interface
 *
 * Copyright (c) 2007-2010, Frank Mertens
 *
 * See ../LICENSE for the license.
 */
#ifndef PONA_COMMANDLINE_HPP
#define PONA_COMMANDLINE_HPP

#include "atoms"
#include "String.hpp"
#include "Variant.hpp"
#include "List.hpp"
#include "Syntax.hpp"
#include "CommandOption.hpp"

namespace pona
{

PONA_EXCEPTION(CommandLineException, Exception);

class CommandLine: public Syntax<String::Media>::Definition
{
public:
	CommandLine();
	
	Ref<CommandOption> define(char shortName, String longName, Variant defaultValue = false, String description = "");
	Ref<StringList, Owner> read(int argc, char** argv);
	
	String entity(String newValue = "");
	String synopsis(String newValue = "");
	String summary(String newValue = "");
	String details(String newValue = "");
	String helpText() const;
	String execPath() const;
	String execName() const;
	String execDir() const;
	
private:
	Ref<StringList, Owner> read(String line);
	
	RULE longNameRule_;
	RULE shortNameRule_;
	RULE valueRule_;
	RULE optionRule_;
	RULE options_;
	
	typedef List< Ref<CommandOption, Owner> > OptionList;
	Ref<OptionList, Owner> optionList_;
	String entity_;
	String synopsis_;
	String summary_;
	String details_;
	
	String execPath_;
	String execName_;
	String execDir_;
	
	Ref<CommandOption> optionByShortName(char name) const;
	Ref<CommandOption> optionByLongName(String name) const;
	void readOption(String line, Ref<Token> token);
	void readValue(Ref<CommandOption> option, String line, Ref<Token> token);
	void verifyTypes();
};

} // namespace pona

#endif // PONA_COMMANDLINE_HPP