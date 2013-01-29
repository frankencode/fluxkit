#ifndef MACH_BUILDPLAN_HPP
#define MACH_BUILDPLAN_HPP

#include <ftl/PrefixTree.hpp>
#include "Module.hpp"
#include "ToolChain.hpp"

namespace ftl {
class FileStatus;
class Config;
} // namespace ftl

namespace mach
{

using namespace ftl;

class BuildPlan;

typedef List< hook<BuildPlan> > BuildPlanList;

FTL_EXCEPTION(BuildPlanException, Exception);

class BuildPlan: public Instance
{
public:
	enum Option {
		Static        = 1,
		Application   = 2,
		Library       = 4,
		ToolSet       = 8,
		Package       = 16,
		Debug         = 32,
		Release       = 64,
		DryRun        = 128,
		Blindfold     = 256,
		Bootstrap     = 512,
		Verbose       = 1024,
		OptimizeSpeed = 2048,
		OptimizeSize  = 4096,
		Unspecified   = -1,
		GlobalOptions = Debug|Release|DryRun|Blindfold|Bootstrap|Verbose|OptimizeSpeed|OptimizeSize
	};

	static hook<BuildPlan> create(int argc, char **argv);

	inline ToolChain *toolChain() const { return toolChain_; }
	inline string projectPath() const { return projectPath_; }
	inline string modulePath() const { return modulePath_; }
	inline Config *recipe() const { return recipe_; }

	inline string name() const { return name_; }
	inline string version() const { return version_; }
	inline int options() const { return options_; }
	inline int speedOptimizationLevel() const { return speedOptimizationLevel_; }
	inline int sizeOptimizationLevel() const { return sizeOptimizationLevel_; }

	inline StringList *includePaths() const { return includePaths_; }
	inline StringList *libraryPaths() const { return libraryPaths_; }
	inline StringList *libraries() const { return libraries_; }
	inline StringList *sources() const { return sources_; }
	inline ModuleList *modules() const { return modules_; }

	inline BuildPlanList *prequisites() const { return prequisites_; }

	int run();

	string sourcePath(string source) const;
	string modulePath(string object) const;

	string beautifyCommand(string command);
	bool runBuild(string command);

	bool mkdir(string path);
	bool rmdir(string path);
	bool symlink(string path, string newPath);
	bool unlink(string path);
	hook<FileStatus> fileStatus(string path);

private:
	hook<BuildPlan> create(ToolChain *toolChain, string projectPath, int globalOptions);

	typedef PrefixTree<char, BuildPlan *> BuildMap;

	BuildPlan(int argc, char **argv);
	BuildPlan(ToolChain *toolChain, string projectPath, BuildPlan *parentPlan);

	void readRecipe(BuildPlan *parentPlan = 0);

	void prepare();
	bool analyse();
	bool build();
	void clean();

	hook<ToolChain> toolChain_;
	string projectPath_;
	string modulePath_;
	hook<Config> recipe_;

	string name_;
	string version_;
	int options_;
	int speedOptimizationLevel_;
	int sizeOptimizationLevel_;

	hook<StringList> includePaths_;
	hook<StringList> libraryPaths_;
	hook<StringList> libraries_;
	hook<StringList> sources_;
	hook<ModuleList> modules_;

	hook<BuildPlanList> prequisites_;

	hook<BuildMap> buildMap_;
	string sourcePrefix_;

	bool prepareComplete_;
	bool analyseComplete_;
	bool buildComplete_;
	bool cleanComplete_;
	bool distCleanComplete_;

	bool analyseResult_;
	bool buildResult_;
};

} // namespace mach

#endif // MACH_BUILDPLAN_HPP