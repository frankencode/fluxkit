/*
 * Copyright (C) 2007-2013 Frank Mertens.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef FMAKE_BUILDSTAGE_H
#define FMAKE_BUILDSTAGE_H

namespace fmake
{

class BuildPlan;
class BuildShell;
class ToolChain;

class BuildStage
{
public:
	inline bool complete() const { return complete_; }
	inline bool success() const { return success_; }
	inline int status() const { return status_; }

protected:
	inline BuildPlan *plan() const { return plan_; }
	BuildShell *shell() const;
	ToolChain *toolChain() const;

	BuildStage(BuildPlan *plan)
		: plan_(plan),
		  complete_(false),
		  success_(false),
		  status_(0)
	{}

	BuildPlan *plan_;
	bool complete_, success_;
	int status_;
};

} // namespace fmake

#endif // FMAKE_BUILDSTAGE_H
