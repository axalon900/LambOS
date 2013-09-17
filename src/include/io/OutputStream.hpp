#pragma once
#include <Object.hpp>

class OutputStream : public Object
{
public:
	virtual void close() { }
	virtual void flush() { }
	virtual void write(uint8_t *bytes, size_t bytesToWrite) { for(; bytesToWrite--; write(*bytes++)); }
	virtual void write(uint8_t byte) = 0;
};
