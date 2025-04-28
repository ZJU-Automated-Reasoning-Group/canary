#include "Alias/FSCS/Program/CFG/CFGNode.h"
#include "Alias/FSCS/Support/FunctionContext.h"
#include "Alias/FSCS/Support/ProgramPoint.h"
#include "Alias/FSCS/IO/Printer.h"
#include "Alias/FSCS/IO/NodePrinter.h"

#include <llvm/IR/Function.h>

namespace util
{
namespace io
{

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::CFGNode& node)
{
	NodePrinter<tpa::CFGNode>(os).visit(node);
	return os;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::ProgramPoint& pp)
{
	os << "(" << *pp.getContext() << ", " << *pp.getCFGNode() << ")";
	return os;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::FunctionContext& fc)
{
	os << "(" << *fc.getContext() << ", " << fc.getFunction()->getName() << ")";
	return os;
}

}
}