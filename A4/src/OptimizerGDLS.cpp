#include "OptimizerGDLS.h"
#include "Objective.h"
#include <iostream>

using namespace std;
using namespace Eigen;

OptimizerGDLS::OptimizerGDLS() :
	alphaInit(1e-4),
	gamma(0.5),
	tol(1e-3),
	iterMax(100),
	iter(0)
{

}

OptimizerGDLS::OptimizerGDLS(int iterations) :
	alphaInit(1e-4),
	gamma(0.5),
	tol(1e-3),
	iterMax(iterations),
	iter(0)
{

}

OptimizerGDLS::~OptimizerGDLS()
{

}

VectorXd OptimizerGDLS::optimize(const shared_ptr<Objective> objective, const VectorXd& xInit)
{
	int n = xInit.rows();
	VectorXd x = xInit;
	VectorXd g(n);
	MatrixXd H(n, n);
	iter = 0;
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

		double val = objective->evalObjective(x, g);
		double alpha = alphaInit;
		VectorXd dxGDLS;
		for (int i = 0; i < iterMax; i++)
		{
			dxGDLS = -alpha * g;
			double newval = objective->evalObjective(x + dxGDLS);
			if (newval < val)
			{
				break;
			}
			alpha *= gamma;
		}

		x += dxGDLS;
		iter++;
		if (dxGDLS.norm() < tol)
		{
			break;
		}
	}
	return x;
}
