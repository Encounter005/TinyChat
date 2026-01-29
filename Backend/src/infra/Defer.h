#ifndef DEFER_H_
#define DEFER_H_
#include <functional>

class Defer {
public:
    explicit Defer(std::function<void()> func) : func_(std::move(func)) {}

    Defer(Defer&& other) noexcept : func_(std::move(other.func_)) {
        other.func_ = nullptr;
    }

    Defer(const Defer&)            = delete;
    Defer& operator=(const Defer&) = delete;
    Defer& operator=(Defer&&)      = delete;

    ~Defer() noexcept {
        if (func_) {
            try {
                func_();
            } catch (...) {
                // 不允许析构抛异常
            }
        }
    }

private:
    std::function<void()> func_;
};


#endif   // DEFER_H_
