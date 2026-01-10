#ifndef SINGLETON_H
#define SINGLETON_H

#include <mutex>
#include <memory>
#include <iostream>

template<typename T>
class SingleTon {
protected:
    explicit SingleTon() = default;
    SingleTon(const SingleTon& ) = delete;
    SingleTon& operator=(const SingleTon&) = delete;
    static std::shared_ptr<T> single;
public:
    static std::shared_ptr<T> getInstance() {
        static std::once_flag flag;
        std::call_once(flag, [&](){
            single = std::shared_ptr<T>(new T());
            // 注意这里不要是使用std::make_shared,因为这要求被构造的对象的构造函数是public
        });
        return single;
    }

    void PrintAddress() {
        std::cout << single.get() << std::endl;
    }

    ~SingleTon() {
        // std::cout << "This is SingleTon deletor" << std::endl;
    }

};

template<typename T>
std::shared_ptr<T> SingleTon<T>::single = nullptr;
#endif // SINGLETON_H
