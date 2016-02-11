#include <pch.h>
#include <MTL.h>

using namespace MTL;

bool Module::CanUnload() NOEXCEPT
{
	return true;
}

void Module::Increment() NOEXCEPT {}

void Module::Decrement() NOEXCEPT {}
