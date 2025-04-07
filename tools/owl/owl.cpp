#include <memory>
#include <stdio.h>
#include <stdlib.h>

#include "Solvers/SMT/SMTSolver.h"
#include "Solvers/SMT/SMTFactory.h"
#include "Solvers/SMT/CNF.h"
#include "Solvers/SMT/SATSolver.h"

#ifdef _MSC_VER
#include <ctime>
double _get_cpu_time(){
  return (double) clock() / CLOCKS_PER_SEC;
}
#else

#include <sys/time.h>

#endif

using namespace std;


int solveCNF(int argc, char **argv) {
	if (argc < 2)
		exit(0);

	cnf *m_cnf = new cnf(argv[1]);
	fflush(stdout);
	sat_solver solver(*m_cnf);
	delete m_cnf;
	bool result = solver.run();
	if (result) {
		printf("s sat\n");
	} else
		printf("s unsat\n");
	exit(result ? 10 : 20);
}

int main(int argc, char **argv) {
	SMTFactory Ft;
	SMTSolver Sol = Ft.createSMTSolver();
	Sol.add(Ft.parseSMTLib2File(argv[1]));
	std::cout << Sol.check() << std::endl;
	return 0;
}

