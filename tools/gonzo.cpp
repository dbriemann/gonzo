#include "libgonzo/error.hpp"
#include "libgonzo/process.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <editline/readline.h>
#include <iostream>
#include <memory>
#include <sched.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
std::unique_ptr<gonzo::process> attach(int argc, const char ** argv) {
	if (argc == 3 && argv[1] == std::string_view("-p")) {
		// Passing PID
		pid_t pid = std::atoi(argv[2]);
		return gonzo::process::attach(pid);
	} else {
		// Passing program name
		const char * program_path = argv[1];
		return gonzo::process::launch(program_path);
	}
}

std::vector<std::string> split(std::string_view str, char delimiter) {
	std::vector<std::string> out{};
	std::stringstream        ss{std::string{str}};
	std::string              item;

	while (std::getline(ss, item, delimiter)) {
		out.push_back(item);
	}

	return out;
}

bool is_prefix(std::string_view str, std::string_view of) {
	if (str.size() > of.size())
		return false;
	return std::equal(str.begin(), str.end(), of.begin());
}

void print_stop_reason(const gonzo::process &process, gonzo::stop_reason reason) {
	std::cout << "Process " << process.pid() << ' ';
	switch (reason.reason) {
	case gonzo::process_state::exited:
		std::cout << "exited with status " << static_cast<int>(reason.info);
		break;
	case gonzo::process_state::terminated:
		std::cout << "terminated with signal " << sigabbrev_np(reason.info);
		break;
	case gonzo::process_state::stopped:
		std::cout << "stopped with signal " << sigabbrev_np(reason.info);
		break;
	}

	std::cout << std::endl;
}

void handle_command(std::unique_ptr<gonzo::process> &process, std::string_view line) {
	auto args    = split(line, ' ');
	auto command = args[0];

	if (is_prefix(command, "continue")) {
		process->resume();
		auto reason = process->wait_on_signal();
		print_stop_reason(*process, reason);
	} else {
		std::cerr << "Unknown command: \"" << command << "\"" << std::endl;
	}
}

void main_loop(std::unique_ptr<gonzo::process> &process) {
	char * line = nullptr;
	while ((line = readline("gonzo> ")) != nullptr) {
		std::string line_str;

		if (line == std::string_view("")) {
			free(line);
			if (history_length > 0) {
				line_str = history_list()[history_length - 1]->line;
			}
		} else {
			line_str = line;
			add_history(line);
			free(line);
		}

		if (!line_str.empty()) {
			try {
				handle_command(process, line_str);
			} catch (const gonzo::error &err) {
				std::cout << err.what() << std::endl;
			}
		}
	}
}
} // namespace

int main(int argc, const char ** argv) {
	if (argc == 1) {
		std::cerr << "No arguments provided" << std::endl;
		return -1;
	}

	try {
		auto process = attach(argc, argv);
		main_loop(process);
	} catch (const gonzo::error &err) {
		std::cout << err.what() << std::endl;
	}
}
