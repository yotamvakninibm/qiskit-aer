/*
 * CSVD.cpp
 *
 * Adapted from: P. A. Businger and G. H. Golub, Comm. ACM 12, 564 (1969)
 *
 *
 *
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <complex>
#include <cassert>
#include "SVD.hpp"

#define mul_factor 1e2
#define tiny_factor 1e30
#define THRESHOLD 1e-9
#define NUM_SVD_TRIES 15

using namespace std;
using namespace AER;

cmatrix_t diag(rvector_t S, uint_t m, uint_t n);

cmatrix_t diag(rvector_t S, uint_t m, uint_t n)
{
	cmatrix_t Res = cmatrix_t(m, n);
	for(uint_t i = 0; i < m; i++)
	{
		for(uint_t j = 0; j < n; j++)
		{
			Res(i,j) = (i==j ? complex_t(S[i]) : 0);
		}
	}
	return Res;
}

cmatrix_t reshape_before_SVD(vector<cmatrix_t> data)
{
//	Turns 4 matrices A0,A1,A2,A3 to big matrix:
//	A0 A1
//	A2 A3
//	if(DEBUG) cout << "started first&second concatenate" << endl;
	cmatrix_t temp1 = AER::Utils::concatenate(data[0], data[1], 1),
		  temp2 = AER::Utils::concatenate(data[2], data[3], 1);
//	if(DEBUG) cout << "started third concatenate" << endl;
	return AER::Utils::concatenate(temp1, temp2 ,0) ;
}
vector<cmatrix_t> reshape_U_after_SVD(const cmatrix_t U)
{
	vector<cmatrix_t> Res(2);
	AER::Utils::split(U, Res[0], Res[1] ,0);
	return Res;
}
vector<cmatrix_t> reshape_V_after_SVD(const cmatrix_t V)
{
	vector<cmatrix_t> Res(2);
	AER::Utils::split(AER::Utils::dagger(V), Res[0], Res[1] ,1);
	return Res;
}

// added cut-off at the end
status csvd(cmatrix_t &A, cmatrix_t &U,rvector_t &S,cmatrix_t &V)
{
	int m = A.GetRows(), n = A.GetColumns(), size = max(m,n);
	rvector_t b(size,0.0), c(size,0.0), t(size,0.0);
	double cs = 0.0, eps = 0.0, f = 0.0 ,g = 0.0, h = 0.0, sn = 0.0 , w = 0.0, x = 0.0, y = 0.0, z = 0.0;
	double eta = 1e-10, tol = 1.5e-34;
	// using int and not uint_t because uint_t caused bugs in loops with condition of >= 0
	int i = 0, j = 0, k = 0, k1 = 0, l = 0, l1 = 0;
	complex_t q = 0;
	// Transpose when m < n
    bool transposed = false;
    if (m < n)
    {
    	transposed = true;
    	A = AER::Utils::dagger(A);
    	swap(m,n);
    }
	cmatrix_t temp_A = A;
	c[0] = 0;
	while(true)
	{
		k1 = k + 1;
		z = 0.0;
		for( i = k; i < m; i++){
			z = z + norm(A(i,k));
		}
		b[k] = 0.0;
		if ( tol < z )
		{
			z = sqrt( z );
			b[k] = z;
			w = abs( A(k,k) );
			if ( w == 0.0 ) {
				q = complex_t( 1.0, 0.0 );
			}
			else {
				q = A(k,k) / w;
			}
			A(k,k) = q * ( z + w );

			if ( k != n - 1 )
			{
				for( j = k1; j < n ; j++)
				{

					q = complex_t( 0.0, 0.0 );
					for( i = k; i < m; i++){
						q = q + conj( A(i,k) ) * A(i,j);
					}
					q = q / ( z * ( z + w ) );

					for( i = k; i < m; i++){
					  A(i,j) = A(i,j) - q * A(i,k);
					}

				}
//
// Phase transformation.
//
				q = -conj(A(k,k))/abs(A(k,k));

				for( j = k1; j < n; j++){
					A(k,j) = q * A(k,j);
				}
			}
        }
		if ( k == n - 1 ) break;

		z = 0.0;
		for( j = k1; j < n; j++){
			z = z + norm(A(k,j));
		}
		c[k1] = 0.0;

		if ( tol < z )
		{
			z = sqrt( z );
			c[k1] = z;
			w = abs( A(k,k1) );

			if ( w == 0.0 ){
				q = complex_t( 1.0, 0.0 );
			}
			else{
				q = A(k,k1) / w;
			}
			A(k,k1) = q * ( z + w );

			for( i = k1; i < m; i++)
			{
				q = complex_t( 0.0, 0.0 );

				for( j = k1; j < n; j++){
					q = q + conj( A(k,j) ) * A(i,j);
				}
				q = q / ( z * ( z + w ) );

				for( j = k1; j < n; j++){
					A(i,j) = A(i,j) - q * A(k,j);
				}
			}
//
// Phase transformation.
//
			q = -conj(A(k,k1) )/abs(A(k,k1));
			for( i = k1; i < m; i++){
				A(i,k1) = A(i,k1) * q;
			}
		}
		k = k1;
    }

    eps = 0.0;
	for( k = 0; k < n; k++)
	{
		S[k] = b[k];
		t[k] = c[k];
//		cout << "S[" << k << "] = " << S[k] << endl;
//		cout << "t[" << k << "] = " << t[k] << endl;
		eps = max( eps, S[k] + t[k] );
	}
	eps = eps * eta;

	if(DEBUG) cout << "initializing U, V" << endl;
//
// Initialization of U and V.
//
	U.initialize(m, m);
	V.initialize(n, n);
	for( j = 0; j < m; j++)
	{
		for( i = 0; i < m; i++){
			U(i,j) = complex_t( 0.0, 0.0 );
		}
		U(j,j) = complex_t( 1.0, 0.0 );
	}

	for( j = 0; j < n; j++)
	{
		for( i = 0; i < n; i++){
			V(i,j) = complex_t( 0.0, 0.0 );
		}
		V(j,j) = complex_t( 1.0, 0.0 );
	}



	for( k = n-1; k >= 0; k--)
	{
		if(DEBUG) cout << "k = " << k << endl;
//		int while_runs = 0;
		while(true)
		{
			if(DEBUG) cout << "in while(true) " << endl;
			bool jump = false;
			for( l = k; l >= 0; l--)
			{

				if ( abs( t[l] ) < eps )
				{
					jump = true;
					break;
				}
				else if ( abs( S[l-1] ) < eps ) {
					break;
				}
			}
			if(!jump)
			{
				cs = 0.0;
				sn = 1.0;
				l1 = l - 1;

				for( i = l; i <= k; i++)
				{
					f = sn * t[i];
					t[i] = cs * t[i];

					if ( abs(f) < eps ) {
						break;
					}
					h = S[i];
					w = sqrt( f * f + h * h );
					S[i] = w;
					cs = h / w;
					sn = - f / w;

					for( j = 0; j < n; j++)
					{
						x = real( U(j,l1) );
						y = real( U(j,i) );
						U(j,l1) = complex_t( x * cs + y * sn, 0.0 );
						U(j,i)  = complex_t( y * cs - x * sn, 0.0 );
					}
				}
			}
			w = S[k];
			if ( l == k ){
				break;
			}
			x = S[l];
			y = S[k-1];
			g = t[k-1];
			h = t[k];
			f = ( ( y - w ) * ( y + w ) + ( g - h ) * ( g + h ) )/ ( 2.0 * h * y );
			g = sqrt( f * f + 1.0 );
			if ( f < -1.0e-13){ //if ( f < 0.0){ //didn't work when f was negative very close to 0 (because of numerical reasons)
				g = -g;
			}
			f = ( ( x - w ) * ( x + w ) + ( y / ( f + g ) - h ) * h ) / x;
			cs = 1.0;
			sn = 1.0;
			l1 = l + 1;
			for( i = l1; i <= k; i++)
			{
				g = t[i];
				y = S[i];
				h = sn * g;
				g = cs * g;
				w = sqrt( h * h + f * f );
//				cout << "h = "<<h << " f = "<<f<<endl;
				if (w == 0) {
				  cout << "ERROR 1: w is exactly 0: h = " << h << " , f = " << f << endl;
				  cout << " w = " << w << endl;
//				  assert(false);
				}
				t[i-1] = w;
				cs = f / w;
				sn = h / w;
				f = x * cs + g * sn; // might be 0

				long double large_f = 0;
				if (f==0) {
				  if (DEBUG) cout << "f == 0 because " << "x = " << x << ", cs = " << cs << ", g = " << g << ", sn = " << sn  <<endl;
				  long double large_x =   x * tiny_factor;
				  long double large_g =   g * tiny_factor;
				  long double large_cs = cs * tiny_factor;
				  long double large_sn = sn * tiny_factor;
				  if (DEBUG) cout << large_x * large_cs <<endl;;
				  if (DEBUG) cout << large_g * large_sn <<endl;
				  large_f = large_x * large_cs + large_g * large_sn;
				  if (DEBUG) cout << "new f = " << large_f << endl;
				}

				g = g * cs - x * sn;
				h = y * sn; // h == 0 because y==0
				y = y * cs;

				for( j = 0; j < n; j++)
				{
					x = real( V(j,i-1) );
					w = real( V(j,i) );
					V(j,i-1) = complex_t( x * cs + w * sn, 0.0 );
					V(j,i)   = complex_t( w * cs - x * sn, 0.0 );
				}

				bool tiny_w = false;
//				if (DEBUG) cout.precision(32);
//				if (DEBUG) cout << " h = " << h << " f = " << f << " large_f = " << large_f << endl;
				if (abs(h)  < 1e-13 && abs(f) < 1e-13 && large_f != 0) {
				  tiny_w = true;
				}
				else {
				  w = sqrt( h * h + f * f );
				}
				w = sqrt( h * h + f * f );
				if (w == 0 && !tiny_w) {

				  if (DEBUG) {
				    cout << "ERROR 2: w is exactly 0: h = " << h << " , f = " << f << endl;
				    cout << " w = " << w << endl;
				  }
				  return FAILURE;
				}

				S[i-1] = w;
				if (tiny_w) {
				  if (DEBUG) cout << "tiny" <<endl;
				  cs = 1.0; // because h==0, so w = f
				  sn = 0;
				} else {
				  cs = f / w;
				  sn = h / w;
				}
//				cs = f / w;
//				sn = h / w;

				f = cs * g + sn * y;
				x = cs * y - sn * g;
				for( j = 0; j < n; j++)
				{
					y = real( U(j,i-1) );
					w = real( U(j,i) );
					U(j,i-1) = complex_t( y * cs + w * sn, 0.0 );
					U(j,i)   = complex_t( w * cs - y * sn, 0.0 );
				}
			}
			t[l] = 0.0;
			t[k] = f;
			S[k] = x;
		}


        if ( w < -1e-13 ) //
		{
			S[k] = - w;
			for( j = 0; j < n; j++){
				V(j,k) = - V(j,k);
			}
		}
	}

	if(DEBUG) cout << "Sorting the singular values" << endl;
//
//  Sort the singular values.
//
	for( k = 0; k < n; k++)
	{
		g = - 1.0;
		j = k;
        for( i = k; i < n; i++)
		{
			if ( g < S[i] )
			{
				g = S[i];
				j = i;
			}
        }

        if ( j != k )
		{
			S[j] = S[k];
			S[k] = g;

			for( i = 0; i < n; i++)
			{
				q      = V(i,j);
				V(i,j) = V(i,k);
				V(i,k) = q;
			}

			for( i = 0; i < n; i++)
			{
				q      = U(i,j);
				U(i,j) = U(i,k);
				U(i,k) = q;
			}
        }
    }

    for( k = n-1 ; k >= 0; k--)
	{
		if ( b[k] != 0.0 )
		{
			q = -A(k,k) / abs( A(k,k) );
			for( j = 0; j < m; j++){
				U(k,j) = q * U(k,j);
			}
			for( j = 0; j < m; j++)
			{
				q = complex_t( 0.0, 0.0 );
				for( i = k; i < m; i++){
					q = q + conj( A(i,k) ) * U(i,j);
				}
				q = q / ( abs( A(k,k) ) * b[k] );
				for( i = k; i < m; i++){
					U(i,j) = U(i,j) - q * A(i,k);
				}
			}
		}
	}

	for( k = n-1 -1; k >= 0; k--)
	{
		k1 = k + 1;
		if ( c[k1] != 0.0 )
		{
			q = -conj( A(k,k1) ) / abs( A(k,k1) );

			for( j = 0; j < n; j++){
				V(k1,j) = q * V(k1,j);
			}

			for( j = 0; j < n; j++)
			{
				q = complex_t( 0.0, 0.0 );
				for( i = k1 ; i < n; i++){
					q = q + A(k,i) * V(i,j);
				}
				q = q / ( abs( A(k,k1) ) * c[k1] );
				for( i = k1; i < n; i++){
					V(i,j) = V(i,j) - q * conj( A(k,i) );
				}
			}
		}
	}

	// Check if SVD out put is wrong
	cmatrix_t diag_S = diag(S,m,n);
	cmatrix_t temp = U*diag_S;
	temp = temp * AER::Utils::dagger(V);
	const auto nrows = temp_A.GetRows();
	const auto ncols = temp_A.GetColumns();
	bool equal = true;

	for (i=0; i < nrows; i++)
	    for (j=0; j < ncols; j++)
	      if (std::real(std::abs(temp_A(i, j) - temp(i, j))) > THRESHOLD)
	      {
	    	  equal = false;
	      	  cout << "diff = " << (std::real(std::abs(temp_A(i, j) - temp(i, j)))) << endl;
	      }
	if( ! equal )
	{
//		temp_A.SetOutputStyle(Matrix);
//		diag_S.SetOutputStyle(Matrix);
		cout << "error: wrong SVD calc: A != USV*" << endl;
//		cout << "A = " << endl;
//		cout << temp_A;
//		cout << "U = " << endl;
//		cout << U;
//		cout << "S = " << endl;
//		cout << diag_S;
//		cout << "V* = " << endl;
//		cout << AER::Utils::dagger(V);
//		cout << "USV* = " << endl;
//		cout << U*diag_S*(AER::Utils::dagger(V));
//		cout << temp;
//		assert(false);
	}

	// Cut-off small elements
//	double cut_off_threshold = 1e-15;
//	if(DEBUG) cout << "Cut-off small elements" << endl;
//	for (i=0; i < nrows; i++)
//		for (j=0; j < nrows; j++)
//		{
//			if(std::abs(U(i, j).real()) < cut_off_threshold)
//				U(i, j).real(0);
//			if(std::abs(U(i, j).imag()) < cut_off_threshold)
//				U(i, j).imag(0);
//		}
//	for (i=0; i < S.size(); i++)
//	{
//		if(S[i] < cut_off_threshold)
//			S[i] = 0;
//	}
//	for (i=0; i < ncols; i++)
//		for (j=0; j < ncols; j++)
//		{
//			if(std::abs(V(i, j).real()) < cut_off_threshold)
//				V(i, j).real(0);
//			if(std::abs(V(i, j).imag()) < cut_off_threshold)
//				V(i, j).imag(0);
//		}

	// Transpose again if m < n
	if(transposed)
		swap(U,V);

	return SUCCESS;
}


void csvd_wrapper (cmatrix_t &A, cmatrix_t &U,rvector_t &S,cmatrix_t &V)
{
	cmatrix_t coppied_A = A;
	int times = 0;
	if(DEBUG) cout << "1st try" << endl;
	status current_status = csvd(A, U, S, V);
	if (current_status == SUCCESS)
	  return;
	if(DEBUG) cout << "1st try success" << endl;

	while(times <= NUM_SVD_TRIES && current_status == FAILURE)
	  {
	    times++;
	    coppied_A = coppied_A*mul_factor;
	    A = coppied_A;
	    if(DEBUG) cout << "another try" << endl;
	    current_status = csvd(A, U, S, V);
	  }
	if(times == NUM_SVD_TRIES) {
	  cout << "SVD failed" <<endl;
	  assert(false);
	}

	//Divide by mul_factor every singular value after we multiplied matrix a
	for(int k = 0; k < S.size(); k++)
			S[k] /= pow(mul_factor, times);
}



