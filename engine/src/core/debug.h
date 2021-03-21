#ifndef DEBUG_H
#define DEBUG_H
#include <iostream>

//#define BBC_DEBUG
/// for cancelling the std::cout stream use
#ifdef BBC_DEBUG
#define COUT std::cout
#else
#define COUT NullStream()
#endif

class NullStream {
public:
	NullStream() { }
	template<class T> NullStream& operator<<(T const&) { return *this; }
	NullStream& operator<<(std::ostream& (*)(std::ostream&)) { return *this; }
};

#endif // !DEBUG_H


