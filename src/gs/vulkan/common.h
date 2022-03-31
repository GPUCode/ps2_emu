#pragma once
#include <vulkan/vulkan.hpp>

struct NonCopyable
{
    NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;

    NonCopyable& operator=(const NonCopyable&) = delete;
};

struct Resource
{

};
