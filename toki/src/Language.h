/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUXTOKI_LANGUAGE_H
#define FLUXTOKI_LANGUAGE_H

#include <flux/syntax/SyntaxDefinition>
#include <flux/regexp/RegExp>

namespace flux {
namespace toki {

using namespace flux::syntax;
using namespace flux::regexp;

class Language: public Object
{
public:
    inline String displayName() const { return displayName_; }
    inline String name() const { return name_; }
    inline RegExp pathPattern() const { return pathPattern_; }
    inline const SyntaxDefinition *highlightingSyntax() const { return highlightingSyntax_; }
    inline const SyntaxDefinition *discoverySyntax() const { return discoverySyntax_; }
    inline const SyntaxDefinition *foldingSyntax() const { return foldingSyntax_; }

protected:
    Language(
        String displayName,
        RegExp pathPattern,
        const SyntaxDefinition *highlightingSyntax,
        const SyntaxDefinition *discoverySyntax = 0,
        const SyntaxDefinition *foldingSyntax = 0
    );

private:
    Language();

    String displayName_;
    String name_;
    RegExp pathPattern_;
    Ref<const SyntaxDefinition> highlightingSyntax_;
    Ref<const SyntaxDefinition> discoverySyntax_;
    Ref<const SyntaxDefinition> foldingSyntax_;
};

}} // namespace flux::toki

#endif // FLUXTOKI_LANGUAGE_H
