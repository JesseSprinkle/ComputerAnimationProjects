#include "OptimizerNM.h"
#include "Objective.h"
#include <math.h>

using namespace std;
using namespace Eigen;

OptimizerNM::OptimizerNM() :
	tol(1e-3),
	iterMax(100),
	iter(0)
{

}

OptimizerNM::OptimizerNM(int iterations) :
	tol(1e-3),
	iterMax(iterations),
	iter(0)
{

}

OptimizerNM::~OptimizerNM()
{

}

VectorXd OptimizerNM::optimize(const shared_ptr<Objective> objective, const VectorXd& xInit)
{
	int n = xInit.rows();
	VectorXd g(n);
	MatrixXd H(n, n);
	iter = 0;
	VectorXd x = xInit;
	while (iter < iterMax)
	{
		for (int i = 0; i < n; i++)
		{
			while (x(i) > 3.141592653589793238463)
			{
				x(i) -= 2 * 3.141592653589793238463;
			}
			while (x(i) < -3.141592653589793238463)
			{
				x(i) += 2 * 3.141592653589793238463;
			}
		}

		double val = objective->evalObjective(x, g, H);
		VectorXd dx = -(H.ldlt().solve(g));

		x += dx;
		iter++;
		if (dx.norm() < tol)
		{
			break;
		}
	}

	return x;
}
