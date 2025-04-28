#include "Alias/FSCS/Context/Context.h"
#include "Alias/FSCS/Context/ProgramPoint.h"
#include "Alias/FSCS/IO/Context/Printer.h"

#include <llvm/Support/raw_ostream.h>

namespace util
{
namespace io
{

llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const context::Context& ctx)
{
	if (ctx.isGlobalContext())
		os << "[GLOBAL]";
	else
		os << "[CTX(" << ctx.size() << ") " << &ctx << "]";
	return os;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const context::ProgramPoint& p)
{
	os << "(" << *p.getContext() << ", " << *p.getInstruction() << ")";
	return os;
}

}
}