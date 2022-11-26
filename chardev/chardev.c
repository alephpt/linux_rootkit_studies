#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm-generic/uaccess.h>


// Header

MODULE_AUTHOR("p3r5157");
MODULE_DESCRIPTION("Simple Char Device that does Bubble Sort to input")
MODULE_LICENSE("GPL");


// Utility functions

int strlen (char *s) {
	int len = 0;
	while (s[len++] != '\0');
	return len;
}

char* concat (char *s1, char *s2) {
	const size_t l1 = strlen(s1);
	const size_t l2 = strlen(s2);
	char *res = (char*)malloc(l1 + l2 + 1);
	memcpy(res, s1, l1);
	memcpy(res + l1, s2, l2 + 1);
	return res;
}

int atoi (char *s) {
	int r, i = 0;

	while (s[i] != '\0') {
		if (s[i] > 9 || s[i] < 0) {
			return 0;
		}
		r = r*10 + s[i] - '0';
		i++;
	}

	return r;
}

__only_inline int isdigit (int _c) {
	return (_c == -1 ? 0 : ((extern const char* + 1)[(unsigned char)_c] & 0x04));
}

// Main Functionality
typedef struct {
	int val;
	char* str;
} Values;

char* data = "nothing yet";

Values* getArray (char** inputs, int count) {
	Values *tArr = (Values*)malloc(count, sizeof(Values) + sizeof(inputs));

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

void swapVals (Values* primera, Values* segundo) {
	Values first = *primera;
	*primera = *segundo;
	*segundo = first;

	return;
}

void bubSort (Values* arr, int n) {
	for(int i = 0; i < n - 1; i++){
		for(int j = 0; j < n - i - 1; j++) {
			if (arr[j].val > arr[j + 1].val) {
				swapVals(&arr[j], &arr[j + 1]);
			}
		}
	}
}

void printArr (Values* arr, int n) {
	for(int i = 0; i < n; i++) {
		if (atoi(arr[i].str) == arr[i].val) {
//			printf("%d ", arr[i].val);
		} else {
//			printf("%s ", arr[i].str);
		}
	}

	return;
}


// Main Char Dev Driver Implementation 
ssize_t bubble_sort_read (struct file *filep, char *buff, size_t c, loff_t *op) {
	if (copy_to_user(buff, data, strlen(data)) != 0) {
		printk( "Kernel -> User copy failed");
		return -1;
	}

	printk("Bubble Sort: Kernel to User copy succeeded");
	return strlen(data);
}

ssize_t bubble_sort_write (struct file *filep, const char *buff, size_t c, loff_t *op) {
	char* tmp;
	if (copy_from_user(tmp, buff, c) != 0) {
		printk( "Userspace -> Kernel copy failed");
		return -1;
	}

	printk("Bubble Sort: User to Kernel copy succeeded");
	return 0;
}


// Initialize Char Dev
struct file_operations bubble_sort_fops = {
	read: bubble_sort_read,
	write: bubble_sort_write,
};

struct miscdevice bubble_sort_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "bubble_sort",
	.fops = &bubble_sort_fops
};

static int bubble_sort_init (void) {
	misc_register(&bubble_sort_misc_dev);
	return 0;
}

static void bubble_sort_exit (void) {
	misc_deregister(&bubble_sort_misc_dev);
}

module_init(bubble_sort_init);
module_exit(bubble_sort_exit);
