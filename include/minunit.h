#ifndef MINUNIT_H
#define MINUNIT_H

extern int tests_run;
extern int soft_assert_failures;
extern int assert_failures;

#define mu_assert(message, test) do {\
	if (!(test)) {\
		assert_failures++;\
		return message;\
	}\
} while (0)

#define mu_assert_and_free_on_failure(message, test, object) do {\
	if (!(test)) {\
		assert_failures++;\
		if(object) free( object );\
		return message;\
	}\
} while(0)

#define mu_assert_and_free_list_on_failure(message, test, object) do {\
	if (!(test)) {\
		assert_failures++;\
		struct list *myList = object;\
		while(myList) {\
			struct list *nextList = myList->next;\
			free( myList );\
			myList = nextList;\
		}\
		return message;\
	}\
} while(0)

#define mu_assert_and_free_items_on_failure(message, test, objects, count) do {\
	if (!(test)) {\
		assert_failures++;\
		for(int i = 0; i < count; i++) {\
			if(objects[i]) free( objects[i] );\
		}\
		return message;\
	}\
} while(0)

#define mu_soft_assert(message, test) do {\
	if (!(test)) {\
		printf("SOFT ASSERTION: %s\n", message);\
		soft_assert_failures++;\
	}\
} while (0)

#define mu_run_test(test) do {\
	char *message = test();\
	tests_run++;\
    if (message) return message;\
} while (0)
 
 

int run_tests();
#endif