// From https://pastebin.com/suTkpYp4

#pragma once

#define CONCAT_INTERNAL(x,y) x##y
#define CONCAT(x,y) CONCAT_INTERNAL(x,y)

template<typename T>
struct Exit_Scope {
    T lambda;
    Exit_Scope(T _lambda)
        : lambda(_lambda)
    {}
    Exit_Scope(const Exit_Scope&);
    ~Exit_Scope() {
        lambda();
    }
private:
    Exit_Scope& operator =(const Exit_Scope&);
};

class Exit_Scope_Help {
public:
    template<typename T>
    Exit_Scope<T> operator+(T t) {
        return t;
    }
};

#define defer const auto& CONCAT(defer__, __LINE__) = Exit_Scope_Help() + [&]()

// //sample code:
// {
//     defer
//     {
//         printf("three\n");
//     };
//     printf("one\n");
//     defer
//     {
//         printf("two\n");
//     };
// }
// prints
// one
// two
// three
