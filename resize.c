// Copyright (C) 2007  Andy Owen
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as 
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Andy Owen: andy-<YOUR NAME HERE>@ultra-premium.com
// http://ultra-premium.com/b
//
// Gabe Rudy made modifications, added the masking method for area dodge/emphasize.
// Gabe Rudy: gabe+seamcarving@gmail.com
// http://gabeiscoding.com

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "resize.h"

static Image edgeFilter(Image img);
static int *findPath(Image weights, Mask weightMask);
static double getWeight(Image weights, int x, int y);
static double weightToCost(int weight);

static Image chopImage(Image img, int *path);
static Image expandImage(Image img, int *path);

struct mask {
  signed char* data; //width * height size
	int width;
	int height;
};

Mask createMask(int width, int height) {
	Mask temp;
	temp = (Mask)malloc(sizeof temp[0]);
	temp->width = width;
	temp->height = height;
	temp->data = (signed char*)malloc(width * height);
	return temp;
}

void destroyMask(Mask msk){
	free(msk->data);
	free(msk);
}

signed char* maskData(Mask msk){
  return msk->data;
}

//Inline chop the mask
void chopMask(Mask m, int *path)
{
  int oldWidth = m->width;
  int oldHeight = m->height;
  int newWidth = oldWidth - 1;

  signed char *chopped = (signed char*)malloc(newWidth * oldHeight);
	
  for (int y = 0; y < oldHeight; y++) {
    for (int x = 0; x < oldWidth; x++) {
      int newX = x;
      if (x >= path[y]) {
        newX--;
      }
			
      if (x == path[y]) {
      }
      else {
        chopped[y * newWidth + newX] = m->data[y * oldWidth + x];
      }
    }
  }
  free(m->data);
  m->data = chopped;
  m->width--;
}

void maskSetValue(Mask msk, int x, int y, int value){
  msk->data[y * msk->width + x] = value;
}

int maskGetValue(Mask msk, int x, int y){
  return msk->data[y * msk->width + x];
}

void saveMask(Mask msk)
{
  Image tmpImg = createImage(msk->width, msk->height);
  for (int y = 0; y < msk->height; y++) {
    for (int x = 0; x < msk->width; x++) {
      imageSetPixel(tmpImg, x, y, (maskGetValue(msk,x,y) == -1 ? 255 : 0),
                    (maskGetValue(msk,x,y) == 1 ? 255 : 0), 0);
    }
  }
  saveImage(tmpImg, "mask.bmp");
  destroyImage(tmpImg);
}

//#define DEBUG

Image makeNarrower(Image img, Mask weightMask) {
	Image weightSpace = edgeFilter(img);
	
#ifdef DEBUG	
	saveImage(weightSpace, "edges.bmp");
#endif
	
	int *path = findPath(weightSpace, weightMask);
	destroyImage(weightSpace);

	Image chopped = chopImage(img, path);
   chopMask(weightMask, path);
   
#ifdef DEBUG
   saveMask(weightMask);
	saveImage(chopped, "chopped.bmp");
#endif
	
	free(path);
	destroyImage(img);
	return chopped;
}

Image makeWider(Image img, Mask weightMask) {
	Image weightSpace = edgeFilter(img);
	
#ifdef DEBUG	
	saveImage(weightSpace, "edges.bmp");
#endif
	
	int *path = findPath(weightSpace, weightMask);
	destroyImage(weightSpace);

	Image expanded = expandImage(img, path);
#ifdef DEBUG
	saveImage(expanded, "expanded.bmp");
#endif
	
	free(path);
	
	return expanded;
}


static Image edgeFilter(Image img) {
	int width = imageGetWidth(img);
	int height = imageGetHeight(img);
	
	Image result = createImage(width, height);
	
	int opWidth = 3;
	int opHeight = 3;
	double sobelX[3][3] = {{-1.0,  0.0, 1.0},
	                       {-2.0, 0.0, 2.0},
	                       {-1.0, 0.0, 1.0}};

	double sobelY[3][3] = {{1.0,  2.0, 1.0},
	                       {0.0, 0.0, 0.0},
	                       {-1.0, -2.0, -1.0}};


	
	int opOffX = -2;
	int opOffY = -2;
	double opScale = 1.0 / 3.0;
	
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double totalX = 0.0;
			double totalY = 0.0;
			
			for (int oY = 0; oY < opHeight; oY++) {
				for (int oX = 0; oX < opWidth; oX++) {
					int hereX = x + oX + opOffX;
					int hereY = y + oY + opOffY;
					
					if (hereX >= 0 && hereX < width && hereY >= 0 && hereY < height) {
						Rgb p = imageGetPixelRgb(img, hereX, hereY);
						totalX += (p.red + p.green + p.blue) * sobelX[oY][oX] * opScale;
						totalY += (p.red + p.green + p.blue) * sobelY[oY][oX] * opScale;
					}
				}
			}
			int total = abs(int(totalX)) + abs(int(totalY));
			if (total < 0) {
				total = 0;
			}
			if (total > 255) {
				total = 255;
			}
			
			if (imageGetPixelRgb(img, x, y).red == 255 && imageGetPixelRgb(img, x, y).green == 0 && imageGetPixelRgb(img, x, y).blue == 0) {
				total = 0;
			}
			
			imageSetPixel(result, x, y, total, total, total);
		}
	}	
	
	return result;
}

static int *findPath(Image weights, Mask weightMask) {
	int width = imageGetWidth(weights);
	int height = imageGetHeight(weights);
	
	// This will find a path from the top to the bottom with minimal weight
	// At every step along the way, we are only allowed to go down, down/left or down right.
	// So the algorithm should be able to run reasonably quickly
	
	double **cost = (double**)malloc(height * (sizeof cost[0]));
	for (int y = 0; y < height; y++) {
		cost[y] = (double*)malloc(width * (sizeof cost[0][0]));
		for (int x = 0; x < width; x++) {
        cost[y][x] = DBL_MAX; //Andy used INFINITY but that causes compiler warnings
		}
	}
	
	// Start with zero cost to get to the top row:
	for (int x = 0; x < width; x++) {
		cost[0][x] = 0.0;
	}
	
	// And now calculate each row after that
	double maxCost = 0.0;
	for(int y = 1; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int bestX = x;
			double bestCost = cost[y - 1][x];
         for (int tryX = x-1; tryX <= x + 1; tryX++ ) {                        
				if (tryX >= 0 && tryX < width) {
					if (cost[y - 1][tryX] < bestCost) {
						bestX = tryX;
						bestCost = cost[y - 1][tryX];
					}
				}
			}
         cost[y][x] = cost[y - 1][bestX] + getWeight(weights, x, y) + weightToCost(maskGetValue(weightMask, x, y));
			
			if (cost[y][x] > maxCost) {
				maxCost = cost[y][x];
			}
		}
	}
	
	// Make a debugging image
#ifdef DEBUG	
	Image costImg = createImage(width, height);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			imageSetPixel(costImg, x, y, (cost[y][x] * 255) / maxCost, (cost[y][x] * 255) / maxCost, (cost[y][x] * 255) / maxCost);
		}
	}
	saveImage(costImg, "cost.bmp");
	destroyImage(costImg);
#endif

	// Find the path, starting from the bottom
	int *path = (int*)malloc(height * (sizeof path[0]));

	// Find the minimum cost on the bottom row:
	double minCost = DBL_MAX;
	int at = -1;
	for (int x = 0; x < width; x++) {
		if (cost[height - 1][x] < minCost) {
			at = x;
			minCost = cost[height - 1][x];
		}
	}	
	
	path[height - 1] = at;
	
	// Now work our way up
	for (int y = height - 2; y >= 0; y--) {
		int next = at;
		double nextCost = cost[y][at];
	
		for (int lookX = at - 1; lookX <= at + 1; lookX++) {			
			if (lookX >= 0 && lookX < width) {
				if (cost[y][lookX] < nextCost) {
					nextCost = cost[y][lookX];
					next = lookX;
				}
			}
		}
		
		at = next;
		path[y] = at;
	}

#ifdef DEBUG
	// Make another debugging image
	Image pathImg = duplicateImage(weights);
	for (int y = 0; y < height; y++) {
		imageSetPixel(pathImg, path[y], y, 0, 255, 0);
	}
	saveImage(pathImg, "path.bmp");
	destroyImage(pathImg);
#endif

	// Free memory
	for (int y = 0; y < height; y++) {
		free(cost[y]);
	}
	free(cost);
	
	return path;
}

//Not sure if 500 is a good value, that's just what Andy was using
#define COST_MAGNITUDE 500
static double weightToCost(int weight) {
  switch(weight)
  {
    case 1:
      return COST_MAGNITUDE;
    case -1:
      return -COST_MAGNITUDE;
    default:
      return 0;
  }
}

static double getWeight(Image weights, int x, int y) {
	return imageGetPixelRgb(weights, x, y).red;
}

static Image chopImage(Image img, int *path) {
	int oldWidth = imageGetWidth(img);
	int oldHeight = imageGetHeight(img);
	
	Image chopped = createImage(oldWidth - 1, oldHeight);
	
	for (int y = 0; y < oldHeight; y++) {
		for (int x = 0; x < oldWidth; x++) {
			int newX = x;
			if (x >= path[y]) {
				newX--;
			}
			
			if (x == path[y]) {
				// Blend the two pixels together
				Rgb left = imageGetPixelRgb(img, x, y);
				Rgb right = imageGetPixelRgb(img, x + 1, y);
				Rgb blend;
				blend.red = (left.red + right.red) / 2;
				blend.green = (left.green + right.green) / 2;
				blend.blue = (left.blue + right.blue) / 2;
				
				imageSetPixelRgb(chopped, newX, y, blend);
			}
			else {
				imageSetPixelRgb(chopped, newX, y, imageGetPixelRgb(img, x, y));
			}
		}
	}
	
	return chopped;
}

static Image expandImage(Image img, int *path) {
	int oldWidth = imageGetWidth(img);
	int oldHeight = imageGetHeight(img);
	
	Image expanded = createImage(oldWidth + 1, oldHeight);
	imageFillRect(expanded, 0, 0, oldWidth + 1, oldHeight, 255, 0, 0);
	
	for (int y = 0; y < oldHeight; y++) {
		for (int x = 0; x < oldWidth; x++) {
			int newX = x;
			if (x >= path[y]) {
				newX++;
			}
			
			if (x == path[y]) {
				// Extrapolate
				Rgb left = imageGetPixelRgb(img, x - 1, y);
				Rgb right = imageGetPixelRgb(img, x + 1, y);
				Rgb blend;
				blend.red = (left.red + right.red) / 2;
				blend.green = (left.green + right.green) / 2;
				blend.blue = (left.blue + right.blue) / 2;
				
				blend.red += (rand() % 11) - 5;
				blend.green += (rand() % 11) - 5;
				blend.blue += (rand() % 11) - 5;
				if (blend.red < 0) blend.red = 0;
				if (blend.green < 0) blend.green = 0;
				if (blend.blue < 0) blend.blue = 0;
				if (blend.red > 255) blend.red = 255;
				if (blend.green > 255) blend.green = 255;
				if (blend.blue > 255) blend.blue = 255;
				
												
				imageSetPixelRgb(expanded, newX - 1, y, blend);
				imageSetPixelRgb(expanded, newX, y, blend);				
			}
			else {
				imageSetPixelRgb(expanded, newX, y, imageGetPixelRgb(img, x, y));				
			}
		}
	}


	
	return expanded;
}
