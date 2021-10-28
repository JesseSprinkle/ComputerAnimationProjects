#pragma once
#ifndef OBJECTIVE_LAB_H
#define OBJECTIVE_LAB_H
#include <Eigen/Dense>

class Objective
{
private:
	const double wtar = 1000;
	const double wreg = 1;
	Eigen::Vector2d goal;
	Eigen::Vector2d calculate_dp(const std::vector<Eigen::Matrix3d>& matrices) const;
	double calculate_f(const Eigen::Vector2d& dp, const Eigen::VectorXd& thetas) const;
	void calculate_g(const Eigen::Vector2d& dp, const Eigen::MatrixXd& partials, const Eigen::VectorXd& thetas, Eigen::VectorXd& g) const;
	void calculate_partials_matrix(const std::vector<Eigen::Matrix3d>& Rs, const std::vector<Eigen::Matrix3d>& dRs, Eigen::MatrixXd& partials) const;
public:
	Objective(Eigen::Vector2d goal);
	virtual ~Objective();
	virtual double evalObjective(const Eigen::VectorXd &x) const;
	virtual double evalObjective(const Eigen::VectorXd &x, Eigen::VectorXd &g) const;
	virtual double evalObjective(const Eigen::VectorXd &x, Eigen::VectorXd &g, Eigen::MatrixXd &H) const;
};

#endif
