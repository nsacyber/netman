#include "general.h"
#include "netinterfaces.h"

char *interfaceToTest = "en4";
int tests_run = 0;
int soft_assert_failures = 0;
int assert_failures = 0;

/**
 * - returns: number of items in a list
 */
int itemsInList(struct list *l) {
	int ans = 0;
	struct list *root = l;
	while(root != NULL) {
		ans += 1;
		root = root->next;
	}
	root = l;
	return ans;
}

static char *interface_tests() {
	list *interfaceList = NULL;
	interfaces(&interfaceList);
	mu_soft_assert("probably have an interface...", itemsInList(interfaceList) > 0);
	u_int64_t totalBytes = 0;
	list *root = interfaceList;
	int hasLoopback = 0;
	while(root != NULL) {
		if( strncmp(((struct interface *)(root->content))->name, "lo", 2)==0) hasLoopback = 1;
		totalBytes += ((struct interface *)(root->content))->obytes;
		totalBytes += ((struct interface *)(root->content))->ibytes;
		root = root->next;
	}
	mu_soft_assert("probably have some data...", totalBytes > 0);
	mu_soft_assert("and probably have a loopback interface...", hasLoopback);

	freeInterfaces(&interfaceList);
	return 0;
}

static char *monitor_tests() {
	int aval = (int) monitor(NULL);
	printf("aval %d\n", aval);
	mu_assert("can't monitor NULL interface",  aval == ERR_NULL );
	mu_soft_assert("can't monitor bad interface", (int) monitor("abcd") < 0);
	pthread_t thread;
	int ret_status = pthread_create(&thread, NULL, monitor, (void *) interfaceToTest);
	mu_soft_assert("can't create pthread", ret_status == 0);
	pthread_mutex_lock(&thread_mutex);
    threads[0] = thread;
    pthread_mutex_unlock(&thread_mutex);
    mu_soft_assert("thread should exist for monitor", threadCount() > 0);
    pid_t p = runCmd("ping google.com -c 5");
    mu_assert("cmd is not null",  p > 0);
    int status = 0; 
    waitpid(p, &status, 0);
    mu_soft_assert("thread should exist for monitor, are you sudo?", threadCount() > 0);
    sleep(1);
    mu_soft_assert("some data should have been happening, are you sudo?", bytesRead > 0);
	pthread_cancel(thread);
	return 0;
}

static char * cmd_tests() {
	mu_assert("cmd is null", runCmd(NULL) == 0);
	mu_assert("cmd is not null", runCmd("sleep 1") > 0);
	return 0;
}

static char * list_tests() {
	struct list *firstNode = NULL;
	mu_assert("list is empty", itemsInList(firstNode) == 0);
	firstNode = calloc(1, sizeof(struct list));
	mu_assert_and_free_on_failure("list has an element", itemsInList(firstNode) == 1, firstNode);
	firstNode->content = "hello";
	mu_assert_and_free_on_failure("list has a test 'hello' string", strncmp("hello", firstNode->content, 5)==0, firstNode);

	struct list *secondNode = calloc(1, sizeof(struct list));
	void *freedom[] = {firstNode, secondNode, NULL};
	firstNode->next = secondNode;
	mu_assert_and_free_items_on_failure("list has two elements", itemsInList(firstNode) == 2, freedom, 2);
	secondNode->content = "world";
	mu_assert_and_free_items_on_failure("list's next has a test 'world' string", strncmp("world", firstNode->next->content, 5)==0, freedom, 2);

	struct list *lastNode = calloc(1, sizeof(struct list));
	freedom[2] = lastNode;
	secondNode->next = lastNode;
	mu_assert_and_free_items_on_failure("list has three elements", itemsInList(firstNode) == 3, freedom, 3);
	lastNode->content = "!";
	mu_assert_and_free_items_on_failure("list's next next has a test '!' string", strncmp("!", firstNode->next->next->content, 1)==0, freedom, 3);

	firstNode->next = NULL;
	mu_assert_and_free_items_on_failure("list has an element", itemsInList(firstNode) == 1, freedom, 3);
	firstNode->next = lastNode;
	mu_assert_and_free_items_on_failure("list has two elements", itemsInList(firstNode) == 2, freedom, 3);
	mu_assert_and_free_items_on_failure("list's next has a test '!' string", strncmp("!", firstNode->next->content, 1)==0, freedom, 3);

	if(lastNode) free(lastNode);
	if(firstNode) free(firstNode);
	if(secondNode) free(secondNode);

	return 0;
}

static char *all_tests() {
	mu_run_test(list_tests);
	mu_run_test(cmd_tests);
	mu_run_test(interface_tests);
	mu_run_test(monitor_tests);
	return 0;
}

int run_tests() {
	char *result = all_tests();

	println("/=RESULTS====================================\\");
	if (result != 0) {
		if(strnlen(result, 140) > 29) {
			char *secondLine = result + 29;
			char *firstLine = calloc(1, 30);
			memcpy(firstLine, result, 29);
			println("| FAILED TEST: %29s |", firstLine);
			println("|              %29s |", secondLine);
			if(firstLine) free(firstLine);
		} else {
			println("| FAILED TEST: %29s |", result);
		}
	} else if(soft_assert_failures > 0) {
	 	println("| ALL TESTS PASSED (WITH SOME SOFT FAILURES) |")
	} else {
	    println("| ALL TESTS PASSED                           |");
	}
	println("|============================================|")
	println("| Tests run:     %3d                         |", tests_run);
	println("| Failures:      %3d                         |", assert_failures);
	println("| Soft Failures: %3d                         |", soft_assert_failures);
	println("\\============================================/")
 
    return result != 0;
}