#include "Objective.h"
#include <cmath>
#include <vector>
#include <memory>
#include <iostream>
using namespace Eigen;

Objective::Objective(Vector2d _goal)
{
	goal = _goal;
}

Objective::~Objective()
{
	
}


Vector2d Objective::calculate_dp(const std::vector<Matrix3d>& matrices) const
{
	Vector3d r(1, 0, 1);
	Matrix3d T = Matrix3d::Identity();
	T(0, 2) = 1;
	for (int i = matrices.size() - 1; i >= 0; i--)
	{
		r = matrices[i] * r;
		if (i != 0)
		{
			r = T * r;
		}
	}
	return Vector2d(r(0) - goal(0), r(1) - goal(1));
}

void Objective::calculate_partials_matrix(const std::vector<Matrix3d>& Rs, const std::vector<Matrix3d>& dRs, MatrixXd& partials) const
{
	int n = Rs.size();
	partials = MatrixXd(2, n);
	Matrix3d T = Matrix3d::Identity();
	T(0, 2) = 1;
	for (int i = 0; i < n; i++)
	{
		Vector3d r = Vector3d(1, 0, 1);
		for (int j = n - 1; j >= 0; j--)
		{
			if (j != i)
			{
				r = (Rs[j] * r);
			}
			else
			{
				r = (dRs[j] * r);
			}
			if (j != 0)
			{
				r = T * r;
			}
		}
		partials(0, i) = r(0);
		partials(1, i) = r(1);

	}
}

double Objective::calculate_f(const Vector2d& dp, const VectorXd& thetas) const
{
	return (wtar / 2.0) * dp.dot(dp) + (wreg / 2.0) * thetas.dot(thetas);
}

void Objective::calculate_g(const Vector2d& dp, const MatrixXd& partials, const VectorXd& thetas, VectorXd& g) const
{
	g = wtar * (dp.transpose() * partials).transpose() + wreg * thetas;
}

double Objective::evalObjective(const VectorXd &thetas) const
{
	
	int n = thetas.rows();
	std::vector<Matrix3d> matrices(n);
	for (int i = 0; i < n; i++)
	{
		matrices[i] = Matrix3d::Identity();
		matrices[i](0, 0) = cos(thetas(i));
		matrices[i](0, 1) = -sin(thetas(i));
		matrices[i](1, 0) = sin(thetas(i));
		matrices[i](1, 1) = cos(thetas(i));
	}
	Vector2d dp = calculate_dp(matrices);

	return calculate_f(dp, thetas);
}

double Objective::evalObjective(const VectorXd &thetas, VectorXd &g) const
{
	
	int n = thetas.rows();
	// generate matrices
	std::vector<Matrix3d> Rs(n);
	std::vector<Matrix3d> dRs(n);
	for (int i = 0; i < n; i++)
	{
		double tcos = cos(thetas(i));
		double tsin = sin(thetas(i));
		Rs[i] = Matrix3d::Identity();
		Rs[i](0, 0) = tcos;
		Rs[i](0, 1) = -tsin;
		Rs[i](1, 0) = tsin;
		Rs[i](1, 1) = tcos;

		dRs[i] = Matrix3d::Zero();
		dRs[i](0, 0) = -tsin;
		dRs[i](0, 1) = -tcos;
		dRs[i](1, 0) = tcos;
		dRs[i](1, 1) = -tsin;
	}

	// calculate delta P
	Vector2d dp = calculate_dp(Rs);

	// calculate partial derives matrix
	MatrixXd partials;
	calculate_partials_matrix(Rs, dRs, partials);

	calculate_g(dp, partials, thetas, g);

	return calculate_f(dp, thetas);
}

double Objective::evalObjective(const VectorXd &thetas, VectorXd &g, MatrixXd &H) const
{
	int n = thetas.rows();

	// generate matrices
	std::vector<Matrix3d> Rs(n);
	std::vector<Matrix3d> dRs(n);
	std::vector<Matrix3d> ddRs(n);
	for (int i = 0; i < n; i++)
	{
		double tcos = cos(thetas(i));
		double tsin = sin(thetas(i));
		Rs[i] = Matrix3d::Identity();
		Rs[i](0, 0) = tcos;
		Rs[i](0, 1) = -tsin;
		Rs[i](1, 0) = tsin;
		Rs[i](1, 1) = tcos;

		dRs[i] = Matrix3d::Zero();
		dRs[i](0, 0) = -tsin;
		dRs[i](0, 1) = -tcos;
		dRs[i](1, 0) = tcos;
		dRs[i](1, 1) = -tsin;

		ddRs[i] = Matrix3d::Zero();
		ddRs[i](0, 0) = -tcos;
		ddRs[i](0, 1) = tsin;
		ddRs[i](1, 0) = -tsin;
		ddRs[i](1, 1) = -tcos;
	}

	// calculate delta P
	Vector2d dp = calculate_dp(Rs);

	// calculate partial derives matrix
	MatrixXd partials;
	calculate_partials_matrix(Rs, dRs, partials);

	calculate_g(dp, partials, thetas, g);


	Matrix3d T = Matrix3d::Identity();
	T(0, 2) = 1;
	// calculate ddP matrix
	MatrixXd ddP(2 * n, n);
	// for each single partial derivative
	for (int i = 0; i < n; i++)
	{
		// for each partial derivative after taking the partial derivative
		for (int j = 0; j < n; j++)
		{
			Vector3d r = Vector3d(1, 0, 1);
			// multiply matricies 
			for (int k = n - 1; k >= 0; k--)
			{
				if (i == j && j == k)
				{
					r = ddRs[k] * r;
				}
				else if (i == k || j == k)
				{
					r = dRs[k] * r;
				}
				else
				{
					r = Rs[k] * r;
				}

				if (k != 0)
				{
					r = T * r;
				}
			}
			ddP(i * 2, j) = r(0);
			ddP(i * 2 + 1, j) = r(1);
		}
	}

	
	MatrixXd weirdMatrix(n, n);
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			double val = 0;
			for (int k = 0; k < 2; k++)
			{
				val += dp(k) * ddP(i * 2 + k, j);
			}
			weirdMatrix(i, j) = val;
		}
	}
	H = wtar * (partials.transpose() * partials + weirdMatrix) + wreg * MatrixXd::Identity(n, n);
	return calculate_f(dp, thetas);
}
