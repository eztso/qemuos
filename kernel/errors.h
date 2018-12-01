#ifndef _ERROR_H_
#define _ERROR_H_

enum class Error {
    OK
};

template <typename T>
class Maybe {
    T value;
    Error error;
public:
    Maybe(T v) : value(v), error(Error::OK) {}
    Maybe(Error err) : value(), error(err) {}

    T getValue(void) { return value; }
    Error getError(void) { return error; }
    bool isOK(void) { return error == Error::OK; }
};

#endif
