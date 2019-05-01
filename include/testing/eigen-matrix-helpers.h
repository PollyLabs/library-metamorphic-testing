#ifndef EIGEN_MATRIX_HELPERS
#define EIGEN_MATRIX_HELPERS

#include "Eigen/Dense"
#include <cassert>

Eigen::MatrixXf getIdentity()
{
	Eigen::MatrixXf res(3,3);

	for(unsigned int i=0; i < 3; i++)
	{
		for(unsigned int j=0; j < 3; j++)
		{
			res(i,j) = 1;
		}
	}

	return res;
}

int getMinusOne()
{
	return -1;
}

int getInteger(int x)
{
	return x;
}

bool isInvertible(Eigen::MatrixXf m)
{
	return (m.determinant() != 0);
}

Eigen::MatrixXf Inverse(Eigen::MatrixXf m)
{
	assert(isInvertible(m));

	return m.inverse();
}

#endif
