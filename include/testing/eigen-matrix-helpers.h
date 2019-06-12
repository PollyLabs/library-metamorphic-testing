#ifndef EIGEN_MATRIX_HELPERS
#define EIGEN_MATRIX_HELPERS

#include "Eigen/Dense"
#include <cassert>
#include<iostream>

Eigen::Matrix3i getIdentity()
{
	Eigen::Matrix3i res;

	for(unsigned int i=0; i < 3; i++)
	{
		for(unsigned int j=0; j < 3; j++)
		{
			if(i == j)
				res(i,j) = 1;
			else
				res(i,j) = 0;
		}
	}

	return res;
}

bool isIdentity(Eigen::Matrix3i m)
{
	for(unsigned int i=0; i < m.rows(); i++)
	{
		for(unsigned int j=0; j < m.cols(); j++)
		{
			if(m(i,j) != 1)
			{
				return false;
			}
		}
	}

	return true;
}

int getMinusOne()
{
	return -1;
}

int getInteger(int x)
{
	return x;
}

/*

Eigen::Matrix3i Inverse(Eigen::Matrix3i m)
{
	if((m.rows() == m.cols()) && (m.determinant() != 0)) // Returns true for a square matrix with a non-zero determinant
		return m.inverse();
	else
		return m;

	#if 0
	unsigned int r, c;

	r = m.rows();
	c = m.cols();

	if(r < c)
	{
		Eigen::Matrix3i t = Eigen::Matrix3i::Random(c,c);

		for(unsigned int i = 0; i < m.rows(); i++)
		{
			for(unsigned int j = 0; j < m.cols(); j++)
			{
				t(i,j) = m(i,j);
			}
		}

		while(t.determinant() != 0)
		{
			t = Eigen::Matrix3i::Random(c,c);
		}

		return t.inverse();
	}
	else if(r > c)
	{
		Eigen::Matrix3i t = Eigen::Matrix3i::Random(r,r);

		for(unsigned int i = 0; i < m.rows(); i++)
		{
			for(unsigned int j = 0; j < m.cols(); j++)
			{
				t(i,j) = m(i,j);
			}
		}

		while(t.determinant() != 0)
		{
			t = Eigen::Matrix3i::Random(r,r);
		}

		return t.inverse();
	}
	else
	{
		while(m.determinant() != 0)
		{
			m = Eigen::Matrix3i::Random(r,r);
		}

		return m.inverse();
	}
	#endif
}
*/

/*
Eigen::Matrix3i addMatrix3i(Eigen::Matrix3i m1, Eigen::Matrix3i m2)
{
//	assert(((m1.rows() == m2.rows()) && (m1.cols() == m2.cols())));
	
	if(((m1.rows() == m2.rows()) && (m1.cols() == m2.cols())))
		return m1.operator+(m2);

	unsigned int r1, r2, c1, c2;

	r1 = m1.rows();
	c1 = m1.cols();

	r2 = m2.rows();
	c2 = m2.cols();

	if(r1 == r2) // Number of rows are identical
	{
		if(c1 < c2) // Matrix m1 has to be resized by changing the number of columns
		{
			Eigen::Matrix3i t = Eigen::Matrix3i::Random(r1, c2);

			for(unsigned int i = 0; i < r1; i++)
			{
				for(unsigned int j = 0; j < c1; j++)
				{
					t(i,j) = m1(i,j);
				}

				for(unsigned int j = c1; j < c2; j++)
				{
					t(i,j) = 0;
				}
			}
		
			return t.operator+(m2);
		}
		else // Matrix m2 has to be resized by changing the number of columns
		{
			Eigen::Matrix3i t = Eigen::Matrix3i::Random(r2, c1);

			for(unsigned int i = 0; i < r2; i++)
			{
				for(unsigned int j = 0; j < c2; j++)
				{
					t(i,j) = m2(i,j);
				}

				for(unsigned int j = c2; j < c1; j++)
				{
					t(i,j) = 0;
				}	
			}
		
			return m1.operator+(t);
		}
	}
	else if(c1 == c2) // Number of columns are identical
	{
		if(r1 < r2) // Matrix m1 has to be resized by changing the number of rows
		{
			Eigen::Matrix3i t = Eigen::Matrix3i::Random(r2, c1);

			for(unsigned int i = 0; i < c1; i++)
			{
				for(unsigned int j = 0; j < r1; j++)
				{
					t(j,i) = m1(j,i);
				}

				for(unsigned int j = r1; j < r2; j++)
				{
					t(j,i) = 0;
				}
			}
		
			return t.operator+(m2);
		}
		else // Matrix m2 has to be resized by changing the number of rows
		{
			Eigen::Matrix3i t = Eigen::Matrix3i::Random(r1, c2);

			for(unsigned int i = 0; i < c2; i++)
			{
				for(unsigned int j = 0; j < r2; j++)
				{
					t(j,i) = m2(j,i);
				}

				for(unsigned int j = r2; j < r1; j++)
				{
					t(j,i) = 0;
				}
			}
		
			return m1.operator+(t);

		}
	}
	else // Dimensions are different
	{
		unsigned int max_r, max_c;

		max_r = (r1 > r2) ? r1 : r2;
		max_c = (c1 > c2) ? c1 : c2;

		Eigen::Matrix3i t1 = Eigen::Matrix3i::Random(max_r, max_c);
		Eigen::Matrix3i t2 = Eigen::Matrix3i::Random(max_r, max_c);

		for(unsigned int i = 0; i < max_r; i++)
		{
			for(unsigned int j = 0; j < max_c; j++)
			{
				t1(i,j) = 0;
			}
		}

		for(unsigned int i = 0; i < r1; i++)
		{
			for(unsigned int j = 0; j < c1; j++)
			{
				t1(i,j) = m1(i,j);
			}
		}

		for(unsigned int i = 0; i < max_r; i++)
		{
			for(unsigned int j = 0; j < max_c; j++)
			{
				t2(i,j) = 0;
			}
		}

		for(unsigned int i = 0; i < r2; i++)
		{
			for(unsigned int j = 0; j < c2; j++)
			{
				t2(i,j) = m2(i,j);
			}
		}

		return t1.operator+(t2);
	}
}

Eigen::Matrix3i subMatrix3i(Eigen::Matrix3i m1, Eigen::Matrix3i m2)
{
	if(((m1.rows() == m2.rows()) && (m1.cols() == m2.cols())))
		return m1.operator-(m2);

	unsigned int r1, r2, c1, c2;

	r1 = m1.rows();
	c1 = m1.cols();

	r2 = m2.rows();
	c2 = m2.cols();

	if(r1 == r2) // Number of rows are identical
	{
		if(c1 < c2) // Matrix m1 has to be resized by changing the number of columns
		{
			Eigen::Matrix3i t = Eigen::Matrix3i::Random(r1, c2);

			for(unsigned int i = 0; i < r1; i++)
			{
				for(unsigned int j = 0; j < c1; j++)
				{
					t(i,j) = m1(i,j);
				}

				for(unsigned int j = c1; j < c2; j++)
				{
					t(i,j) = 0;
				}

			}
		
			return t.operator-(m2);
		}
		else // Matrix m2 has to be resized by changing the number of columns
		{
			Eigen::Matrix3i t = Eigen::Matrix3Matrix3im(r2, c1);

			for(unsigned int i = 0; i < r2; i++)
			{
				for(unsigned int j = 0; j < c2; j++)
				{
					t(i,j) = m2(i,j);
				}

				for(unsigned int j = c2; j < c1; j++)
				{
					t(i,j) = 0;
				}
			}
		
			return m1.operator-(t);
		}
	}
	else if(c1 == c2) // Number of columns are identical
	{
		if(r1 < r2) // Matrix m1 has to be resized by changing the number of rows
		{
			Eigen::Matrix3i t = Eigen::Matrix3i::Random(r2, c1);

			for(unsigned int i = 0; i < c1; i++)
			{
				for(unsigned int j = 0; j < r1; j++)
				{
					t(j,i) = m1(j,i);
				}

				for(unsigned int j = r1; j < r2; j++)
				{
					t(j,i) = 0;
				}
			}
		
			return t.operator-(m2);
		}
		else // Matrix m2 has to be resized by changing the number of rows
		{
			Eigen::Matrix3i t = Eigen::Matrix3i::Random(r1, c2);

			for(unsigned int i = 0; i < c2; i++)
			{
				for(unsigned int j = 0; j < r2; j++)
				{
					t(j,i) = m2(j,i);
				}

				for(unsigned int j = r2; j < r1; j++)
				{
					t(j,i) = 0;
				}

			}
		
			return m1.operator-(t);

		}
	}
	else // Dimensions are different
	{
		unsigned int max_r, max_c;

		max_r = (r1 > r2) ? r1 : r2;
		max_c = (c1 > c2) ? c1 : c2;

		Eigen::Matrix3i t1 = Eigen::Matrix3i::Random(max_r, max_c);
		Eigen::Matrix3i t2 = Eigen::Matrix3i::Random(max_r, max_c);

		for(unsigned int i = 0; i < max_r; i++)
		{
			for(unsigned int j = 0; j < max_c; j++)
			{
				t1(i,j) = 0;
			}
		}

		for(unsigned int i = 0; i < m1.rows(); i++)
		{
			for(unsigned int j = 0; j < m1.cols(); j++)
			{
				t1(i,j) = m1(i,j);
			}
		}

	
		for(unsigned int i = 0; i < max_r; i++)
		{
			for(unsigned int j = 0; j < max_c; j++)
			{
				t2(i,j) = 0;
			}
		}

		for(unsigned int i = 0; i < m2.rows(); i++)
		{
			for(unsigned int j = 0; j < m2.cols(); j++)
			{
				t2(i,j) = m2(i,j);
			}
		}

		return t1.operator-(t2);
	}
}

Eigen::Matrix3i(Eigen::Matrix3i m1, Eigen::Matrix3i m2)
{
	unsigned int cols, rows;

	cols = m1.cols();
	rows = m2.rows();

//	assert(cols == rows);

//	return m1.operator*(m2);

	if(cols == rows)
	{
		return m1.operator*(m2);
	}

	if(isIdentity(m1))
	{
		return m2;
	}
	else if(isIdentity(m2))
	{
		return m1;
	}

	if(cols < rows)
	{
		Eigen::Matrix3i t = Eigen::Matrix3i::Random(m1.rows(), rows);

		for(unsigned int i = 0; i < m1.rows(); i++)
		{
			for(unsigned int j = 0; j < m1.cols(); j++)
			{
				t(i,j) = m1(i,j);
			}

			for(unsigned int j = cols; j < rows; j++)
			{
				t(i,j) = 0;
			}
		}

		return t.operator*(m2);
	}
	else
	{
		Eigen::Matrix3i t = Eigen::Matrix3i::Random(cols, m2.cols());

		for(unsigned int i = 0; i < m2.cols(); i++)
		{
			for(unsigned int j = 0; j < m2.rows(); j++)
			{
				t(j,i) = m2(j,i);
			}

			for(unsigned int j = rows; j < cols; j++)
			{
				t(j,i) = 0;
			}
		}

		return m1.operator*(t);
	}
}
*/

#endif
