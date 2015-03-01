/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/stdio>
#include <flux/Glob>
#include <flux/Dir>
#include <flux/ResourceGuard>
#include <flux/yason>
#include <flux/Arguments>
#include "BuildMap.h"
#include "DependencyCache.h"
#include "GnuToolChain.h"
#include "JobScheduler.h"
#include "RecipeProtocol.h"
#include "BuildPlan.h"

namespace fluxmake {

Ref<BuildPlan> BuildPlan::create(int argc, char **argv)
{
    return new BuildPlan(argc, argv);
}

Ref<BuildPlan> BuildPlan::create(String projectPath)
{
    Ref<BuildPlan> plan;
    if (buildMap()->lookupPlan(String(projectPath->absolutePath()), &plan)) return plan;
    return new BuildPlan(projectPath, this);
}

#define FLUXMAKE_BUILDPLAN_COMPONENTS_INIT \
    shell_(this), \
    configureStage_(this), \
    preparationStage_(this), \
    analyseStage_(this), \
    compileLinkStage_(this), \
    testRunStage_(this), \
    installStage_(this), \
    uninstallStage_(this), \
    cleanStage_(this)

BuildPlan::BuildPlan(int argc, char **argv):
    projectPath_("."),
    concurrency_(-1),
    FLUXMAKE_BUILDPLAN_COMPONENTS_INIT
{
    Ref<Arguments> arguments = Arguments::parse(argc, argv);
    StringList *items = arguments->items();

    if (items->count() > 0) {
        if (items->count() > 1)
            throw UsageError("Handling multiple source directories at once is not supported");
        projectPath_ = items->at(0);
    }

    projectPath_ = projectPath_->absolutePath()->canonicalPath();
    recipePath_ = recipePath(projectPath_);

    ResourceGuard context(recipePath_);
    recipe_ = yason::parse(File::open(recipePath_)->map(), recipeProtocol());
    arguments->validate(recipe_);
    arguments->override(recipe_);

    readRecipe();

    toolChain_ = GnuToolChain::create(compiler());
    if (optimize_ == "") optimize_ = toolChain_->defaultOptimization(this);

    buildMap()->insertPlan(projectPath_, this);
    scope_ = projectPath_;
}

BuildPlan::BuildPlan(String projectPath, BuildPlan *parentPlan):
    toolChain_(parentPlan->toolChain_),
    projectPath_(projectPath),
    recipePath_(recipePath(projectPath)),
    scope_(parentPlan->scope_),
    concurrency_(parentPlan->concurrency_),
    FLUXMAKE_BUILDPLAN_COMPONENTS_INIT
{
    ResourceGuard context(recipePath_);
    recipe_ = yason::parse(File::open(recipePath_)->map(), recipeProtocol());
    readRecipe(parentPlan);
    buildMap()->insertPlan(projectPath_, this);
}

void BuildPlan::readRecipe(BuildPlan *parentPlan)
{
    name_ = recipe_->value("name");
    alias_ = recipe_->value("alias");
    version_ = recipe_->value("version");
    installPrefix_ = recipe_->value("prefix");
    testArgs_ = recipe_->value("test-args");

    options_ = 0;

    if (recipe_->className() == "Application")  options_ |= Application;
    else if (recipe_->className() == "Test")    options_ |= Application | Test;
    else if (recipe_->className() == "Library") options_ |= Library;
    else if (recipe_->className() == "Tools")   options_ |= Tools;
    else if (recipe_->className() == "Tests")   options_ |= Tools | Test;
    else if (recipe_->className() == "Package") options_ |= Package;

    checkDuplicateTargetNames();

    if (recipe_->value("debug"))       options_ |= Debug;
    if (recipe_->value("release"))     options_ |= Release;
    if (recipe_->value("simulate"))    options_ |= Simulate;
    if (recipe_->value("blindfold"))   options_ |= Blindfold;
    if (recipe_->value("bootstrap"))   options_ |= Bootstrap | Simulate | Blindfold;
    if (recipe_->value("test"))        options_ |= BuildTests;
    if (recipe_->value("test-run"))    options_ |= BuildTests;
    if (recipe_->value("test-report")) options_ |= BuildTests;
    if (recipe_->value("clean"))       options_ |= BuildTests;
    if (recipe_->value("verbose"))     options_ |= Verbose;
    if (recipe_->value("paranoid"))    options_ |= Paranoid;
    if (recipe_->value("configure"))   options_ |= Configure;

    concurrency_ = recipe_->value("concurrency");

    if (parentPlan) {
        options_ &= ~GlobalOptions;
        options_ |= parentPlan->options() & GlobalOptions;
        concurrency_ = parentPlan->concurrency_;
        installPrefix_ = parentPlan->installPrefix_;
        testArgs_ = parentPlan->testArgs_;
    }

    BuildParameters::read(recipe_, this);

    String defaultIncludePath = projectPath_->expandPath("include");
    if (Dir::exists(defaultIncludePath)) {
        if (!includePaths_->contains(defaultIncludePath))
            includePaths_->append(defaultIncludePath);
    }

    if (recipe_->hasChildren()) {
        for (int i = 0; i < recipe_->children()->count(); ++i) {
            MetaObject *object = recipe_->children()->at(i);
            if (object->className() == "SystemPrerequisite") {
                Ref<SystemPrerequisite> p = SystemPrerequisite::read(object, this);
                Ref<SystemPrerequisiteList> l;
                if (!systemPrerequisitesByName_)
                    systemPrerequisitesByName_ = SystemPrerequisitesByName::create();
                if (!systemPrerequisitesByName_->lookup(p->name(), &l))
                    systemPrerequisitesByName_->insert(p->name(), l = SystemPrerequisiteList::create());
                l->append(p);
            }
            else if (object->className() == "Predicate") {
                if (!predicates_) predicates_ = PredicateList::create();
                predicates_->append(Predicate::read(object));
            }
            else if (object->className() == "Usage") {
                usage_ = BuildParameters::create();
                usage_->read(object, this);
                BuildParameters::readSpecific(usage_);
            }
        }
    }

    if (parentPlan) {
        optimize_ = parentPlan->optimize();
        linkStatic_ = parentPlan->linkStatic();
    }
}

void BuildPlan::checkDuplicateTargetNames()
{
    if (name_ == "") return;

    String otherRecipePath;
    bool ok = false;
    if (options_ & Library)
        ok = buildMap()->registerLibrary(name_, recipePath_, &otherRecipePath);
    else if (options_ & Application)
        ok = buildMap()->registerApplication(name_, recipePath_, &otherRecipePath);
    if (!ok) {
        throw UsageError(
            Format("Duplicate target name '%%' in\n  %%\n  and\n  %%")
            << name_
            << otherRecipePath
            << recipePath_
        );
    }
}

int BuildPlan::run()
{
    readPrerequisites();

    if (options_ & Bootstrap) {
        fout(
            "#!/bin/sh -ex\n"
            "SOURCE=$1\n"
            "MACHINE=$(%%)\n"
        ) << toolChain_->machineCommand();
    }

    if (!recipe_->value("clean")) {
        if (!preparationStage()->run())
            return 1;
    }

    globSources();
    initModules();

    if (!configureStage()->run()) return 1;
    if (options_ & Configure) return 0;

    if (!analyseStage()->run()) return 1;

    if (recipe_->value("clean")) return !cleanStage()->run();
    if (recipe_->value("uninstall")) return !uninstallStage()->run();

    if (!compileLinkStage()->run()) return 1;

    if (recipe_->value("test-run") || recipe_->value("test-report")) {
        if (!testRunStage()->run())
            return testRunStage()->status();
    }

    if (recipe_->value("install")) return !installStage()->run();

    return 0;
}

String BuildPlan::sourcePath(String source) const
{
    if (projectPath_ == ".") return source;
    return projectPath_ + "/" + source;
}

String BuildPlan::modulePath(String object) const
{
    return modulePath_ + "/" + object;
}

String BuildPlan::installPath(String relativeInstallPath) const
{
    return installPrefix_ + "/" + relativeInstallPath;
}

Ref<StringList> BuildPlan::globSources(StringList *pattern) const
{
    Ref<StringList> sources = StringList::create();
    for (int i = 0; i < pattern->count(); ++i) {
        Ref<Glob> glob = Glob::open(sourcePath(pattern->at(i)));
        for (String path; glob->read(&path);)
            sources->append(path);
    }
    sources = sources->sort();
    return sources;
}

void BuildPlan::use(BuildPlan *plan)
{
    if (plan->options() & Library) {
        String path = plan->projectPath();
        String defaultIncludePath = path->expandPath("include");
        if (Dir::exists(defaultIncludePath)) {
            if (!includePaths_->contains(defaultIncludePath))
                includePaths_->append(defaultIncludePath);
        }
        else if (!includePaths_->contains(path)) {
            includePaths_->append(path);
        }
        if (!libraryPaths_->contains("."))
            libraryPaths_->append(".");
        libraries_->append(plan->name());

        if (plan->usage()) BuildParameters::readSpecific(plan->usage());
    }
    else if (plan->options() & Package) {
        for (int i = 0; i < plan->prerequisites()->count(); ++i)
            use(plan->prerequisites()->at(i));
    }
    // else if FIXME: warn/error when using Application or Tools
}

String BuildPlan::findPrerequisite(String prerequisitePath) const
{
    if (prerequisitePath->isAbsolutePath()) {
        if (File::exists(recipePath(prerequisitePath))) return prerequisitePath;
        return String();
    }
    for (String path = projectPath_; path != "/"; path = path->reducePath()) {
        String candidatePath = path + "/" + prerequisitePath;
        if (File::exists(recipePath(candidatePath))) {
            candidatePath = candidatePath->canonicalPath();
            if (candidatePath == projectPath_) continue;
            return candidatePath;
        }
    }
    return String();
}

void BuildPlan::readPrerequisites()
{
    if (prerequisites_) return;

    prerequisites_ = BuildPlanList::create();

    if ((options_ & Test) && !(options_ & BuildTests)) return;

    StringList *prerequisitePaths = cast<StringList>(recipe_->value("use"));

    for (int i = 0; i < prerequisitePaths->count(); ++i) {
        String path = findPrerequisite(prerequisitePaths->at(i));
        if (path == "")
            throw UsageError(Format() << recipePath() << ": Failed to locate prerequisite \"" << prerequisitePaths->at(i) << "\"");
        Ref<BuildPlan> plan = BuildPlan::create(path);
        plan->readPrerequisites();
        use(plan);
        prerequisites_->append(plan);
    }

    findVersion();
}

void BuildPlan::findVersion()
{
    String useVersion = recipe_->value("use-version");
    if (useVersion != "") {
        String path = findPrerequisite(useVersion);
        if (path == "")
            throw UsageError(Format() << recipePath() << ": Failed to locate prerequisite \"" << useVersion << "\"");
        Ref<BuildPlan> plan = BuildPlan::create(path);
        if (plan->version_ == "")
            throw UsageError(Format() << recipePath() << ": Failed to retrieve version from \"" << useVersion << "\"");
        version_ = plan->version_;
    }
    else if (version_ == "") {
        String path = findPrerequisite("..");
        if (path != "") {
            Ref<BuildPlan> plan = BuildPlan::create(path);
            if (plan->version_ == "") plan->findVersion();
            if (plan->version_ != "") version_ = plan->version_;
        }
    }
    if (version_ == "")
        version_ = "0.1.0";
}

void BuildPlan::globSources()
{
    if (sources_) return;

    if ((options_ & Test) && !(options_ & BuildTests)) return;

    if (recipe_->contains("source"))
        sources_ = globSources(cast<StringList>(recipe_->value("source")));
    else
        sources_ = StringList::create();

    sourcePrefix_ = buildMap()->commonPrefix();
    if (sourcePrefix_ == "") sourcePrefix_ = projectPath_;
    else sourcePrefix_ = sourcePrefix_->canonicalPath();

    containsCPlusPlus_ = false;
    for (int i = 0; i < sources_->count(); ++i) {
        String suffix = sources_->at(i)->suffix();
        if (suffix == "cpp" || suffix == "cc" || suffix == "cxx" || suffix == "mm") {
            containsCPlusPlus_ = true;
            break;
        }
    }

    if (recipe_->contains("bundle"))
        bundle_ = globSources(cast<StringList>(recipe_->value("bundle")));
    else
        bundle_ = StringList::create();

    for (int i = 0; i < prerequisites_->count(); ++i)
        prerequisites_->at(i)->globSources();
}

void BuildPlan::initModules()
{
    if (modules_) return;

    if ((options_ & Test) && !(options_ & BuildTests)) return;

    modules_ = ModuleList::create();

    Format f;
    f << ".modules";
    {
        Format h;
        String path = projectPath_->absolutePath();
        String topLevel = sourcePrefix_->absolutePath();
        while (path != topLevel && path != "/") {
            h << path->fileName();
            path = path->reducePath();
        }
        h << topLevel->fileName();
        f << h->reverse()->join("_");
    }
    if (version_ != "") f << version_;
    if (linkStatic_) f << "static";
    if (options_ & Debug) f << "debug";
    if (options_ & Release) f << "release";
    if (optimize_ != "") f << "O" + optimize_;
    if (options_ & Bootstrap)
        f << "$MACHINE";
    else
        f << toolChain_->machine();
    modulePath_ = f->join("-");

    for (int i = 0; i < prerequisites_->count(); ++i)
        prerequisites_->at(i)->initModules();
}

} // namespace fluxmake
