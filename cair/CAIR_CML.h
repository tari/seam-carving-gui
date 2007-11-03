#ifndef CAIR_CML_H
#define CAIR_CML_H

//CAIR Matrix Library
//Copyright (C) 2007 Joseph Auman (brain.recall@gmail.com)
//http://brain.recall.googlepages.com/cair

//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <http://www.gnu.org/licenses/>.

//This class will be used to handle and manage all matrix types used in CAIR.
//For grayscale images, use the type CML_gray.
//For standard color images, use the type CML_color.
//For energy, edges, and weights, use the type CML_int.
//This class is used to replace and consolidate the several types I had prevously maintained seperately.
//This will also give a slight performance boost, as EasyBMP wasn't really intended for such things...

//Note for developers: Unfortunetly, this class means that a seperate translation function will need
//	to be written to translate from whatever internal image object to the CML_Matrix. This will keep CAIR
//	more flexable, but adds another small step.

#include <vector>
#include <limits>

using namespace std;

typedef unsigned char CML_byte;

struct CML_RGBA
{
	CML_byte red;
	CML_byte green;
	CML_byte blue;
	CML_byte alpha;
};

//used for the Get() method.
//MAX will return the integer maximum value if out of bounds, used only for integer types.
//ZERO will return zero if out of bounds, also for integer types.
//For all other types DON'T use Get() with the CML_Bounds parameter. Either use Get(int x, int y)
//only if you are aware of its operation, or just use the [][] operators if you can't go out-of-bounds.
enum CML_bounds { MAX, ZERO };

template <typename T>
class CML_Matrix
{
public:
	//simple constructor
	CML_Matrix( int x, int y )
	{
		CML_Matrix::Resize( x, y );
	}

	//lets for direct access of the matrix
	vector<T>& operator[]( int x )
	{
		if( x < 0 ) //minor check for add/remove
		{
			x = 0;
		}
		return matrix[x];
	}

	int Width()
	{
		return (int)matrix.size();
	}

	int Height()
	{
		return (int)matrix[0].size();
	}

	//non-destructive resize of the matrix to the new paramaters
	void Resize( int x, int y )
	{
		matrix.resize( x );

		for( int i = 0; i < CML_Matrix::Width(); i++ )
		{
			matrix[i].resize( y );
		}
	}

	//does a flip/rotate on Source and stores it into ourselves
	void Transpose( CML_Matrix<T> * Source )
	{
		CML_Matrix::Resize( (*Source).Height(), (*Source).Width() );

		for( int x = 0; x < (*Source).Width(); x++ )
		{
			for( int y = 0; y < (*Source).Height(); y++ )
			{
				matrix[y][x] = (*Source)[x][y];
			}
		}
	}

	//Mimics the EasyBMP () operation, by constraining an out-of-bounds value back into the matrix.
	T Get( int x, int y )
	{
		if( x < 0 )
		{
			x = 0;
		}
		else if( x >= CML_Matrix::Width() )
		{
			x = CML_Matrix::Width() - 1;
		}
		if( y < 0 )
		{
			y = 0;
		}
		else if( y >= CML_Matrix::Height() )
		{
			y = CML_Matrix::Height() - 1;
		}
		return matrix[x][y];
	}

	//use only for integer types, otherwise this won't even compile
	//kinda hackish that I have to overload it to get it to work right... to fix someday
	//constrains in the X direction and replaces the old Get_Element() function
	T Get( int x, int y, CML_bounds clip_type )
	{
		switch( clip_type )
		{
		case MAX:
			if( ( x < 0 ) || ( x >= CML_Matrix::Width() ) )
			{
				return numeric_limits<int>::max();
			}
			else
			{
				return matrix[x][y];
			}
		case ZERO:
			if( ( x < 0 ) || ( x >= CML_Matrix::Width() ) )
			{
				return 0;
			}
			else
			{
				return matrix[x][y];
			}
		default:
			return 0;
		};
	}

	//memory reservation for the internal matrix
	void Reserve( int x, int y )
	{
		for( int i = 0; i < CML_Matrix::Width(); i++ )
		{
			matrix[i].reserve( y );
		}
		matrix.reserve( x );
	}

private:
	vector< vector<T> > matrix;
};

//typedef'ing to make it a little cleaner
typedef CML_Matrix<CML_RGBA> CML_color;
typedef CML_Matrix<CML_byte> CML_gray;
typedef CML_Matrix<int> CML_int;

#endif
