// This File:		cache2Drows.c
// Other Files:		cache2Dcols.c
//			cache1D.c
//			p4questions.txt
// Semester:		CS 354 Spring 2017
//
// Author:		Matt Stout
// Email:		mcstout@wisc.edu
// CS Login:		stout

#define ROWS  3000		// Number of rows
#define COLS  500		// Number of columns
static int arr[ROWS][COLS];	// 2D Array of size 3000 x 500

int main(int argc, const char *argv[]) {

	for (int i = 0; i < ROWS; i++) {
		for (int j = 0; i < COLS; j++){
			arr[i][j] = i + j;
		}
	}

	return 0;
}
