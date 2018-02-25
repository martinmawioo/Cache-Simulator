// This File: 		cache1D.c
// Other Files:		cache2Drows.c
//			cache2Dcols.c
//			p4questions.txt
// Semester:		CS 354 Spring 2017
//
// Author:		Matt Stout
// Email:		mcstout@wisc.edu
// CS Login:		stout


#define SIZE  100000	// Hold the array size
static int arr[SIZE];	// Array of size 100,000


int main(int argc, const char *argv[]) {

	for (int i = 0; i < SIZE; i++) {
		arr[i] = i;
	}

	return 0;
}
