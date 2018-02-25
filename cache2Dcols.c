// This File:		cache2Dcols.c
// Other Files:		cache2Drows.c
//			cache1D.c
//			p4questions.txt
// Semester:		cS 354 Spring 2017
//
// Author:		Matt Stout
// Email:		mcstout@wisc.edu
// CS Login:		stout

#define ROWS  3000		// Number of rows
#define COLS  500		// Number of columns
static int arr[ROWS][COLS];	// 2D Array of size 3000 x 500

int main(int argc, const char *argv[]) {

	for (int j = 0; j < COLS; j++) {
		for (int i = 0; i < ROWS; i++) {
			arr[i][j] = i + j;
		}
	}

	return 0;
}
