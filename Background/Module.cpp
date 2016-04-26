#include <pch.h>
#include <MTL.h>

using namespace MTL;

volatile unsigned _refCount = 0;

void Module::Increment() NOEXCEPT
{
	InterlockedIncrement(&_refCount);
}

void Module::Decrement() NOEXCEPT
{
	InterlockedDecrement(&_refCount);
}

bool Module::CanUnload() NOEXCEPT
{
    return InterlockedCompareExchange(&_refCount, 0, 0) == 0;
}