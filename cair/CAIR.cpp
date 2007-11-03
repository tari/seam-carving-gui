
//CAIR - Content Aware Image Resizer
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

//This thing should hopefuly perform the image resize method developed by Shai Avidan and Ariel Shamir.
//This is to be released under the GPLv3 with no implied warrenty of any kind.

//TODO (maybe):
//		- Try insertion and deletion in Add_Path() and Remove_Path(), but will require CML to go row-major, which it is not...
//		- Abstract out pthreads into macros allowing for mutliple thread types to be used (ugh, not for a while at least)
//		- Maybe someday push CAIR into OO land and create a class out of it.

//CAIR v2.0 Changelog:
//	- Now 50% faster across the board.
//  - EasyBMP has been replaced with CML, the CAIR Matrix Library. This gave speed improvements and code standardization.
//		This is such a large change it has affected all functions in CAIR, all for the better. Refrence objects have been
//		replaced with standard pointers.
//	- Added three new functions: CAIR_Grayscale(), CAIR_Edge(), and CAIR_Energy(), which give the grayscale, edge detection,
//		and energy maps of a source image.
//	- Add_Path() and Remove_Path() now maintain Grayscale during resizing. This gave a performance boost with no real 
//		quality reduction; special thanks to Brett Taylor.
//	- Edge_Detect() now handles the boundries seperately for a performance boost.
//  - Add_Path() and Remove_Path() no longer refill unchanged portions of an image since CML Resize's are no longer destructive.
//  - CAIR_Add() now Reserve's memory for the vectors in CML to prevent memory thrashing as they are enlarged.
//	- Fixed another adding bug; new paths have thier own artifical weight
//CAIR v1.2 Changelog:
//  - Fixed ANOTHER adding bug; now works much better with < 1 quality
//  - a few more comment updates
//CAIR v1.1 Changelog:
//  - Fixed a bad bug in adding; averaging the wrong pixels
//  - Fixed a few incorrect/outdated comments
//CAIR v1.0 Changelog:
//  - Path adding now working with reasonable results; special thanks to Ramin Sabet
//  - Add_Path() has been multi-threaded
//CAIR v0.5 Changelog:
//	- Multi-threaded Energy_Map() and Remove_Path(), gave another 30% speed boost with quality = 0
//	- Fixed a few compiler warnings when at level 4 (just stuff in the CAIR_DEBUG code)

#include "CAIR.h"
#include "CAIR_CML.h"
#include <iostream>
#include <vector>
#include <pthread.h>
#include <cmath>

using namespace std;

struct pixel
{
	int x;
	int y;
};

/*****************************************************************************************
**                                     G R A Y S C A L E                                **
*****************************************************************************************/
//for the grayscale threads, so they know what to work on and how much of the image to do
struct Gray_Params
{
	CML_color * Source;
	CML_gray * Dest;
	int top_x;
	int top_y;
	int bot_x;
	int bot_y;
};

CML_byte Grayscale_Pixel( CML_RGBA * pixel )
{
	return (CML_byte)floor( 0.299 * pixel->red +
							0.587 * pixel->green +
							0.114 * pixel->blue );
}

//Our thread function for the Grayscale
void * Gray_Quadrant( void * area )
{
	Gray_Params * gray_area;
	gray_area = (Gray_Params *)area;

	CML_byte gray = 0;

	for( int x = gray_area->top_x; x <= gray_area->bot_x; x++ )
	{
		for( int y = gray_area->top_y; y <= gray_area->bot_y; y++ )
		{
			gray = Grayscale_Pixel( &(*(gray_area->Source))[x][y] );

			(*(gray_area->Dest))[x][y] = gray;
		}
	}

	return NULL;
}


//Sort-of does a RGB->YUV conversion
//This is multi-threaded to 4 threads, spliting the image into 4 quadrants
void Grayscale_Image( CML_color * Source, CML_gray * Dest )
{
	pthread_t gray_threads[4];
	Gray_Params thread_params[4];

	(*Dest).Resize( (*Source).Width(), (*Source).Height() );

	//top left quadrant
	thread_params[0].Source = Source;
	thread_params[0].Dest = Dest;
	thread_params[0].top_x = 0;
	thread_params[0].top_y = 0;
	thread_params[0].bot_x = (*Source).Width() / 2;
	thread_params[0].bot_y = (*Source).Height() / 2;

	//top right quadrant
	thread_params[1].Source = Source;
	thread_params[1].Dest = Dest;
	thread_params[1].top_x = thread_params[0].bot_x + 1;
	thread_params[1].top_y = 0;
	thread_params[1].bot_x = (*Source).Width() - 1;
	thread_params[1].bot_y = thread_params[0].bot_y;

	//bottom left quadrant
	thread_params[2].Source = Source;
	thread_params[2].Dest = Dest;
	thread_params[2].top_x = 0;
	thread_params[2].top_y = thread_params[0].bot_y + 1;
	thread_params[2].bot_x = thread_params[0].bot_x;
	thread_params[2].bot_y = (*Source).Height() - 1;

	//bottom right quadrant
	thread_params[3].Source = Source;
	thread_params[3].Dest = Dest;
	thread_params[3].top_x = thread_params[1].top_x;
	thread_params[3].top_y = thread_params[2].top_y;
	thread_params[3].bot_x = thread_params[1].bot_x;
	thread_params[3].bot_y = thread_params[2].bot_y;

	//create the four threads
	for( int i = 0; i < 4; i++ )
	{
		int rc = pthread_create( &gray_threads[i], NULL, Gray_Quadrant, (void *)&thread_params[i] );
		if( rc != 0 )
		{
			cout << endl << "CAIR: Grayscale_Image(): FATAL ERROR! Unable to spawn thread!" << endl;
			exit(1);
		}
	}

	//now wait for them to come back to us
	pthread_join( gray_threads[0], NULL );
	pthread_join( gray_threads[1], NULL );
	pthread_join( gray_threads[2], NULL );
	pthread_join( gray_threads[3], NULL );
}

/*****************************************************************************************
**                                   E D G E                                            **
*****************************************************************************************/
enum edge_safe { SAFE, UNSAFE };

//for the edge detection threads, works in a similar way as the grayscale params
struct Edge_Params
{
	CML_gray * Source;
	CML_int * Dest;
	int top_x;
	int top_y;
	int bot_x;
	int bot_y;
	edge_safe safety;
};

//returns the convolution value of the pixel Source[x][y] with the given kernel
//Safe version for the perimiters
int Convolve_Pixel( CML_gray * Source, int x, int y, int kernel[][3], edge_safe safety )
{
	int conv = 0;

	//these loops are a bit complicted since I need to access pixels all around the called pixel
	for( int xx = 0; xx < 3; xx++ )
	{
		for( int yy = 0; yy < 3; yy++ )
		{
			if( safety == SAFE )
			{
				conv += (*Source).Get( x + xx - 1, y + yy - 1 ) * kernel[xx][yy];
			}
			else
			{
				conv += (*Source)[x + xx - 1][y + yy - 1] * kernel[xx][yy];
			}
		}
	}

	return conv;
}

//This is multi-threaded to 4 threads, spliting the image into 4 quadrants
void * Edge_Quadrant( void * area )
{
	Edge_Params * edge_area;
	edge_area = (Edge_Params *)area;

	//Prewitt's gradient edge detector kernels, first the x direction, then the y
	int Prewitt_Kernel_X[3][3] = 
	{
		{ 1, 0, -1 },
		{ 1, 0, -1 },
		{ 1, 0, -1 }
	};
	int Prewitt_Kernel_Y[3][3] = 
	{
		{ 1, 1, 1 },
		{ 0, 0, 0 },
		{ -1, -1, -1 }
	};
	int edge_x = 0;
	int edge_y = 0;
	int edge_value = 0;

	for( int i = edge_area->top_x; i <= edge_area->bot_x; i++ )
	{
		for( int j = edge_area->top_y; j <= edge_area->bot_y; j++ )
		{
			//get the edges for the x and y directions
			edge_x = Convolve_Pixel( edge_area->Source, i, j, Prewitt_Kernel_X, edge_area->safety );
			edge_y = Convolve_Pixel( edge_area->Source, i, j, Prewitt_Kernel_Y, edge_area->safety );

			//add their weights up
			edge_value = abs(edge_x) + abs(edge_y);

			(*(edge_area->Dest))[i][j] = edge_value;
		}
	}

	return NULL;
}

//performs the Prewitt edge detection on the Source image, places it into Dest
//Obvious choices for edge detection include Laplacian, Sobel, and Canny.
//Laplacian doesn't like noise all that much. Sobel can produce great results for rather simple coding.
//Canny produces perhaps the best edge detection, however, it is very sensitive and requires tweaking to get it better than the rest.
//I decided to go with a modified Sobel, called Prewitt. It has the ease of coding, but doesn't suffer the isotropic nature of sobel.
//(ie. it is less particualr to a set of directions around the pixel. Sobel likes the cardinal directions somewhat. They're both decent.)
//Note: Dest better be of proper size.
void Edge_Detect( CML_gray * Source, CML_int * Dest )
{
	//There is no easy solution to the boundries. Calling the same boundry pixel to convolve itself against seems actually better
	//than padding the image with zeros or 255's.
	//Calling itself induces a "ringing" into the near edge of the image. Padding can lead to a darker or lighter edge.
	//The only "good" solution is to have the entire one-pixel wide edge not included in the edge detected image.
	//This would reduce the size of the image by 2 pixels in both directions, something that is unacceptable here.

	pthread_t edge_threads[4];
	Edge_Params thread_params[5];

	//top left quadrant
	thread_params[0].Source = Source;
	thread_params[0].Dest = Dest;
	thread_params[0].safety = UNSAFE;
	thread_params[0].top_x = 1;
	thread_params[0].top_y = 1;
	thread_params[0].bot_x = (*Source).Width() / 2;
	thread_params[0].bot_y = (*Source).Height() / 2;

	//top right quadrant
	thread_params[1].Source = Source;
	thread_params[1].Dest = Dest;
	thread_params[1].safety = UNSAFE;
	thread_params[1].top_x = thread_params[0].bot_x + 1;
	thread_params[1].top_y = 1;
	thread_params[1].bot_x = (*Source).Width() - 2;
	thread_params[1].bot_y = thread_params[0].bot_y;

	//bottom left quadrant
	thread_params[2].Source = Source;
	thread_params[2].Dest = Dest;
	thread_params[2].safety = UNSAFE;
	thread_params[2].top_x = 1;
	thread_params[2].top_y = thread_params[0].bot_y + 1;
	thread_params[2].bot_x = thread_params[0].bot_x;
	thread_params[2].bot_y = (*Source).Height() - 2;

	//bottom right quadrant
	thread_params[3].Source = Source;
	thread_params[3].Dest = Dest;
	thread_params[3].safety = UNSAFE;
	thread_params[3].top_x = thread_params[1].top_x;
	thread_params[3].top_y = thread_params[2].top_y;
	thread_params[3].bot_x = thread_params[1].bot_x;
	thread_params[3].bot_y = thread_params[2].bot_y;

	//create the four threads
	for( int i = 0; i < 4; i++ )
	{
		int rc = pthread_create( &edge_threads[i], NULL, Edge_Quadrant, (void *)&thread_params[i] );
		if( rc != 0 )
		{
			cout << endl << "CAIR: Edge_Detect(): FATAL ERROR! Unable to spawn thread!" << endl;
			exit(1);
		}
	}

	//while those are running we can go back and do the boundry pixels with the extra safety checks
	thread_params[4] = thread_params[0];
	thread_params[4].top_x = 0;
	thread_params[4].top_y = 0;
	thread_params[4].bot_x = (*Source).Width() - 1;
	thread_params[4].bot_y = 0;
	thread_params[4].safety = SAFE;
	Edge_Quadrant( &thread_params[4] );

	thread_params[4].top_y = (*Source).Height() - 1;
	thread_params[4].bot_y = (*Source).Height() - 1;
	Edge_Quadrant( &thread_params[4] );

	thread_params[4].bot_x = 0;
	thread_params[4].top_y = 0;
	Edge_Quadrant( &thread_params[4] );

	thread_params[4].top_x = (*Source).Width() - 1;
	thread_params[4].top_y = 0;
	thread_params[4].bot_x = (*Source).Width() - 1;
	Edge_Quadrant( &thread_params[4] );

	//now wait on them
	pthread_join( edge_threads[0], NULL );
	pthread_join( edge_threads[1], NULL );
	pthread_join( edge_threads[2], NULL );
	pthread_join( edge_threads[3], NULL );
}

/*****************************************************************************************
**                                     E N E R G Y                                      **
*****************************************************************************************/

struct Energy_Params
{
	vector<pthread_mutex_t> * Mine;
	vector<pthread_mutex_t> * Not_Mine;
	pthread_mutex_t * Lock_Done_mutex;
	pthread_cond_t * Lock_Done;
	pthread_mutex_t * Good_to_Go;
	CML_int * Edge;
	CML_int * Weights;
	CML_int * Energy_Map;
	int top_x;
	int bot_x;
	bool is_left;
};

//Simple fuction returning the minimum of three values.
int min_of_three( int x, int y, int z )
{
	int min = y;

	if( x < min )
	{
		min = x;
	}
	if( z < min )
	{
		min = z;
	}

	return min;
}

//This calculates a minimum energy path from the given start point (min_x) and the energy map.
//Note: Path better be of proper size.
void Generate_Path( CML_int * Energy, int min_x, vector<pixel> * Path )
{
	pixel min;
	int x = min_x;
	for( int y = (*Energy).Height() - 1; y >= 0; y-- ) //builds from bottom up
	{
		min.x = x; //assume the minimum is straight up
		min.y = y;

		if( (*Energy).Get( x-1, y, MAX ) < (*Energy).Get( min.x, min.y, MAX ) ) //check to see if min is up-left
		{
			min.x = x - 1;
		}
		if( (*Energy).Get( x+1, y, MAX ) < (*Energy).Get( min.x, min.y, MAX ) ) //up-right
		{
			min.x = x + 1;
		}
		
		(*Path)[y] = min;
		x = min.x;
	}
}

//threading procedure for Energy Map
////main_thread locks mutex1
////main_thread locks mutex2
////main_thread creates energy_thread1
////main_thread waits for signal
////energy_thread1 locks its mutexes
////energy_thread1 signals its done
////energy_thread1 waits to lock mutex1
////
////main_thread wakes up from signal
////main_thread creates energy_thread2
////main_thread waits for signal
////energy_thread2 locks its mutexes
////energy_thread2 signals its done
////energy_thread2 waits to lock mutex2
////
////
////main_thread wakes up from signal
////main_thread unlocks mutex1
////main_thread unlocks mutex2
////
////main_thread waits for energy_thread1 to join
////main_thread waits for energy_thread2 to join
void * Energy_Half( void * area )
{
	Energy_Params * energy_area;
	energy_area = (Energy_Params *)area;
	int min = 0;

	//lock our mutexes
	for( int i = 0; i < (int)(*(energy_area->Mine)).size(); i++ )
	{
		pthread_mutex_lock( &(*(energy_area->Mine))[i] );
	}

	//signal we are done
	pthread_mutex_lock( energy_area->Lock_Done_mutex );
	pthread_cond_signal( energy_area->Lock_Done );
	pthread_mutex_unlock( energy_area->Lock_Done_mutex );

	//wait until we are good to go
	pthread_mutex_lock( energy_area->Good_to_Go );

	//set the first row with the correct energy
	for( int x = energy_area->top_x; x <= energy_area->bot_x; x++ )
	{
		(*(energy_area->Energy_Map))[x][0] = (*(energy_area->Edge))[x][0] + (*(energy_area->Weights))[x][0];
	}
	//now signal that one is done
	pthread_mutex_unlock( &(*(energy_area->Mine))[0] );

	for( int y = 1; y < (*(energy_area->Edge)).Height(); y++ )
	{
		for( int x = energy_area->top_x; x <= energy_area->bot_x; x++ )
		{
			if( energy_area->is_left == true )
			{
				if( x == energy_area->bot_x ) //in the trouble area
				{
					//wait to make sure we have a good value in that trouble area (for left, x+1)
					pthread_mutex_lock( &(*(energy_area->Not_Mine))[y-1] );
				}

				//grab the minimum of straight up, up left, or up right
				min = min_of_three( (*(energy_area->Energy_Map)).Get( x-1, y-1, MAX ),
									(*(energy_area->Energy_Map)).Get(   x, y-1, MAX ),
									(*(energy_area->Energy_Map)).Get( x+1, y-1, MAX ) );
				//set the energy of the pixel
				(*(energy_area->Energy_Map))[x][y] = min + (*(energy_area->Edge))[x][y] + (*(energy_area->Weights))[x][y];

				//now signal if this is the trouble area
				if( x == energy_area->bot_x )
				{
					pthread_mutex_unlock( &(*(energy_area->Mine))[y] );
				}
			}
			else //the right thread
			{
				if( x == energy_area->top_x )
				{
					//wait to get the value, for right the trouble value is x-1
					pthread_mutex_lock( &(*(energy_area->Not_Mine))[y-1] );
				}

				//grab the minimum of straight up, up left, or up right
				min = min_of_three( (*(energy_area->Energy_Map)).Get( x-1, y-1, MAX ),
									(*(energy_area->Energy_Map)).Get(   x, y-1, MAX ),
									(*(energy_area->Energy_Map)).Get( x+1, y-1, MAX ) );
				//set the energy of the pixel
				(*(energy_area->Energy_Map))[x][y] = min + (*(energy_area->Edge))[x][y] + (*(energy_area->Weights))[x][y];

				//now signal if this is the trouble area
				if( x == energy_area->top_x )
				{
					pthread_mutex_unlock( &(*(energy_area->Mine))[y] );
				}
			}

		}
	}

	return NULL;
}

//Calculates the energy map
//This uses a dual-threaded approach. More than two threads becomes difficult, since one thread needs values from
//the area another thread is responsible for. There would be too much blocking to make it effective.
void Energy_Map( CML_int * Edge, CML_int * Weights, CML_int * Map )
{
	pthread_t energy_threads[2];
	Energy_Params thread_params[2];

	//signals need a mutex to prevent lost signals
	pthread_mutex_t Lock_Done_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t Lock_Done = PTHREAD_COND_INITIALIZER;

	//good to go mutuexes
	pthread_mutex_t Good1 = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t Good2 = PTHREAD_MUTEX_INITIALIZER;

	//two arrays of mutexes, keep the trouble areas safe
	vector<pthread_mutex_t> Left_mutexes ( (*Edge).Height() );
	vector<pthread_mutex_t> Right_mutexes ( (*Edge).Height() );
	for(int k = 0; k < (*Edge).Height(); k++)
	{
		pthread_mutex_init(&Left_mutexes[k], 0);
		pthread_mutex_init(&Right_mutexes[k], 0);
	}

	//set the paramaters
	//left side
	thread_params[0].Edge = Edge;
	thread_params[0].Weights = Weights;
	thread_params[0].Energy_Map = Map;
	thread_params[0].Lock_Done_mutex = &Lock_Done_mutex;
	thread_params[0].Lock_Done = &Lock_Done;
	thread_params[0].Good_to_Go = &Good1;
	thread_params[0].Mine = &Left_mutexes;
	thread_params[0].Not_Mine = &Right_mutexes;
	thread_params[0].top_x = 0;
	thread_params[0].bot_x = (*Edge).Width() / 2;
	thread_params[0].is_left = true;

	//the right side
	thread_params[1] = thread_params[0];
	thread_params[1].Mine = &Right_mutexes;
	thread_params[1].Not_Mine = &Left_mutexes;
	thread_params[1].Good_to_Go = &Good2;
	thread_params[1].top_x = thread_params[0].bot_x + 1;
	thread_params[1].bot_x = (*Edge).Width() - 1;
	thread_params[1].is_left = false;

	//lock what we need
	pthread_mutex_lock( &Good1 );
	pthread_mutex_lock( &Good2 );
	pthread_mutex_lock( &Lock_Done_mutex ); //make sure to lock it so other thread doesn't signal when we aren't waiting!

	//create the threads
	for( int i = 0; i < 2; i++ )
	{
		int rc = pthread_create( &energy_threads[i], NULL, Energy_Half, (void *)&thread_params[i] );
		if( rc != 0 )
		{
			cout << endl << "CAIR: Energy_Path(): FATAL ERROR! Unable to spawn thread!" << endl;
			exit(1);
		}
		//wait until the thread initlizes
		pthread_cond_wait( &Lock_Done, &Lock_Done_mutex );
	}

	//now both threads have thier mutexes locked, ready for them to start
	pthread_mutex_unlock( &Good1 );
	pthread_mutex_unlock( &Good2 );


	//now wait on them
	pthread_join( energy_threads[0], NULL );
	pthread_join( energy_threads[1], NULL );
}


//Energy_Path() generates the least energy Path of the Edge and Weights.
//This uses a dynamic programming method to easily calculate the path and energy map (see wikipedia for a good example).
//Weights should be of the same size as Edge, Path should be of proper length (the height of Edge).
void Energy_Path( CML_int * Edge, CML_int * Weights, vector<pixel> * Path )
{

	CML_int Energy( (*Edge).Width(), (*Edge).Height() );

	//calculate the energy map
	Energy_Map( Edge, Weights, &Energy );

	//find minimum path start
	int min_x = 0;
	for( int x = 0; x < Energy.Width(); x++ )
	{
		if( Energy[x][ Energy.Height() - 1 ] < Energy[min_x][ Energy.Height() - 1 ] )
		{
			min_x = x;
		}
	}

	//generate the path back from the energy map
	Generate_Path( &Energy, min_x, Path );
}

//averages two pixels and returns the values
CML_RGBA Average_Pixels( CML_RGBA Pixel1, CML_RGBA Pixel2 )
{
	CML_RGBA average;

	average.alpha = ( Pixel1.alpha + Pixel2.alpha ) / 2;
	average.blue = ( Pixel1.blue + Pixel2.blue ) / 2;
	average.green = ( Pixel1.green + Pixel2.green ) / 2;
	average.red = ( Pixel1.red + Pixel2.red ) / 2;

	return average;
}



/*****************************************************************************************
**                                        A D D                                         **
*****************************************************************************************/

void Add_Weights( CML_int * Add1, CML_int * Add2, CML_int * Sum )
{
	for( int x = 0; x < (*Add1).Width(); x++ )
	{
		for( int y = 0; y < (*Add1).Height(); y++ )
		{
			(*Sum)[x][y] = (*Add1)[x][y] + (*Add2)[x][y];
		}
	}
}

struct Add_Params
{
	CML_color * Source;
	CML_color * Dest;
	vector<pixel> * Path;
	CML_int * Weights;
	CML_int * AWeights;
	CML_int * Edge;
	CML_gray * Grayscale;
	int add_weight;
	int top_x;
	int top_y;
	int bot_x;
	int bot_y;
};

//This works like Remove_Quadrant, stripes across the image.
void * Add_Quadrant( void * area )
{
	Add_Params * add_area;
	add_area = (Add_Params *)area;

	for( int y = add_area->top_y; y <= add_area->bot_y; y++ )
	{
		pixel add = (*(add_area->Path))[y];
		for( int x = add_area->bot_x; x > add.x; x-- )
		{
			//fill in from right to left. This is to avoid copying over data we need
			(*(add_area->Dest))[x][y] = (*(add_area->Source)).Get( x-1, y);
			(*(add_area->AWeights))[x][y] = (*(add_area->AWeights)).Get( x-1, y, ZERO );
			(*(add_area->Weights))[x][y] = (*(add_area->Weights)).Get( x-1, y, ZERO );
			(*(add_area->Edge))[x][y] = (*(add_area->Edge)).Get( x-1, y, ZERO );
			(*(add_area->Grayscale))[x][y] = (*(add_area->Grayscale)).Get( x-1, y );

		}
		//go back and set the added pixel
		(*(add_area->Dest))[add.x][add.y] = Average_Pixels( (*(add_area->Source))[add.x][add.y], (*(add_area->Source)).Get(add.x-1, add.y));
		(*(add_area->Weights))[add.x][add.y] = ( (*(add_area->Weights))[add.x][add.y] + (*(add_area->Weights)).Get(add.x-1, add.y) ) / 2;
		(*(add_area->Edge))[add.x][add.y] = ( (*(add_area->Edge))[add.x][add.y] + (*(add_area->Edge)).Get(add.x-1, add.y) ) / 2;
		(*(add_area->Grayscale))[add.x][add.y] = ( (*(add_area->Grayscale))[add.x][add.y] + (*(add_area->Grayscale)).Get(add.x-1, add.y) ) / 2;

		(*(add_area->AWeights))[add.x][add.y] = add_area->add_weight; //the new path
		(*(add_area->AWeights))[add.x+1][add.y] += add_area->add_weight; //the previous least-energy path
	}
	return NULL;
}

//Adds Path into Source, storing the result in Dest.
//AWeights is used to store the enlarging artifical weights.
void Add_Path( CML_color * Source, vector<pixel> * Path, CML_int * Weights, CML_int * Edge, CML_gray * Grayscale, CML_int * AWeights, int add_weight, CML_color * Dest )
{
	(*Dest).Resize( (*Source).Width() + 1, (*Source).Height() );
	(*AWeights).Resize( (*Source).Width() + 1, (*Source).Height() );
	(*Weights).Resize( (*Source).Width() + 1, (*Source).Height() );
	(*Edge).Resize( (*Source).Width() + 1, (*Source).Height() );
	(*Grayscale).Resize( (*Source).Width() + 1, (*Source).Height() );

	pthread_t add_threads[4];
	Add_Params thread_params[4];

	//top strip, goes all the way across the image, but only 1/4 of its height
	thread_params[0].Source = Source;
	thread_params[0].Dest = Dest;
	thread_params[0].Path = Path;
	thread_params[0].Weights = Weights;
	thread_params[0].AWeights = AWeights;
	thread_params[0].Edge = Edge;
	thread_params[0].Grayscale = Grayscale;
	thread_params[0].add_weight = add_weight;
	thread_params[0].top_x = 0;
	thread_params[0].top_y = 0;
	thread_params[0].bot_x = (*Dest).Width() - 1;
	thread_params[0].bot_y = (*Source).Height() / 4;

	//top middle strip, going to reuse the first one since its mostly the same
	thread_params[1] = thread_params[0];
	thread_params[1].top_y = thread_params[0].bot_y + 1;
	thread_params[1].bot_y = thread_params[0].bot_y * 2;

	//bottom middle strip
	thread_params[2] = thread_params[0];
	thread_params[2].top_y = thread_params[1].bot_y + 1;
	thread_params[2].bot_y = thread_params[0].bot_y * 3;

	//bottom strip
	thread_params[3] = thread_params[0];
	thread_params[3].top_y = thread_params[2].bot_y + 1;
	thread_params[3].bot_y = (*Source).Height() - 1;

	//create the four threads
	for( int i = 0; i < 4; i++ )
	{
		int rc = pthread_create( &add_threads[i], NULL, Add_Quadrant, (void *)&thread_params[i] );
		if( rc != 0 )
		{
			cout << endl << "CAIR: Add_Path(): FATAL ERROR! Unable to spawn thread!" << endl;
			exit(1);
		}
	}

	//now wait on them
	pthread_join( add_threads[0], NULL );
	pthread_join( add_threads[1], NULL );
	pthread_join( add_threads[2], NULL );
	pthread_join( add_threads[3], NULL );
}

//Adds some paths to Source and outputs into Dest.
//This doesn't work anything like the paper describes, mostly because they probably forgot to mention a key point:
//Added paths need a high weight so they are not chosen again, or so thier neighbors are less likely to be chosen.
//For each new path, the energy of the image and its least-energy path is calculated. 
//To prevent the same path from being chosen and to prevent merging paths from being chosen, 
//additonal weight is placed to the old least-energy path and the new inserted path. Having 
//a very large add_weight will cause the algorithm to work more like a linear algorithm, evenly distributing new paths.
//Having a very small weight will cause stretching. I provide this as a paramater mainly because I don't know if someone
//will see a need for it, so I might of well leave it in.
bool CAIR_Add( CML_color * Source, CML_int * Weights, int goal_x, double quality, int add_weight, CML_color * Dest, ProgressPtr p )
{
	CML_gray Grayscale( (*Source).Width(), (*Source).Height() );
	CML_color Temp( (*Source).Width(), (*Source).Height() );

	int adds = goal_x - (*Source).Width();
	CML_int art_weight( (*Source).Width(), (*Source).Height() ); //artifical path weight
	CML_int sum_weight( (*Source).Width(), (*Source).Height() ); //the sum of Weights and the artifical weight
	CML_int Edge( (*Source).Width(), (*Source).Height() );
	vector<pixel> Min_Path ( (*Source).Height() );

	//have to do this first to get it started
	Temp = (*Source);
	(*Dest) = (*Source);
	Grayscale_Image( Source, &Grayscale );
	Edge_Detect( &Grayscale, &Edge );

	//prevent memory thrashing as vector<> is enlarged
	(*Dest).Reserve( goal_x, (*Source).Height() );
	Temp.Reserve( goal_x, (*Source).Height() );
	Grayscale.Reserve( goal_x, (*Source).Height() );
	Edge.Reserve( goal_x, (*Source).Height() );
	(*Weights).Reserve( goal_x, (*Source).Height() );
	art_weight.Reserve( goal_x, (*Source).Height() );
	sum_weight.Reserve( goal_x, (*Source).Height() );

	for( int i = 0; i < adds; i++ )
	{
		if(p && !p(i)) return false;
		Add_Weights( Weights, &art_weight, &sum_weight );
		Energy_Path( &Edge, &sum_weight, &Min_Path );
		Add_Path( &Temp, &Min_Path, Weights, &Edge, &Grayscale, &art_weight, add_weight, Dest );
		sum_weight.Resize( (*Dest).Width(), (*Dest).Height() );
		Temp = (*Dest);

		//quality calculation
		if( ( ( i % (int)( 1 / quality ) ) == 0 ) && ( quality != 0 ) )
		{
			Edge_Detect( &Grayscale, &Edge );
		}
	}
	return true;
}

/*****************************************************************************************
**                                      R E M O V E                                     **
*****************************************************************************************/


struct Remove_Params
{
	CML_color * Source;
	CML_color * Dest;
	vector<pixel> * Path;
	CML_int * Weights;
	CML_int * Edge;
	CML_gray * Grayscale;
	int top_x;
	int top_y;
	int bot_x;
	int bot_y;
};

//more multi-threaded goodness
//the areas are not quadrants, rather, more like strips, but I keep the name convention
void * Remove_Quadrant( void * area )
{
	Remove_Params * remove_area;
	remove_area = (Remove_Params *)area;

	for( int y = remove_area->top_y; y <= remove_area->bot_y; y++ )
	{
		//reduce each row by one, the removed pixel
		pixel remove = (*(remove_area->Path))[y];
		for( int x = remove.x; x <= remove_area->bot_x; x++ )
		{
			if( x > remove.x )
			{
				//shift over the pixels
				(*(remove_area->Dest))[x][y] = (*(remove_area->Source)).Get( x+1, y );
				(*(remove_area->Grayscale))[x][y] = (*(remove_area->Grayscale)).Get( x+1, y );
				(*(remove_area->Weights))[x][y] = (*(remove_area->Weights)).Get( x+1, y, ZERO );
				(*(remove_area->Edge))[x][y] = (*(remove_area->Edge)).Get( x+1, y, ZERO );
			}
			else
			{
				//blend the removed energy back in
				if( x != 0 )
				{
					(*(remove_area->Edge))[x-1][y] = ( (*(remove_area->Edge))[x][y] + (*(remove_area->Edge))[x-1][y] ) / 2; //no need to do a Get_Element here
				}
				(*(remove_area->Edge))[x][y] = ( (*(remove_area->Edge))[x][y] + (*(remove_area->Edge)).Get( x+1, y ) ) / 2;

				//blend the pixel
				if( (*(remove_area->Weights))[x][y] < 0 ) //area marked for removal, don't blend
				{
					(*(remove_area->Dest))[x][y] = (*(remove_area->Source)).Get( x+1, y );
				}
				else
				{
					//average removed pixel back in
					if( x != 0 )
					{
						(*(remove_area->Dest))[x-1][y] = Average_Pixels( (*(remove_area->Source))[x][y], (*(remove_area->Source)).Get(x-1,y) );
					}
					(*(remove_area->Dest))[x][y] = Average_Pixels( (*(remove_area->Source))[x][y], (*(remove_area->Source)).Get(x+1,y) );
				}

				//don't forget about the weights!
				(*(remove_area->Weights))[x][y] = (*(remove_area->Weights)).Get( x+1, y, ZERO );

				//adjusting grayscale
				if( x != 0 )
				{
					(*(remove_area->Grayscale))[x-1][y] = ( (*(remove_area->Grayscale))[x][y] + (*(remove_area->Grayscale))[x-1][y] ) / 2; //no need to do a Get_Element here
				}
				(*(remove_area->Grayscale))[x][y] = ( (*(remove_area->Grayscale))[x][y] + (*(remove_area->Grayscale)).Get( x+1, y ) ) / 2;
			}
		}
	}
	return NULL;
}

//Removes the requested path from the Edge, Weights, and the image itself.
//Edge and the image have the path blended back into the them.
//Weights and Edge better match the dimentions of Source! Path needs to be the same length as the height of the image!
void Remove_Path( CML_color * Source, vector<pixel> * Path, CML_int * Weights, CML_int * Edge, CML_gray * Grayscale, CML_color * Dest )
{
	pthread_t remove_threads[4];
	Remove_Params thread_params[4];

	//top strip, goes all the way across the image, but only 1/4 of its height
	thread_params[0].Source = Source;
	thread_params[0].Dest = Dest;
	thread_params[0].Path = Path;
	thread_params[0].Weights = Weights;
	thread_params[0].Edge = Edge;
	thread_params[0].Grayscale = Grayscale;
	thread_params[0].top_x = 0;
	thread_params[0].top_y = 0;
	thread_params[0].bot_x = (*Source).Width() - 1;
	thread_params[0].bot_y = (*Source).Height() / 4;

	//top middle strip, going to reuse the first one since its mostly the same
	thread_params[1] = thread_params[0];
	thread_params[1].top_y = thread_params[0].bot_y + 1;
	thread_params[1].bot_y = thread_params[0].bot_y * 2;

	//bottom middle strip
	thread_params[2] = thread_params[0];
	thread_params[2].top_y = thread_params[1].bot_y + 1;
	thread_params[2].bot_y = thread_params[0].bot_y * 3;

	//bottom strip
	thread_params[3] = thread_params[0];
	thread_params[3].top_y = thread_params[2].bot_y + 1;
	thread_params[3].bot_y = (*Source).Height() - 1;

	//create the four threads
	for( int i = 0; i < 4; i++ )
	{
		int rc = pthread_create( &remove_threads[i], NULL, Remove_Quadrant, (void *)&thread_params[i] );
		if( rc != 0 )
		{
			cout << endl << "CAIR: Remove_Path(): FATAL ERROR! Unable to spawn thread!" << endl;
			exit(1);
		}
	}

	//now wait on them
	pthread_join( remove_threads[0], NULL );
	pthread_join( remove_threads[1], NULL );
	pthread_join( remove_threads[2], NULL );
	pthread_join( remove_threads[3], NULL );

	//now we can safely resize everyone down
	(*Dest).Resize( (*Source).Width() - 1, (*Source).Height() );
	(*Weights).Resize( (*Source).Width() - 1, (*Source).Height() );
	(*Edge).Resize( (*Source).Width() - 1, (*Source).Height() );
	(*Grayscale).Resize( (*Source).Width() - 1, (*Source).Height() );
}


//Removes all requested vertical paths form the image.
bool CAIR_Remove( CML_color * Source, CML_int * Weights, int goal_x, double quality, CML_color * Dest, ProgressPtr p )
{
	CML_gray Grayscale( (*Source).Width(), (*Source).Height() );
	CML_color Temp( (*Source).Width(), (*Source).Height() );

	int removes = (*Source).Width() - goal_x;
	CML_int Edge( (*Source).Width(), (*Source).Height() );
	vector<pixel> Min_Path ( (*Source).Height() );

	//we must do this the first time. quality will determine how often we do this once we get going
	Temp = (*Source);
	(*Dest) = (*Source);
	Grayscale_Image( Source, &Grayscale );
	Edge_Detect( &Grayscale, &Edge );

	for( int i = 0; i < removes; i++ )
	{
		if(p && !p(i)) return false;
		Energy_Path( &Edge, Weights, &Min_Path );
		Remove_Path( &Temp, &Min_Path, Weights, &Edge, &Grayscale, Dest );
		Temp = (*Dest);

		//quality represents a precent of how often during the remove cycle we recalculate the edge from the image
		//so, I do this little wonder here to accomplish that
		if( ( ( i % (int)( 1 / quality ) ) == 0 ) && ( quality != 0 ) )
		{
			Edge_Detect( &Grayscale, &Edge ); //don't need to resize since Remove_Path already did
		}
	}
	return true;
}

/*****************************************************************************************
**                                      F R O N T E N D                                 **
*****************************************************************************************/
//The Great CAIR Frontend. This baby will resize Source using Weights into the dimensions supplied by goal_x and goal_y into Dest.
//It will use quality as a factor to determine how often the edge is generated between removals.
//Source can be of any size. goal_x and goal_y must be less than the dimensions of the Source image for anything to happen.
//Weights allows for an area to be biased for remvoal/protection. A large positive value will protect a portion of the image,
//and a large negative value will remove it. Do not exceed the limits of int's, as this will cause an overflow. I would suggest
//a safe range of -2,000,000 to 2,000,000.
//Weights must be the same size as Source. It will be scaled  with Source as paths are removed or added. Dest is the output,
//and as such has no constraints (its contents will be destroyed, just so you know). Quality should be >=0 and <=1. A >1 quality
//will work just like 1, meaning the edge will be recalculated after every removal (and a <0 quality will also work like 1).
//To prevent the same path from being chosen during an add, and to prevent merging paths from being chosen during an add, 
//additonal weight is placed to the old least-energy path and the new inserted path. Having  a very large add_weight 
//will cause the algorithm to work more like a linear algorithm. Having a very small add_weight will cause stretching. 
//A weight of greater than 25 should prevent stretching, but may not evenly distribute paths through an area. 
//Note: Weights does affect path adding, so a large negative weight will atract the most paths.
//The internal order is this: remove horizontal, remove vertical, add horizontal, add vertical.
void CAIR( CML_color * Source, CML_int * Weights, int goal_x, int goal_y, double quality, int add_weight, CML_color * Dest, ProgressPtr p )
{
	CML_color Temp( (*Source).Width(), (*Source).Height() );
	Temp = (*Source);
	if( goal_x < (*Source).Width() )
	{
		if(!CAIR_Remove( Source, Weights, goal_x, quality, Dest, p ))
			return;
		Temp = (*Dest);
	}

	if( goal_y < (*Source).Height() )
	{
		//remove horiztonal paths
		//works like above, except hand it a rotated image AND weights
		CML_color TSource( Temp.Height(), Temp.Width() );
		CML_color TDest( Temp.Height(), Temp.Width() );
		CML_int TWeights( (*Weights).Height(), (*Weights).Width() );
		TSource.Transpose( &Temp );
		TWeights.Transpose( Weights );

		if(!CAIR_Remove( &TSource, &TWeights, goal_y, quality, &TDest, p ))
			return;
		
		//store back the transposed info
		(*Dest).Transpose( &TDest );
		(*Weights).Transpose( &TWeights );
		Temp = (*Dest);
	}

	if( goal_x > (*Source).Width() )
	{
		if(!CAIR_Add( &Temp, Weights, goal_x, quality, add_weight, Dest, p ))
			return;
		Temp = (*Dest); //incase we resize in the y direction
	}
	if( goal_y > (*Source).Height() )
	{
		//add horiztonal paths
		//works like above, except hand it a rotated image
		CML_color TSource( Temp.Height(), Temp.Width() );
		CML_color TDest( Temp.Height(), Temp.Width() );
		CML_int TWeights( (*Weights).Height(), (*Weights).Width() );
		TSource.Transpose( &Temp );
		TWeights.Transpose( Weights );

		if(!CAIR_Add( &TSource, &TWeights, goal_y, quality, add_weight, &TDest, p ))
			return;
		
		//store back the transposed info
		(*Dest).Transpose( &TDest );
		(*Weights).Transpose( &TWeights );
	}
}

//Simple function that generates the grayscale image of Source and places the result in Dest.
void CAIR_Grayscale( CML_color * Source, CML_color * Dest )
{
	CML_gray gray( (*Source).Width(), (*Source).Height() );
	Grayscale_Image( Source, &gray );

	(*Dest).Resize( (*Source).Width(), (*Source).Height() );

	for( int x = 0; x < (*Source).Width(); x++ )
	{
		for( int y = 0; y < (*Source).Height(); y++ )
		{
			(*Dest)[x][y].red = gray[x][y];
			(*Dest)[x][y].green = gray[x][y];
			(*Dest)[x][y].blue = gray[x][y];
		}
	}
}

//Simple function that generates the edge-detection image of Source and stores it in Dest.
void CAIR_Edge( CML_color * Source, CML_color * Dest )
{
	CML_gray gray( (*Source).Width(), (*Source).Height() );
	Grayscale_Image( Source, &gray );

	CML_int edge( (*Source).Width(), (*Source).Height() );
	Edge_Detect( &gray, &edge );

	(*Dest).Resize( (*Source).Width(), (*Source).Height() );

	for( int x = 0; x < (*Source).Width(); x++ )
	{
		for( int y = 0; y < (*Source).Height(); y++ )
		{
			int value = edge[x][y];

			if( value > 255 )
			{
				value = 255;
			}

			(*Dest)[x][y].red = (CML_byte)value;
			(*Dest)[x][y].green = (CML_byte)value;
			(*Dest)[x][y].blue = (CML_byte)value;
		}
	}
}

//Simple function that generates the energy map of Source placing it into Dest.
//All values are scaled down to their relative gray value. Weights are assumed all zero.
void CAIR_Energy( CML_color * Source, CML_color * Dest )
{
	CML_gray gray( (*Source).Width(), (*Source).Height() );
	Grayscale_Image( Source, &gray );

	CML_int edge( (*Source).Width(), (*Source).Height() );
	Edge_Detect( &gray, &edge );

	CML_int energy( edge.Width(), edge.Height() );
	CML_int weights( edge.Width(), edge.Height() );

	//calculate the energy map
	Energy_Map( &edge, &weights, &energy );

	int max_energy = 0; //find the maximum energy value
	for( int x = 0; x < energy.Width(); x++ )
	{
		for( int y = 0; y < energy.Height(); y++ )
		{
			if( energy[x][y] > max_energy )
			{
				max_energy = energy[x][y];
			}
		}
	}
	
	(*Dest).Resize( (*Source).Width(), (*Source).Height() );

	for( int x = 0; x < energy.Width(); x++ )
	{
		for( int y = 0; y < energy.Height(); y++ )
		{
			//scale the gray value down so we can get a realtive gray value for the energy level
			int value = (int)(((double)energy[x][y] / max_energy) * 255);
			if( value < 0 )
			{
				value = 0;
			}

			(*Dest)[x][y].red = (CML_byte)value;
			(*Dest)[x][y].green = (CML_byte)value;
			(*Dest)[x][y].blue = (CML_byte)value;
		}
	}
}
