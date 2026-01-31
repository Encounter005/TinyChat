#ifndef CALLDATA_H_
#define CALLDATA_H_


class CallData {
public:
    virtual ~CallData()           = default;
    virtual void Proceed(bool ok) = 0;
};

#endif   // CALLDATA_H_
