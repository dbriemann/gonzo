#include <csignal>
#include <cstdlib>
#include <libgonzo/error.hpp>
#include <libgonzo/process.hpp>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

gonzo::stop_reason::stop_reason(int wait_status) {
	if (WIFEXITED(wait_status)) {
		reason = process_state::exited;
		info   = WEXITSTATUS(wait_status);
	} else if (WIFSIGNALED(wait_status)) {
		reason = process_state::terminated;
		info   = WTERMSIG(wait_status);
	} else if (WIFSTOPPED(wait_status)) {
		reason = process_state::stopped;
		info   = WSTOPSIG(wait_status);
	}
}

gonzo::process::~process() {
	if (pid_ != 0) {
		int status;
		if (state_ == process_state::running) {
			kill(pid_, SIGSTOP);
			waitpid(pid_, &status, 0);
		}
		ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
		kill(pid_, SIGCONT);

		if (terminate_on_end_) {
			kill(pid_, SIGKILL);
			waitpid(pid_, &status, 0);
		}
	}
}

std::unique_ptr<gonzo::process> gonzo::process::launch(std::filesystem::path path) {
	pid_t pid;
	if ((pid = fork()) < 0) {
		error::send_errno("fork failed");
	}
	if (pid == 0) {
		if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
			error::send_errno("tracing failed");
		}
		if (execlp(path.c_str(), path.c_str(), nullptr) < 0) {
			error::send_errno("exec failed");
		}
	}
	std::unique_ptr<process> proc(new process(pid, true));
	proc->wait_on_signal();
	return proc;
}

std::unique_ptr<gonzo::process> gonzo::process::attach(pid_t pid) {
	if (pid == 0) {
		error::send("invalid PID");
	}
	if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
		error::send_errno("could not attach");
	}
	std::unique_ptr<process> proc(new process(pid, false));
	proc->wait_on_signal();
	return proc;
}

void gonzo::process::resume() {
	if (ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0) {
		error::send_errno("could not resume");
	}
	state_ = process_state::running;
}

gonzo::stop_reason gonzo::process::wait_on_signal() {
	int wait_status;
	int options = 0;
	if (waitpid(pid_, &wait_status, options) < 0) {
		error::send_errno("waitpid failed");
	}
	stop_reason reason(wait_status);
	state_ = reason.reason;
	return reason;
}
