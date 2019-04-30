#ifndef EIGEN_MATRIX_HELPERS
#define EIGEN_MATRIX_HELPERS

#include "Eigen/Dense"

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

#endif
