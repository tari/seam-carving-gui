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

//Note for developers: Unfortunetly, this class means that a seperate translation function will need
//	to be written to translate from whatever internal image object to the CML_Matrix. This will keep CAIR
//	more flexable, but adds another small step.

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
//only if you are aware of its operation, or just use the () operators if you can't go out-of-bounds.
enum CML_bounds { MAX, ZERO };

template <typename T>
class CML_Matrix
{
public:
	//Simple constructor.
	CML_Matrix( int x, int y )
	{
		Allocate_Matrix( x, y );
		current_x = x;
		current_y = y;
	}
	//Simple destructor.
	~CML_Matrix()
	{
		Deallocate_Matrix();
	}

	//Assignment operator; not really fast, but it works reasonably well.
	CML_Matrix& operator= ( const CML_Matrix& input )
	{
		if( this == &input ) //self-assignment check
		{
			return *this;
		}
		Deallocate_Matrix();
		Allocate_Matrix( input.current_x, input.current_y );
		current_x = input.current_x;
		current_y = input.current_y;

		for( int x = 0; x < current_x; x++ )
		{
			for( int y = 0; y < current_y; y++ )
			{
				matrix[x][y] = input(x,y);
			}
		}
		return *this;
	}

	//Set all values.
	//This is important to do since the memory is not set a value after it is allocated.
	//Make sure to do this for the weights.
	void Fill( T value )
	{
		for( int x = 0; x < CML_Matrix::Width(); x++ )
		{
			for( int y = 0; y < CML_Matrix::Height(); y++ )
			{
				matrix[x][y] = value;
			}
		}
	}

	//Access the matrix members.
	//NOTE: There are no bounds check with this operation.
	//		If bounds check is desired, use the Get() method.
	inline const T& operator()( int x, int y ) const
	{
		return matrix[x][y];
	}

	//For the assignment () calls.
	//Only peforms a minor x direction check.
	inline T& operator()( int x, int y )
	{
		if( x < 0 ) //minor check for add/remove
		{
			x = 0;
		}
		return matrix[x][y];
	}

	//Returns the current image Width.
	inline int Width()
	{
		return current_x;
	}

	//Returns the current image Height.
	inline int Height()
	{
		return current_y;
	}

	//Does a flip/rotate on Source and stores it into ourself.
	void Transpose( CML_Matrix<T> * Source )
	{
		CML_Matrix::D_Resize( (*Source).Height(), (*Source).Width() );

		for( int x = 0; x < (*Source).Width(); x++ )
		{
			for( int y = 0; y < (*Source).Height(); y++ )
			{
				matrix[y][x] = (*Source)(x,y);
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

	//Return a very large value or zero if out-of-bounds in the x direction.
	//Only intended for integer types!!
	//Use for now, but later optimize so this isn't required.
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

	//Destructive resize of the matrix.
	void D_Resize( int x, int y )
	{
		Deallocate_Matrix();
		Allocate_Matrix( x, y );
		current_x = x;
		current_y = y;
	}

	//Non-destructive resize, but only in the x direction.
	void Resize_Width( int x )
	{
		if( x > current_x )
		{
			//create new "master" row
			T ** temp = new T*[x];

			//copy over the old pointer values
			for( int i = 0; i < current_x; i++ )
			{
				temp[i] = matrix[i];
			}

			//create new columns as needed
			while( x > current_x )
			{
				temp[current_x] = new T[current_y];
				current_x++;
			}

			//delete the old "master" row
			delete[] matrix;

			//assign the new row;
			matrix = temp;
		}
		else if( x < current_x )
		{
			//create new master row
			T ** temp = new T*[x];

			//copy over the old values
			for( int i = 0; i < x; i++ )
			{
				temp[i] = matrix[i];
			}

			//delete the old columns
			while( x < current_x )
			{
				current_x--;
				delete[] matrix[current_x];
			}

			//delete the old master row
			delete[] matrix;

			//reassign it
			matrix = temp;
		}
	}

private:
	//Simple 2D allocation algorithm.
	//The size variables must be assigned seperately.
	void Allocate_Matrix( int x, int y )
	{
		matrix = new T*[x];

		for( int i = 0; i < x; i++ )
		{
			matrix[i] = new T[y];
		}
	}
	//Simple 2D deallocation algorithm.
	//Doest not maintain size variables.
	void Deallocate_Matrix()
	{
		for( int i = 0; i < current_x; i++ )
		{
			delete[] matrix[i];
		}

		delete[] matrix;
	}

	T ** matrix;
	int current_x;
	int current_y;
};

//typedef'ing to make it a little cleaner
typedef CML_Matrix<CML_RGBA> CML_color;
typedef CML_Matrix<CML_byte> CML_gray;
typedef CML_Matrix<int> CML_int;

#endif
