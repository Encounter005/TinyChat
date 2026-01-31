#ifndef DEFER_H_
#define DEFER_H_
#include <functional>

template<typename F>
class Defer {
public:
    explicit Defer(F&& func) : func_(std::forward<F>(func)) {}

    Defer(Defer&& other) noexcept : func_(std::move(other.func_)) {
        other.func_ = nullptr;
    }

    Defer(const Defer&)            = delete;
    Defer& operator=(const Defer&) = delete;
    Defer& operator=(Defer&&)      = delete;

    ~Defer() noexcept {
        if (active_) {
            try {
                func_();
            } catch (...) {
                // 不允许析构抛异常
            }
        }
    }

private:
        F func_;
        bool active_ = true;
};


#endif   // DEFER_H_
