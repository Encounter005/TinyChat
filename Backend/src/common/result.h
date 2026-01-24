#ifndef RESULT_H_
#define RESULT_H_
#include "const.h"

template<typename T> class Result {
public:
    // 构造成功
    static Result OK(const T& value) {
        Result res(ErrorCodes::SUCCESS);
        res._value = value;
        return res;
    }

    static Result OK(T&& value) {
        Result res(ErrorCodes::SUCCESS);
        res._value = std::move(value);
        return res;
    }
    static Result OK() { return Result(ErrorCodes::SUCCESS); }
    static Result Error(ErrorCodes code) { return Result(code); };
    ErrorCodes    Error() const { return _error; }
    bool          IsOK() const { return _error == ErrorCodes::SUCCESS; }
    const T&      Value() const { return _value; }

private:
    explicit Result(ErrorCodes error) : _error(error) {}
    ErrorCodes _error;
    T          _value{};
};


template<> class Result<void> {
public:
    static Result OK() { return Result(ErrorCodes::SUCCESS); }
    static Result Error(ErrorCodes code) { return Result(code); };
    bool          IsOK() const { return _error == ErrorCodes::SUCCESS; }
    ErrorCodes    Error() const { return _error; }

private:
    explicit Result(ErrorCodes error) : _error(error) {}
    ErrorCodes _error;
};

#endif   // RESULT_H_
