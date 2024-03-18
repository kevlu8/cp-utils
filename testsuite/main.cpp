#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <regex>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/resource.h>

int main(int argc, char *argv[]) {
	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		std::cout << "Usage: " << argv[0] << " <binary> <testfile> [--help]" << std::endl;
		std::cout << "The binary should be the compiled a.out file" << std::endl;
		std::cout << "The testfile should be the file containing the test cases in the following format: " << std::endl;
		std::cout << "[test1]\n<test data goes here>\n[expected1]\n<expected output goes here>\n[test2]\nand so on..." << std::endl;
		return 0;
	}
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <binary> <testfile> [--help]" << std::endl;
		return 1;
	}
	std::string binary = argv[1];
	std::string testfile = argv[2];
	std::ifstream file(testfile);
	if (!file.is_open()) {
		std::cerr << "Could not open file " << testfile << std::endl;
		return 1;
	}
	std::regex test_re("\\[test\\]");
	std::regex expected_re("\\[expected\\]");
	int tests = 0;
	int passed = 0;
	std::vector<std::string> test_data;
	std::vector<std::string> expected_output;
	std::string line;
	// test data and expected data may be multiline
	std::getline(file, line);
	while (!file.eof()) {
		if (std::regex_match(line, test_re)) {
			tests++;
			std::string test;
			while (std::getline(file, line)) {
				if (std::regex_match(line, expected_re)) {
					break;
				}
				test += line + "\n";
			}
			test_data.push_back(test);
		}
		if (std::regex_match(line, expected_re)) {
			std::string expected;
			while (std::getline(file, line)) {
				if (std::regex_match(line, test_re)) {
					break;
				}
				expected += line + "\n";
			}
			expected_output.push_back(expected);
		}
	}
	if (test_data.size() != expected_output.size()) {
		std::cerr << "Test data and expected output do not match" << std::endl;
		return 1;
	}
	file.close();
	bool AC = true;
	for (int i = 0; i < tests; i++) {
		// write test data to a file
		std::ofstream test("test.in");
		test << test_data[i];
		test.close();
		pid_t pid = fork();
		if (pid == 0) {
			freopen("test.in", "r", stdin);
			freopen("test.out", "w", stdout);
			struct rlimit rlim;
			// 5 seconds
			rlim.rlim_cur = 6000;
			rlim.rlim_max = 6000;
			setrlimit(RLIMIT_CPU, &rlim);
			// 256MB
			rlim.rlim_cur = 256 * 1024 * 1024;
			rlim.rlim_max = 256 * 1024 * 1024;
			setrlimit(RLIMIT_AS, &rlim);
			execl(binary.c_str(), binary.c_str(), NULL);
		} else if (pid > 0) {
			// parent
			// time the execution of child, timeout after 5 seconds
			// also monitor mem usage (check /proc/pid/status for VmPeak and VmHWM)
			int status;
			struct timespec start, end;
			clock_gettime(CLOCK_MONOTONIC, &start);
			waitpid(pid, &status, 0);
			clock_gettime(CLOCK_MONOTONIC, &end);
			if (WIFSIGNALED(status)) {
				// print in red
				std::cout << "\033[1;31mTest [" << i + 1 << "/" << tests << "] failed with Runtime Error - signal " << WTERMSIG(status) << "\033[0m" << std::endl;
				AC = false;
				continue;
			} else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
				// print in red
				std::cout << "\033[1;31mTest [" << i + 1 << "/" << tests << "] failed with Invalid Return - exit code " << WEXITSTATUS(status) << "\033[0m" << std::endl;
				AC = false;
				continue;
			}
			double time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
			if (time > 5) {
				// print in red
				std::cout << "\033[1;31mTest [" << i + 1 << "/" << tests << "] failed with TLE: " << time << "s\033[0m" << std::endl;
				AC = false;
				continue;
			}
			// check if the output matches the expected output
			std::ifstream output("test.out");
			std::string out;
			std::string expected;
			std::getline(output, out, '\0');
			expected = expected_output[i];
			if (out == expected) {
				// print in green
				std::cout << "\033[1;32mTest [" << i + 1 << "/" << tests << "] passed in " << time << "s\033[0m" << std::endl;
				passed++;
			} else {
				// print in red
				std::cout << "\033[1;31mTest [" << i + 1 << "/" << tests << "] failed with Wrong Answer:\033[0m\n";
				std::cout << "Expected: " << expected;
				std::cout << "Got: " << out;
				AC = false;
			}
		} else {
			std::cerr << "Fork failed" << std::endl;
			return 1;
		}
	}
	if (AC) std::cout << "\033[1;32m" << passed << "/" << tests << " tests passed\033[0m" << std::endl;
	else std::cout << "\033[1;31m" << passed << "/" << tests << " tests passed\033[0m" << std::endl;
	// delete test files (test.in and test.out)
	remove("test.in");
	remove("test.out");
}