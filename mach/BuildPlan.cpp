#include <ftl/PrintDebug.hpp>
#include <ftl/File.hpp>
#include <ftl/Dir.hpp>
#include <ftl/Glob.hpp>
#include <ftl/Process.hpp>
#include <ftl/Config.hpp>
#include <ftl/System.hpp>
#include "DependencyCache.hpp"
#include "GccToolChain.hpp"
#include "JobScheduler.hpp"
#include "BuildPlan.hpp"

namespace mach
{

hook<BuildPlan> BuildPlan::create(int argc, char **argv)
{
	return new BuildPlan(argc, argv);
}

hook<BuildPlan> BuildPlan::create(ToolChain *toolChain, string projectPath, int globalOptions)
{
	BuildPlan *buildPlan;
	if (buildMap_->lookup(projectPath, &buildPlan)) return buildPlan;
	return new BuildPlan(toolChain, projectPath, this);
}

BuildPlan::BuildPlan(int argc, char **argv)
	: toolChain_(GccToolChain::create()),
	  projectPath_("."),
	  buildMap_(BuildMap::create()),
	  prepareComplete_(false),
	  analyseComplete_(false),
	  buildComplete_(false),
	  cleanComplete_(false),
	  distCleanComplete_(false),
	  buildResult_(false)
{
	recipe_ = Config::create();
	recipe_->read(argc, argv);
	if (recipe_->arguments()->length() > 0) {
		if (recipe_->arguments()->length() > 1)
			FTL_THROW(BuildPlanException, "Processing multiple Recipe files at once is not supported");
		projectPath_ = recipe_->arguments()->at(0)->canonicalPath();
	}
	recipe_->clear();
	recipe_->read(projectPath_ + "/Recipe");
	recipe_->read(argc, argv);
	readRecipe();

	buildMap_->insert(projectPath_, this);
}

BuildPlan::BuildPlan(ToolChain *toolChain, string projectPath, BuildPlan *parentPlan)
	: toolChain_(toolChain),
	  projectPath_(projectPath),
	  buildMap_(parentPlan->buildMap_),
	  prepareComplete_(false),
	  analyseComplete_(false),
	  buildComplete_(false),
	  cleanComplete_(false),
	  distCleanComplete_(false),
	  analyseResult_(false),
	  buildResult_(false)
{
	recipe_ = Config::create();
	recipe_->read(projectPath_ + "/Recipe");

	readRecipe(parentPlan);

	buildMap_->insert(projectPath_, this);
}

void BuildPlan::readRecipe(BuildPlan *parentPlan)
{
	name_ = recipe_->value("name");
	version_ = recipe_->value("version");
	options_ = 0;
	speedOptimizationLevel_ = -1;
	sizeOptimizationLevel_ = -1;

	if (recipe_->className() == "Application") options_ |= Application;
	else if (recipe_->className() == "Library") options_ |= Library;
	else if (recipe_->className() == "ToolSet") options_ |= ToolSet;
	else if (recipe_->className() == "Package") options_ |= Package;

	if (recipe_->flag("debug"))     options_ |= Debug;
	if (recipe_->flag("release"))   options_ |= Release;
	if (recipe_->flag("static"))    options_ |= Static;
	if (recipe_->flag("dry-run"))   options_ |= DryRun;
	if (recipe_->flag("blindfold")) options_ |= Blindfold;
	if (recipe_->flag("bootstrap")) options_ |= Bootstrap | DryRun | Blindfold;
	if (recipe_->flag("verbose"))   options_ |= Verbose;

	if (recipe_->contains("optimize-speed")) {
		options_ |= OptimizeSpeed;
		variant h = recipe_->value("optimize-speed");
		if (type(h) == IntType)
			speedOptimizationLevel_ = h;
		else
			speedOptimizationLevel_ = toolChain_->defaultSpeedOptimizationLevel();
	}
	if (recipe_->contains("optimize-size")) {
		options_ |= OptimizeSize;
		variant h = recipe_->value("optimize-size");
		if (type(h) == IntType)
			sizeOptimizationLevel_ = h;
		else
			sizeOptimizationLevel_ = toolChain_->defaultSizeOptimizationLevel();
	}

	if (recipe_->contains("include-path"))
		includePaths_ = cast<VariantList>(recipe_->value("include-path"))->toList<string>();
	else
		includePaths_ = StringList::create();

	if (recipe_->contains("link-path"))
		libraryPaths_ = cast<VariantList>(recipe_->value("link-path"))->toList<string>();
	else
		libraryPaths_ = StringList::create();

	if (recipe_->contains("link"))
		libraries_ = cast<VariantList>(recipe_->value("link"))->toList<string>();
	else
		libraries_ = StringList::create();

	if (parentPlan) {
		options_ &= ~GlobalOptions;
		options_ |= (parentPlan->options() & GlobalOptions);
		speedOptimizationLevel_ = parentPlan->speedOptimizationLevel();
		sizeOptimizationLevel_ = parentPlan->sizeOptimizationLevel();
	}
}

int BuildPlan::run()
{
	if (recipe_->flag("h") || recipe_->flag("help")) {
		print("no help, yet...\n");
		return 0;
	}

	prepare();

	if (options_ & Bootstrap) {
		printTo(error(),
			"#!/bin/sh -ex\n"
			"SOURCE=$1\n"
			"MACHINE=$(%%)\n",
			toolChain_->machineCommand()
		);
	}

	if (!analyse()) return 1;

	if (recipe_->flag("c") || recipe_->flag("clean")) {
		clean();
		return 0;
	}

	return build() ? 0 : 1;
}

string BuildPlan::sourcePath(string source) const
{
	if (projectPath_ == ".") return source;
	return projectPath_ + "/" + source;
}

string BuildPlan::modulePath(string object) const
{
	return modulePath_ + "/" + object;
}

string BuildPlan::beautifyCommand(string command)
{
	if (options_ & Bootstrap) {
		return command
			->replace(sourcePrefix_, string("$SOURCE"))
			->replace(Process::cwd(), string("$PWD"));
	}
	return command;
}

bool BuildPlan::runBuild(string command)
{
	error()->writeLine(beautifyCommand(command));
	if (options_ & DryRun) return true;
	return Process::start(command)->wait() == 0;
}

bool BuildPlan::mkdir(string path)
{
	if (!fileStatus(path)->exists())
		printTo(error(), "mkdir -p %%\n", path);
	if (options_ & DryRun) return true;
	return Dir::establish(path);
}

bool BuildPlan::rmdir(string path)
{
	if (fileStatus(path)->exists())
		printTo(error(), "rmdir %%\n", path);
	if (options_ & DryRun) return true;
	return Dir::unlink(path);
}

bool BuildPlan::symlink(string path, string newPath)
{
	printTo(error(), "ln -sf %% %%\n", path, newPath);
	if (options_ & DryRun) return true;
	File::unlink(newPath);
	return File::symlink(path, newPath);
}

bool BuildPlan::unlink(string path)
{
	if (File::unresolvedStatus(path)->exists()) {
		printTo(error(), "rm %%\n", path);
		if (options_ & DryRun) return true;
		return File::unlink(path);
	}
	return true;
}

hook<FileStatus> BuildPlan::fileStatus(string path)
{
	if (options_ & Blindfold) return FileStatus::read();
	return FileStatus::read(path);
}

void BuildPlan::prepare()
{
	if (prepareComplete_) return;
	prepareComplete_ = true;

	prequisites_ = BuildPlanList::create();

	hook<StringList> prequisitePaths;
	if (recipe_->contains("use"))
		prequisitePaths = cast<VariantList>(recipe_->value("use"))->toList<string>();
	else
		prequisitePaths = StringList::create();

	for (int i = 0; i < prequisitePaths->length(); ++i) {
		string path = prequisitePaths->at(i);
		if (path->isRelativePath()) path = projectPath_ + "/" + path;
		path = path->canonicalPath();
		hook<BuildPlan> buildPlan = BuildPlan::create(toolChain_, path, options_ & GlobalOptions);
		if (buildPlan->options() & Library) {
			path = path->reducePath();
			if (!includePaths_->contains(path))
				includePaths_->append(path);
			if (!libraryPaths_->contains("."))
				libraryPaths_->append(".");
			libraries_->append(buildPlan->name());
		}
		buildPlan->prepare();
		prequisites_->append(buildPlan);
	}

	sources_ = StringList::create();
	if (recipe_->contains("source")) {
		VariantList *sourcePatterns = cast<VariantList>(recipe_->value("source"));
		for (int i = 0; i < sourcePatterns->length(); ++i) {
			hook<Glob> glob = Glob::open(sourcePath(sourcePatterns->at(i)));
			for (string path; glob->read(&path);)
				sources_->append(path);
		}
	}
}

bool BuildPlan::analyse()
{
	if (analyseComplete_) return analyseResult_;
	analyseComplete_ = true;

	sourcePrefix_ = buildMap_->commonPrefix()->canonicalPath();

	{
		format f;
		f << ".modules";
		{
			format h;
			string path = projectPath_->absolutePath();
			string topLevel = sourcePrefix_->absolutePath();
			while (path != topLevel) {
				h << path->fileName();
				path = path->reducePath();
			} ;
			h << topLevel->fileName();
			f << h->reverse()->join("_");
		}
		if (version_ != "") f << version_;
		if (options_ & Static) f << "static";
		if (options_ & Debug) f << "debug";
		if (options_ & Release) f << "release";
		if (options_ & OptimizeSpeed) {
			format h;
			h << "optimize" << "speed" << speedOptimizationLevel_;
			f << h->join("-");
		}
		if (options_ & OptimizeSize) {
			format h;
			h << "optimize" << "size" << sizeOptimizationLevel_;
			f << h->join("-");
		}
		if (options_ & Bootstrap)
			f << "$MACHINE";
		else
			f << toolChain_->machine();
		modulePath_ = f->join("-");
	}

	for (int i = 0; i < prequisites_->length(); ++i)
		if (!prequisites_->at(i)->analyse()) return analyseResult_ = false;

	if (options_ & Package) return analyseResult_ = true;

	mkdir(modulePath_);

	modules_ = ModuleList::create();
	hook<JobScheduler> scheduler;

	hook<DependencyCache> dependencyCache = DependencyCache::create(this);
	for (int i = 0; i < sources_->length(); ++i) {
		hook<Module> module;
		if (dependencyCache->lookup(sources_->at(i), &module)) {
			modules_->append(module);
		}
		else {
			if (!scheduler) {
				scheduler = JobScheduler::create();
				scheduler->start();
			}
			scheduler->schedule(toolChain_->createAnalyseJob(this, sources_->at(i)));
		}
	}

	if (!scheduler) return analyseResult_ = true;

	for (hook<Job> job; scheduler->collect(&job);) {
		if ((options_ & Verbose) || (job->status() != 0)) {
			error()->writeLine(beautifyCommand(job->command()));
			// error()->writeLine(job->outputText());
		}
		if (job->status() != 0) break;
		hook<Module> module = toolChain_->finishAnalyseJob(this, job);
		dependencyCache->insert(module->sourcePath(), module);
		modules_->append(module);
	}

	return analyseResult_ = (scheduler->status() == 0);
}

bool BuildPlan::build()
{
	if (buildComplete_) return buildResult_;
	buildComplete_ = true;

	for (int i = 0; i < prequisites_->length(); ++i)
		if (!prequisites_->at(i)->build()) return buildResult_ = false;

	if (options_ & Package) return buildResult_ = true;

	hook<JobScheduler> compileScheduler;
	hook<JobScheduler> linkScheduler;

	for (int i = 0; i < modules_->length(); ++i) {
		Module *module = modules_->at(i);
		bool dirty = module->dirty();
		if (options_ & ToolSet)
			dirty = dirty || !fileStatus(module->toolName())->exists();
		if (dirty) {
			hook<Job> job = toolChain_->createCompileJob(this, module);
			if (options_ & DryRun) {
				error()->writeLine(beautifyCommand(job->command()));
			}
			else {
				if (!compileScheduler) {
					compileScheduler = JobScheduler::create();
					compileScheduler->start();
				}
				compileScheduler->schedule(job);
				if (options_ & ToolSet) {
					if (!linkScheduler) linkScheduler = JobScheduler::create();
					linkScheduler->schedule(toolChain_->createLinkJob(this, module));
				}
			}
		}
	}

	if (compileScheduler) {
		for (hook<Job> job; compileScheduler->collect(&job);) {
			error()->writeLine(beautifyCommand(job->command()));
			output()->write(job->outputText());
			if (job->status() != 0) return buildResult_ = false;
		}
	}

	if (options_ & ToolSet) {
		if (linkScheduler) {
			linkScheduler->start();
			for (hook<Job> job; linkScheduler->collect(&job);) {
				error()->writeLine(beautifyCommand(job->command()));
				output()->write(job->outputText());
				if (job->status() != 0) return buildResult_ = false;
			}
		}
		return buildResult_ = true;
	}

	hook<FileStatus> targetStatus = fileStatus(toolChain_->linkPath(this));
	if (targetStatus->exists()) {
		Time targetTime = targetStatus->lastModified();
		bool targetDirty = false;
		for (int i = 0; i < modules_->length(); ++i) {
			Module *module = modules_->at(i);
			if (fileStatus(module->modulePath())->lastModified() > targetTime) {
				targetDirty = true;
				break;
			}
		}
		hook<FileStatus> recipeStatus = fileStatus(recipe_->path());
		if (recipeStatus->exists()) {
			if (recipeStatus->lastModified() > targetTime) targetDirty = true;
		}
		if (!targetDirty) return buildResult_ = true;
	}

	return buildResult_ = toolChain_->link(this);
}

void BuildPlan::clean()
{
	if (cleanComplete_) return;
	cleanComplete_ = true;

	for (int i = 0; i < prequisites_->length(); ++i)
		prequisites_->at(i)->clean();

	if (options_ & Package) return;

	toolChain_->clean(this);
	unlink(DependencyCache::cachePath(this));
	rmdir(modulePath_);
}

} // namespace mach