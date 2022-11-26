#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
	int val;
	char* str;
} Values;

Values* getArray (char** inputs, int count) {
	Values *tArr = (Values*)calloc(count, sizeof(Values) + sizeof(inputs)); 

	// iterate through all of the inputs
	for (int i = 1; i <= count; i++) {
		int input_int = 0; 	// integer value of input char
		int ischar = 0;   	// bool for if non-digit char

		// loop through all characters of input
		for (int l = 0; l < strlen(inputs[i]); l++) {
			// if we have not found non-digit char
			if (ischar == 0) {
				// check if it is a non-digit
				if (!isdigit(inputs[i][l])) {
					// set bool and restart count
					ischar = 1;
					l = 0;
				}
			// if we have found a non-digit char
			} else {
				// add the ascii value  
				input_int += (int)inputs[i][l];
			}
		}

		// if we did not find a non-digit char convert string to int
		if (ischar == 0) { input_int = atoi(inputs[i]); }

		// store the string and the int version (if they are the same)
		tArr[i - 1].str = inputs[i];
		tArr[i - 1].val = input_int;
	}

	return tArr;
}

void swap (Values* primera, Values* segundo) {
	Values first = *primera;
	*primera = *segundo;
	*segundo = first;

	return;
}

void bubSort(Values* arr, int n) {
	for(int i = 0; i < n - 1; i++){
		for(int j = 0; j < n - i - 1; j++) {
			if (arr[j].val > arr[j + 1].val) {
				swap(&arr[j], &arr[j + 1]);
			}
		}
	}
}

void printArr(Values* arr, int n) {
	for(int i = 0; i < n; i++) {
		if (atoi(arr[i].str) == arr[i].val) {
			printf("%d ", arr[i].val);
		} else {
			printf("%s ", arr[i].str);
		}
	}

	printf("\n");

	return;
}

int main(int argc, char** argv) {
	Values* nArr = getArray(argv, argc - 1);

	printf("input: ");
	printf("\n");
	printArr(nArr, argc - 1);
	printf("\n");

	bubSort(nArr, argc - 1); 

	printf("sorted: ");
	printf("\n");
	printArr(nArr, argc - 1);
	printf("\n");
	
	free(nArr);

	return 0;
}
