/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/stdio>
#include <flux/RegExp>
#include <flux/DirWalker>
#include <flux/Arguments>

using namespace flux;

class Match: public Object {
public:
    inline static Ref<Match> create(int ln, int i0, int i1) {
        return new Match(ln, i0, i1);
    }

    inline int ln() const { return ln_; }
    inline int i0() const { return i0_; }
    inline int i1() const { return i1_; }

    inline void moveTo(int ln, int i0, int i1) {
        ln_ = ln;
        i0_ = i0;
        i1_ = i1;
    }

private:
    Match(int ln, int i0, int i1):
        ln_(ln),
        i0_(i0),
        i1_(i1)
    {}
    int ln_;
    int i0_;
    int i1_;
};

typedef List< Ref<Match> > Matches;

Ref<Matches> findMatches(ByteArray *text, SyntaxDefinition *textPattern);
void displayMatch(ByteArray *path, ByteArray *text, Match *match);
String replaceMatches(ByteArray *text, Matches *matches, ByteArray *replacement);

int main(int argc, char **argv)
{
    String toolName = String(argv[0])->fileName();

    try {
        Ref<Arguments> arguments = Arguments::parse(argc, argv);

        Ref<VariantMap> options = VariantMap::create();
        options->insert("path", "");
        options->insert("name", "");
        options->insert("type", "");
        options->insert("depth", -1);
        options->insert("hidden", false);
        options->insert("text", "");
        options->insert("word", "");
        options->insert("ranges", false);
        options->insert("replace", "");
        options->insert("paste", "");
        options->insert("erase", false);
        arguments->validate(options);
        arguments->override(options);

        Ref<StringList> items = arguments->items();

        RegExp pathPattern = options->value("path");
        RegExp namePattern = options->value("name");
        RegExp typePattern = options->value("type");
        int maxDepth = options->value("depth");
        bool ignoreHidden = !options->value("hidden");

        RegExp textPattern = String(options->value("text"))->unescape();
        if (String(options->value("word")) != "")
            textPattern = String(Format() << "(?<!:[a..z]|[A..Z]|[0..9]|_)" << options->value("word") << "(?>!:[a..z]|[A..Z]|[0..9]|_)");

        bool rangesOption = options->value("ranges");
        bool replaceOption = false;
        String replacement;
        if (arguments->options()->lookup("replace", &replacement))
            replaceOption = true;
        if (String(options->value("paste")) != "") {
            // if (replaceOption == true) // FIXME: multiple conflicting replacement options
            replaceOption = true;
            replacement = File::open(options->value("paste"))->map();
        }
        if (options->value("erase")) {
            // if (replaceOption == true) // FIXME: multiple conflicting replacement options
            replaceOption = true;
            replacement = "";
        }

        if (items->count() == 0) items->append(".");

        for (int i = 0; i < items->count(); ++i) {
            String dirPath = items->at(i)->canonicalPath();
            Ref<DirWalker> dirWalker = DirWalker::open(dirPath);
            dirWalker->setMaxDepth(maxDepth);
            dirWalker->setIgnoreHidden(ignoreHidden);
            String path;
            while (dirWalker->read(&path)) {
                if (pathPattern != "") {
                    if (!pathPattern->match(path)->valid()) continue;
                }
                if (namePattern != "") {
                    if (!namePattern->match(path->fileName())->valid()) continue;
                }
                if (typePattern != "") {
                    int type = FileStatus::read(path, false)->type();
                    bool shortMode = (typePattern->matchLength() == 1);
                    String typeString;
                    if (type == File::Regular)          typeString = shortMode ? "r" : "regular file";
                    else if (type == File::Directory)   typeString = shortMode ? "d" : "directory";
                    else if (type == File::Symlink)     typeString = shortMode ? "l" : "symlink";
                    else if (type == File::CharDevice)  typeString = shortMode ? "c" : "character device";
                    else if (type == File::BlockDevice) typeString = shortMode ? "b" : "block device";
                    else if (type == File::Fifo)        typeString = shortMode ? "f" : "fifo";
                    else if (type == File::Socket)      typeString = shortMode ? "s" : "socket";
                    if (!typePattern->find(typeString)->valid()) continue;
                }
                if (textPattern != "") {
                    if (FileStatus::read(path)->type() != File::Regular)
                        continue;

                    String text = File::open(path)->map();
                    Ref<Matches> matches = findMatches(text, textPattern);

                    if (replaceOption && matches->count() > 0) {
                        Ref<File> file = File::open(path, File::ReadWrite);
                        text = replaceMatches(text, matches, replacement);
                        file->truncate(0);
                        file->write(text);
                    }
                    for (int i = 0; i < matches->count(); ++i) {
                        Match *match = matches->at(i);
                        if (rangesOption)
                            fout("%%:%%:%%..%%\n") << path << match->ln() << match->i0() << match->i1();
                        else
                            displayMatch(path, text, match);
                    }
                    continue;
                }
                fout() << path << nl;
            }
        }
    }
    catch (HelpError &) {
        fout(
            "Usage: %% [OPTION]... [DIR]...\n"
            "Find (and replace) recursively descending into each [DIR]\n"
            "\n"
            "Options:\n"
            "  -path     file path pattern\n"
            "  -name     file name pattern\n"
            "  -type     file type pattern\n"
            "              r .. regular file\n"
            "              d .. directory\n"
            "              l .. symbolic link\n"
            "              c .. character device\n"
            "              b .. block device\n"
            "              f .. fifo\n"
            "              s .. socket\n"
            "  -depth    maximum depth of recursive directory search\n"
            "  -hidden   also search hidden files and directories\n"
            "  -text     text search pattern\n"
            "  -word     word search pattern\n"
            "  -ranges   show line and byte range for each match\n"
            "  -replace  replace matches by given text\n"
            "  -paste    paste replacement from file\n"
            "  -erase    replace matches by empty string\n"
        ) << toolName;
        return 1;
    }
    catch (SystemResourceError &ex) {
        ferr() << toolName << ": " << ex << nl;
    }
    catch (SyntaxError &ex) {
        ferr() << toolName << ": " << ex << nl;
    }
    #ifdef NDEBUG
    catch (Exception &ex) {
        ferr() << toolName << ": " << ex << nl;
        return 1;
    }
    #endif

    return 0;
}

Ref<Matches> findMatches(ByteArray *text, SyntaxDefinition *textPattern)
{
    Ref<Matches> matches = Matches::create();
    int ln = 1;
    for (int i = 0; i < text->count();) {
        Ref<SyntaxState> state = textPattern->find(text, i);
        if (!state->valid()) break;
        int i0 = state->i0();
        int i1 = state->i1();
        Ref<Range> capture;
        if (state->lookupCapture("", &capture)) {
            i0 = capture->i0();
            i1 = capture->i1();
        }
        for (;i < i0; ++i)
            if (text->at(i) == '\n') ++ln;
        matches->append(Match::create(ln, i0, i1));
        for (;i < i1; ++i)
            if (text->at(i) == '\n') ++ln;
        if (i0 == i1) ++i;
    }
    return matches;
}

void displayMatch(ByteArray *path, ByteArray *text, Match *match)
{
    int ln = match->ln();
    int i0 = match->i0();
    int i1 = match->i1();

    int j0 = i0;
    for (;j0 > 0; --j0)
        if (text->at(j0 - 1) == '\n') break;

    fout("%%:") << path;
    if (i0 == i1) {
        fout("%%: %%\n") << ln << text->copy(j0, text->find("\n", i0));
        return;
    }

    bool multiline = text->copy(i0, i1)->contains('\n');
    if (multiline) fout() << nl;

    for (int j1 = j0; j0 < i1; j0 = j1) {
        for (;j1 < text->count(); ++j1)
            if (text->at(j1) == '\n') break;
        Format line;
        line << ln << ": ";
        int k0 = j0, k1 = j1;
        if (j0 <= i0 && i0 < j1) k0 = i0;
        if (j0 < i1 && i1 < j1) k1 = i1;
        if (j0 < k0) line << text->copy(j0, k0);
        if (k0 < k1) line << "\033[7m" << text->copy(k0, k1) << "\033[m";
        if (k1 < j1) line << text->copy(k1, j1);
        line << "\n";
        fout() << line->join();
        ++ln;
        ++j1;
    }
}

String replaceMatches(ByteArray *text, Matches *matches, ByteArray *replacement)
{
    Ref<StringList> fragments = StringList::create();
    int fi0 = 0; // begin of fragment
    int si = 0, sl = 0; // index and line shift
    int nr = replacement->count('\n');
    for (int i = 0; i < matches->count(); ++i) {
        Match *match = matches->at(i);
        fragments->append(text->copy(fi0, match->i0()));
        fi0 = match->i1();
        int i0s = match->i0() + si;
        si += replacement->count() - (match->i1() - match->i0());
        sl += nr - text->copy(match->i0(), match->i1())->count('\n');
        match->moveTo(match->ln() + sl, i0s, i0s + replacement->count());
    }
    fragments->append(text->copy(fi0, text->count()));
    return fragments->join(replacement);
}
