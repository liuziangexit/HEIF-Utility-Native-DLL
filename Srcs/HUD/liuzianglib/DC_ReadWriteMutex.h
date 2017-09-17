#pragma once
#ifndef liuzianglib_RWMutex
#define liuzianglib_RWMutex
#include <mutex>
#include <condition_variable>
//Version 2.4.21V30
//20170914
//注意，locker系列不是线程安全

namespace DC {

	class ReadWriteMutex;

	namespace RWMutexSpace {

		class LockerBase {
		public:
			LockerBase(ReadWriteMutex& inputmut) :mut(inputmut), islock(false) {}

			virtual ~LockerBase() = default;

		public:
			virtual void lock() = 0;

			virtual void unlock() = 0;

			inline bool isLock()const noexcept {
				return islock;
			}

		protected:
			ReadWriteMutex& mut;
			bool islock;
		};

	}

	class ReadWriteMutex final {
	public:
		ReadWriteMutex() :writer(false), readers(0) {}

		ReadWriteMutex(const ReadWriteMutex&) = delete;

		ReadWriteMutex& operator=(const ReadWriteMutex&) = delete;

	public:
		void read_lock() {
			std::unique_lock<std::mutex> lock(mutex);
			unlocked.wait(lock, [&] {return !writer; });
			readers++;
		}

		void read_unlock() {
			std::lock_guard<std::mutex> lock(mutex);
			readers--;
			if (readers == 0)
				unlocked.notify_all();
		}

		void write_lock() {
			std::unique_lock<std::mutex> lock(mutex);
			unlocked.wait(lock, [&] {return !writer && (readers == 0); });
			writer = true;
		}

		void write_unlock() {
			std::lock_guard<std::mutex> lock(mutex);
			writer = false;
			unlocked.notify_all();
		}

	private:
		std::condition_variable unlocked;
		std::mutex mutex;
		bool writer;
		int32_t readers;
	};

	class ReadLocker final :public RWMutexSpace::LockerBase {
	public:
		ReadLocker(ReadWriteMutex& inputmut) :RWMutexSpace::LockerBase(inputmut) {
			lock();
		}

		ReadLocker(ReadWriteMutex& inputmut, const std::defer_lock_t&) :RWMutexSpace::LockerBase(inputmut) {}

		ReadLocker(const ReadLocker&) = delete;

		ReadLocker& operator=(const ReadLocker&) = delete;

		virtual ~ReadLocker() {
			if (islock) this->unlock();
		}

	public:
		virtual void lock()override {
			mut.read_lock();
			islock = true;
		}

		virtual void unlock()override {
			mut.read_unlock();
			islock = false;
		}
	};

	class WriteLocker final :public RWMutexSpace::LockerBase {
	public:
		WriteLocker(ReadWriteMutex& inputmut) :RWMutexSpace::LockerBase(inputmut) {
			lock();
		}

		WriteLocker(ReadWriteMutex& inputmut, const std::defer_lock_t&) :RWMutexSpace::LockerBase(inputmut) {}

		WriteLocker(const WriteLocker&) = delete;

		WriteLocker& operator=(const WriteLocker&) = delete;

		virtual ~WriteLocker() {
			if (islock) this->unlock();
		}

	public:
		virtual void lock()override {
			mut.write_lock();
			islock = true;
		}

		virtual void unlock()override {
			mut.write_unlock();
			islock = false;
		}
	};

	using RWMutex = ReadWriteMutex;

}

#endif
