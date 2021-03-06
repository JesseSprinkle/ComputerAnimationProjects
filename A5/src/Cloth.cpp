#include <iostream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Cloth.h"
#include "Particle.h"
#include "Spring.h"
#include "MatrixStack.h"
#include "Program.h"
#include "GLSL.h"

using namespace std;
using namespace Eigen;

shared_ptr<Spring> createSpring(const shared_ptr<Particle> p0, const shared_ptr<Particle> p1, double E)
{
	auto s = make_shared<Spring>(p0, p1);
	s->E = E;
	Vector3d x0 = p0->x;
	Vector3d x1 = p1->x;
	Vector3d dx = x1 - x0;
	s->L = dx.norm();
	return s;
}

Cloth::Cloth(int rows, int cols,
			 const Vector3d &x00,
			 const Vector3d &x01,
			 const Vector3d &x10,
			 const Vector3d &x11,
			 double mass,
			 double stiffness)
{
	assert(rows > 1);
	assert(cols > 1);
	assert(mass > 0.0);
	assert(stiffness > 0.0);
	
	this->rows = rows;
	this->cols = cols;
	
	// Create particles
	n = 0;
	double r = 0.02; // Used for collisions
	int nVerts = rows*cols;
	for(int i = 0; i < rows; ++i) {
		double u = i / (rows - 1.0);
		Vector3d x0 = (1 - u)*x00 + u*x10;
		Vector3d x1 = (1 - u)*x01 + u*x11;
		for(int j = 0; j < cols; ++j) {
			double v = j / (cols - 1.0);
			Vector3d x = (1 - v)*x0 + v*x1;
			auto p = make_shared<Particle>();
			particles.push_back(p);
			p->r = r;
			p->x = x;
			p->v << 0.0, 0.0, 0.0;
			p->m = mass/(nVerts);
			// Pin two particles
			if(i == 0 && (j == 0 || j == cols-1)) {
				p->fixed = true;
				p->i = -1;
			} else {
				p->fixed = false;
				p->i = n;
				n += 3;
			}
		}
	}
	
	// Create x springs
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols-1; ++j) {
			int k0 = i*cols + j;
			int k1 = k0 + 1;
			springs.push_back(createSpring(particles[k0], particles[k1], stiffness));
		}
	}
	
	// Create y springs
	for(int j = 0; j < cols; ++j) {
		for(int i = 0; i < rows-1; ++i) {
			int k0 = i*cols + j;
			int k1 = k0 + cols;
			springs.push_back(createSpring(particles[k0], particles[k1], stiffness));
		}
	}
	
	// Create shear springs
	for(int i = 0; i < rows-1; ++i) {
		for(int j = 0; j < cols-1; ++j) {
			int k00 = i*cols + j;
			int k10 = k00 + 1;
			int k01 = k00 + cols;
			int k11 = k01 + 1;
			springs.push_back(createSpring(particles[k00], particles[k11], stiffness));
			springs.push_back(createSpring(particles[k10], particles[k01], stiffness));
		}
	}
	
	// Create x bending springs
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols-2; ++j) {
			int k0 = i*cols + j;
			int k2 = k0 + 2;
			springs.push_back(createSpring(particles[k0], particles[k2], stiffness));
		}
	}
	
	// Create y bending springs
	for(int j = 0; j < cols; ++j) {
		for(int i = 0; i < rows-2; ++i) {
			int k0 = i*cols + j;
			int k2 = k0 + 2*cols;
			springs.push_back(createSpring(particles[k0], particles[k2], stiffness));
		}
	}

	// Build system matrices and vectors
	M.resize(n,n);
	K.resize(n,n);
	v.resize(n);
	f.resize(n);
	
	// Build vertex buffers
	posBuf.clear();
	norBuf.clear();
	texBuf.clear();
	eleBuf.clear();
	posBuf.resize(nVerts*3);
	norBuf.resize(nVerts*3);
	updatePosNor();

	// Texture coordinates (don't change)
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			texBuf.push_back(i/(rows-1.0));
			texBuf.push_back(j/(cols-1.0));
		}
	}

	// Elements (don't change)
	for(int i = 0; i < rows-1; ++i) {
		for(int j = 0; j < cols; ++j) {
			int k0 = i*cols + j;
			int k1 = k0 + cols;
			// Triangle strip
			eleBuf.push_back(k0);
			eleBuf.push_back(k1);
		}
	}
}

Cloth::~Cloth()
{
}

void Cloth::tare()
{
	for(int k = 0; k < (int)particles.size(); ++k) {
		particles[k]->tare();
	}
}

void Cloth::reset()
{
	for(int k = 0; k < (int)particles.size(); ++k) {
		particles[k]->reset();
	}
	updatePosNor();
}

void Cloth::updatePosNor()
{
	// Position
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			int k = i*cols + j;
			Vector3d x = particles[k]->x;
			posBuf[3*k+0] = x(0);
			posBuf[3*k+1] = x(1);
			posBuf[3*k+2] = x(2);
		}
	}
	
	// Normal
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			// Each particle has four neighbors
			//
			//      v1
			//     /|\
			// u0 /_|_\ u1
			//    \ | /
			//     \|/
			//      v0
			//
			// Use these four triangles to compute the normal
			int k = i*cols + j;
			int ku0 = k - 1;
			int ku1 = k + 1;
			int kv0 = k - cols;
			int kv1 = k + cols;
			Vector3d x = particles[k]->x;
			Vector3d xu0, xu1, xv0, xv1, dx0, dx1, c;
			Vector3d nor(0.0, 0.0, 0.0);
			int count = 0;
			// Top-right triangle
			if(j != cols-1 && i != rows-1) {
				xu1 = particles[ku1]->x;
				xv1 = particles[kv1]->x;
				dx0 = xu1 - x;
				dx1 = xv1 - x;
				c = dx0.cross(dx1);
				nor += c.normalized();
				++count;
			}
			// Top-left triangle
			if(j != 0 && i != rows-1) {
				xu1 = particles[kv1]->x;
				xv1 = particles[ku0]->x;
				dx0 = xu1 - x;
				dx1 = xv1 - x;
				c = dx0.cross(dx1);
				nor += c.normalized();
				++count;
			}
			// Bottom-left triangle
			if(j != 0 && i != 0) {
				xu1 = particles[ku0]->x;
				xv1 = particles[kv0]->x;
				dx0 = xu1 - x;
				dx1 = xv1 - x;
				c = dx0.cross(dx1);
				nor += c.normalized();
				++count;
			}
			// Bottom-right triangle
			if(j != cols-1 && i != 0) {
				xu1 = particles[kv0]->x;
				xv1 = particles[ku1]->x;
				dx0 = xu1 - x;
				dx1 = xv1 - x;
				c = dx0.cross(dx1);
				nor += c.normalized();
				++count;
			}
			nor /= count;
			nor.normalize();
			norBuf[3*k+0] = nor(0);
			norBuf[3*k+1] = nor(1);
			norBuf[3*k+2] = nor(2);
		}
	}
}

void Cloth::step(double h, const Vector3d &grav, const vector< shared_ptr<Particle> > spheres)
{
	// collision stiffness
	const double c = 1e1;
	
	// store previous velocity to help solve later
	VectorXd pv = v;
	v.setZero();
	f.setZero();
	

	vector<Triplet<double>> kTrips;
	vector<Triplet<double>> mTrips;
	for (int i = 0; i < particles.size(); i++)
	{
		// forces spheres have on cloth. store K matrix triplets
		for (int j = 0; j < spheres.size(); j++)
		{
			Vector3d dx = particles[i]->x - spheres[j]->x;
			double l = dx.norm();
			double d = particles[i]->r + spheres[j]->r - l;
			if (d > 0)
			{
				f.segment(particles[i]->i, 3) += c * d * dx / l;
				kTrips.push_back(Triplet<double>(particles[i]->i, particles[i]->i, c * d));
				kTrips.push_back(Triplet<double>(particles[i]->i + 1, particles[i]->i + 1, c * d));
				kTrips.push_back(Triplet<double>(particles[i]->i + 2, particles[i]->i + 2, c * d));
			}
		}
		
		// make mass matrix triplets, set gravity and initial velocity
		if (!particles[i]->fixed)
		{
			f.segment(particles[i]->i, 3) += particles[i]->m * grav;
			v.segment(particles[i]->i, 3) = particles[i]->v;
			mTrips.push_back(Triplet<double>(particles[i]->i, particles[i]->i, particles[i]->m));
			mTrips.push_back(Triplet<double>(particles[i]->i + 1, particles[i]->i + 1, particles[i]->m));
			mTrips.push_back(Triplet<double>(particles[i]->i + 2, particles[i]->i + 2, particles[i]->m));
		}
	}

	M.setFromTriplets(mTrips.begin(), mTrips.end());


	// get spring forces between particles
	for (int i = 0; i < springs.size(); i++)
	{
		shared_ptr<Particle> p0 = springs[i]->p0;
		shared_ptr<Particle> p1 = springs[i]->p1;
		Vector3d dx = p1->x - p0->x;

		// spring force between particles
		double l = dx.norm();
		double lscale = (l - springs[i]->L) / l;
		VectorXd fs = springs[i]->E * lscale * dx;
		if (!p0->fixed)
		{
			f.segment(p0->i, 3) += fs;
		}
		if (!p1->fixed)
		{
			f.segment(p1->i, 3) += -fs;
		}

		// k matrix stuff
		MatrixXd Ks = (springs[i]->E / (l * l)) * ((1 - lscale) * (dx * dx.transpose()) + (lscale * (dx.dot(dx))) * Matrix3d::Identity());
		if (!p0->fixed)
		{
			for (int j = 0; j < Ks.rows(); j++)
			{
				for (int k = 0; k < Ks.cols(); k++)
				{
					kTrips.push_back(Triplet<double>(p0->i + j, p0->i + k, -Ks(j, k)));
				}
			}
		}

		if (!p1->fixed)
		{
			for (int j = 0; j < Ks.rows(); j++)
			{
				for (int k = 0; k < Ks.cols(); k++)
				{
					kTrips.push_back(Triplet<double>(p1->i + j, p1->i + k, -Ks(j, k)));
				}
			}
		}

		if (!p0->fixed && !p1->fixed)
		{

			for (int j = 0; j < Ks.rows(); j++)
			{
				for (int k = 0; k < Ks.cols(); k++)
				{
					kTrips.push_back(Triplet<double>(p0->i + j, p1->i + k, Ks(j, k)));
					kTrips.push_back(Triplet<double>(p1->i + j, p0->i + k, Ks(j, k)));
				}
			}
		}
	}

	K.setFromTriplets(kTrips.begin(), kTrips.end());

	// solve sparse matrices
	VectorXd b = M * v + h * f;
	ConjugateGradient< SparseMatrix<double> > cg;
	cg.setMaxIterations(25);
	cg.setTolerance(1e-6);
	cg.compute(M - (h * h) * K);
	v = cg.solveWithGuess(b, pv);

	// set new position and velocity of particles
	for (int i = 0; i < particles.size(); i++)
	{
		if (!particles[i]->fixed)
		{
			particles[i]->v = v.segment(particles[i]->i, 3);
			particles[i]->x = particles[i]->x + particles[i]->v * h;
		}
	}

	// Update position and normal buffers
	updatePosNor();
}

void Cloth::init()
{
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
	
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_DYNAMIC_DRAW);
	
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	
	glGenBuffers(1, &eleBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf.size()*sizeof(unsigned int), &eleBuf[0], GL_STATIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	assert(glGetError() == GL_NO_ERROR);
}

void Cloth::draw(shared_ptr<MatrixStack> MV, const shared_ptr<Program> p) const
{
	// Draw mesh
	glUniform3fv(p->getUniform("kdFront"), 1, Vector3f(1.0, 0.0, 0.0).data());
	glUniform3fv(p->getUniform("kdBack"),  1, Vector3f(1.0, 1.0, 0.0).data());
	MV->pushMatrix();
	glUniformMatrix4fv(p->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	int h_pos = p->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	int h_nor = p->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID);
	for(int i = 0; i < rows; ++i) {
		glDrawElements(GL_TRIANGLE_STRIP, 2*cols, GL_UNSIGNED_INT, (const void *)(2*cols*i*sizeof(unsigned int)));
	}
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	MV->popMatrix();
}
