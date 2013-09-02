#ifndef FCLAIM_SHHEADERSCANNER_H
#define FCLAIM_SHHEADERSCANNER_H

#include "HeaderScanner.h"

namespace fclaim
{

using namespace fkit;

class ShHeaderScannerInitializer
{
public:
	ShHeaderScannerInitializer();
private:
	static int count_;
};

namespace { ShHeaderScannerInitializer initializer; }

class ShHeaderSyntax;

class ShHeaderScanner: public HeaderScanner
{
public:
	Ref<Header> scanHeader(String path) const;

private:
	friend class ShHeaderScannerInitializer;
	ShHeaderScanner();
	Ref<ShHeaderSyntax> headerSyntax_;
};

} // namespace fclaim

#endif // FCLAIM_SHHEADERSCANNER_H