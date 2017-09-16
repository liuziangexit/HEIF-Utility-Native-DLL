#pragma once
#ifndef liuzianglib_ThreadPool
#define liuzianglib_ThreadPool
#include <string>
#include <thread>
#include <mutex>
#include <future>
#include <queue>
//Version 2.4.21V30
//20170914

namespace DC {

	class ThreadPool {
	public:
		ThreadPool(std::size_t inputThreadNumber) {
			ThreadNumber = inputThreadNumber;
			RunningNumber = 0;
			state_ = state::STOP;
			WorkerThreads.reserve(ThreadNumber);
			for (std::size_t i = 0; i < ThreadNumber; i++)
				WorkerThreads.emplace_back(&ThreadPool::thread_func, this);
		}

		ThreadPool(const ThreadPool& input) = delete;

		ThreadPool& operator=(const ThreadPool& input) = delete;

		~ThreadPool() {
			join();
		}

	private:
		enum class state
		{
			RUNNING,
			STOP,
			EXPIRING//将亡，意味着join已被调用
		};

		typedef std::function<void()> job_type;

	public:
		template <typename funtype, typename ...argstype> std::future<typename std::result_of<funtype(argstype...)>::type> async(funtype&& func, argstype&&... args) {
			typedef std::packaged_task<std::result_of<funtype(argstype...)>::type(argstype...)> task_type;
			auto task = std::make_shared<task_type>(std::forward<funtype>(func));
			push_task(std::bind([task](argstype... args) mutable {
				(*task)(std::forward<argstype>(args)...);
			}, std::forward<argstype>(args)...));
			return task->get_future();
		}

		inline void start() {
			state_ = state::RUNNING;
			thread_cv.notify_all();
		}

		inline void stop() {
			state_ = state::STOP;
		}

		inline state GetState() {
			return state_;
		}

		inline std::size_t GetJobsQueueSize() {
			return jobs.size();
		}

		inline std::size_t GetRunningNumber() {
			return RunningNumber;
		}

		void join() {
			if (state_ == state::EXPIRING) return;
			state_ = state::EXPIRING;
			thread_cv.notify_all();
			for (std::thread& t : WorkerThreads) {
				if (t.joinable()) t.join();
			}
		}

	private:
		void push_task(job_type job) {
			{
				std::lock_guard<std::mutex> LMwthread(Mwthread);
				jobs.push(std::move(job));
			}
			thread_cv.notify_one();
		}

		void thread_func() {
			while (1) {
				std::unique_lock<std::mutex> lock(Mwthread);
				thread_cv.wait(lock, [this] {return (!jobs.empty() && state_ == state::RUNNING) || state_ == state::EXPIRING; });
				if (state_ == state::EXPIRING) return;
				job_type job = std::move(jobs.front());
				jobs.pop();
				lock.unlock();
				RunningNumber++;
				job();
				RunningNumber--;
			}
		}

	private:
		std::vector<std::thread> WorkerThreads;
		std::queue<job_type> jobs;
		std::atomic<std::size_t> ThreadNumber, RunningNumber;
		std::mutex Mwthread;
		std::condition_variable thread_cv;
		state state_;
	};

}

#endif
