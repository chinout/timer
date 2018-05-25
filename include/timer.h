#ifndef TIMER_H_
#define TIMER_H_
#include <stdio.h>
#include <ctime>
#include <string>
#include <functional>
#include <list>
#include <vector>
#include <memory>

#if defined(_WIN32)
#else
#include <unistd.h>
#endif

#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER < 1800

#include <windows.h>
namespace gdp {
	namespace gdu {
		const UINT64 kSecond     = 1000;
		const UINT64 kMinute	 = kSecond * 60;
		const UINT64 kHour		 = kMinute * 60;
		const UINT64 kDay		 = kHour * 24;
		const UINT64 kWeek		 = kDay * 7;
		const UINT64 kMonth		 = kDay * 30;
		const UINT64 kYear		 = kMonth * 12;

		inline UINT64 get_time() {
			std::time_t time = std::time(NULL);
			return time;
		}

		static LARGE_INTEGER frequency; // ticks per second

		void init_timer( void ){
			QueryPerformanceFrequency(&frequency);
		}

		double get_current_time() {
			static LARGE_INTEGER t;
			QueryPerformanceCounter(&t);
			return 1000.0 * t.QuadPart /frequency.QuadPart;
		}
		class IntervalTimer
		{
		public:
			IntervalTimer() : interval_(0), current_(0) {
			}

			void Update(time_t diff) {
				current_ += diff;
				if (current_ < 0) {
					current_ = 0;
				}
			}

			bool Passed() {
				return current_ >= interval_;
			}

			void Reset() {
				if (current_ >= interval_)
					current_ -= interval_;
			}

			void set_current(time_t current) {
				current_ = current;
			}

			void set_interval(time_t interval) {
				interval_ = interval;
			}

			time_t get_interval() const {
				return interval_;
			}

			time_t get_current() const {
				return current_;
			}

		private:
			time_t interval_;
			time_t current_;
		};


		template<class T>
		class FunctionExcute
		{
		public:
			FunctionExcute(T* obj, void (T::*func)()) : obj_(obj), func_(func){
			}
			void operator()() const{
				(obj_->*func_)();
			}
		private:
			T* obj_;
			void (T::*func_)();
		};

		class TimerTaskBase
		{
		public:
			virtual void Update(UINT64 diff)=0;
		};
		template<class T>
		class TimerTask :public TimerTaskBase
		{
		public:
			TimerTask(T * obj , UINT64 interval , void(T::*func)()) : func_(obj, func){
				timer_.set_interval(interval);
			}
			void Update(UINT64 diff) {
				timer_.Update(diff);
				if (timer_.Passed()) {
					timer_.Reset();
					func_();
				}
			}
		private:
			IntervalTimer timer_;
			FunctionExcute<T> func_;
		};

		class TimerManager
		{
		public:
			TimerManager():kSleepConst(100),current_time_(0),previous_sleep_time_(0){
				init_timer();
				previous_time_ =get_current_time();
			}
			template <class T>
			void AddTask(T* obj, UINT64 interval, void (T::*func)()) {
				TimerTaskBase* task = new TimerTask<T>(obj, interval, func);
				this->tasks_.push_back(task);
			}
			void Tick() {

				current_time_ = get_current_time();

				time_t diff;
				// get_millisecond() have limited data range and this is case when it
				// overflow in this tick
				if (previous_time_ > current_time_)
					diff = 0xFFFFFFFFFFFFFFFF - (previous_time_ - current_time_);
				else
					diff = current_time_ - previous_time_;

				Update(diff);
				previous_time_ = current_time_;
				// diff (D0) include time of previous sleep (d0) + tick time (t0)
				// we want that next d1 + t1 == kSleepConst
				// we can't know next t1 and then can use(t0 + d1) == kSleepConst requirement
				// d1 = kSleepConst - t0 = kSleepConst - (D0 - d0) = kSleepConst + d0 - D0
				if (diff <= kSleepConst + previous_sleep_time_) {
					previous_sleep_time_ = kSleepConst + previous_sleep_time_ - diff;
					Sleep(kSleepConst);
				} else {
					previous_sleep_time_ = 0;
				}

			}
			void Update(UINT64 diff){
				int s=tasks_.size(),i;
				for(i=0;i<s;i++){
					tasks_[i]->Update(diff);
				}
			}
		private:
			const UINT64 kSleepConst;
			UINT64 current_time_;
			UINT64 previous_time_;
			UINT64 previous_sleep_time_;
			std::vector<TimerTaskBase *> tasks_;

		};

	}  // namespace gdu
}  // namespace gdp

#else

#include <thread>
#include <chrono>

namespace gdp {
	namespace gdu {

		const uint64_t kSecond         = 1000;
		const uint64_t kMinute         = kSecond * 60;
		const uint64_t kHour           = kMinute * 60;
		const uint64_t kDay            = kHour * 24;
		const uint64_t kWeek           = kDay * 7;
		const uint64_t kMonth          = kDay * 30;
		const uint64_t kYear           = kMonth * 12;

		inline uint64_t get_time() {
			std::time_t time = std::time(nullptr);
			return time;
		}

		inline uint64_t get_millisecond() {
			return std::chrono::duration_cast<std::chrono::milliseconds>
				(std::chrono::system_clock::now().time_since_epoch()).count();
		}

		inline std::string get_date() {
			auto tt = std::chrono::system_clock::to_time_t  
				(std::chrono::system_clock::now());
			struct tm* ptm = localtime(&tt);
			char date[10] = {0};
			sprintf(date, "%d%02d%02d",
				(int)ptm->tm_year + 1900,(int)ptm->tm_mon + 1,(int)ptm->tm_mday);
			return std::string(date);  
		}

		class IntervalTimer
		{
		public:
			IntervalTimer(): interval_(0), current_(0) {
			}

			void Update(time_t diff) {
				current_ += diff;
				if (current_ < 0) {
					current_ = 0;
				}
			}

			bool Passed() {
				return current_ >= interval_;
			}

			void Reset() {
				if (current_ >= interval_)
					current_ -= interval_;
			}

			void set_current(time_t current) {
				current_ = current;
			}

			void set_interval(time_t interval) {
				interval_ = interval;
			}

			time_t get_interval() const {
				return interval_;
			}

			time_t get_current() const {
				return current_;
			}

		private:
			time_t interval_;
			time_t current_;
		};

		class TimerTask {
		public:
			TimerTask() = default;
			virtual ~TimerTask() = default;

			explicit TimerTask(uint64_t interval, std::function<void()> f) {
				timer_.set_interval(interval);
				func_ = f;
			}

			void Update(uint64_t diff) {
				timer_.Update(diff);
				if (timer_.Passed()) {
					timer_.Reset();
					func_();
				}
			} 

		private:
			IntervalTimer timer_;
			std::function<void()> func_;
		};

		class TimerManager {
		public:
			TimerManager():current_time_(0), previous_sleep_time_(0) {
				previous_time_ = get_millisecond();
			}
			virtual ~TimerManager() = default;

			template<typename Obj>
			void AddTask(Obj* obj, uint64_t interval, void (Obj::*func)()) {
				tasks_.push_back(
					std::make_shared<TimerTask>(interval,
					std::bind(func, obj)));
			}

			void Tick() {
				current_time_ = get_millisecond(); 

				time_t diff;
				// get_millisecond() have limited data range and this is case when it
				// overflow in this tick
				if (previous_time_ > current_time_) {
					diff = 0xFFFFFFFFFFFFFFFF - (previous_time_ - current_time_);
					std::cout << "hhehe" << std::endl;
				} else {
					diff = current_time_ - previous_time_;
				}
				Update(diff);

				previous_time_ = current_time_;

				// diff (D0) include time of previous sleep (d0) + tick time (t0)
				// we want that next d1 + t1 == kSleepConst
				// we can't know next t1 and then can use(t0 + d1) == kSleepConst requirement
				// d1 = kSleepConst - t0 = kSleepConst - (D0 - d0) = kSleepConst + d0 - D0
				if (diff <= kSleepConst + previous_sleep_time_) {
					previous_sleep_time_ = kSleepConst + previous_sleep_time_ - diff;
#if defined(_WIN32)
std::this_thread::sleep_for(std::chrono::microseconds(kSleepConst * 1000));
#else
uSleep(kSleepConst * 1000);
#endif
					
					std::this_thread::sleep_for(std::chrono::microseconds(kSleepConst * 1000));
				} else {
					previous_sleep_time_ = 0;
					std::cout << "time out" << std::endl;
				}
			}

		private:

			void Update(uint64_t diff) {
				for (auto task : tasks_) {
					if (task) {
						task->Update(diff);
					}
				}
			}

		private:
			//Is this still needed?? [On linux some time ago not working 50ms]
			const uint64_t kSleepConst = 100;

			uint64_t current_time_;
			uint64_t previous_time_;
			uint64_t previous_sleep_time_;

			std::list<std::shared_ptr<TimerTask> > tasks_;
		};

	}  // namespace gdu
}  // namespace gdp
#endif 
#endif