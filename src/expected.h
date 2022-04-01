#pragma once

#include <tl/expected.hpp>
#include <string>
#include <format>

template <typename T>
using expected = tl::expected<T, std::string>;

template <typename ...Types>
auto unexpected_str(const std::string_view format, Types&&... args) {
    return tl::unexpected(std::format(format, std::forward<Types>(args)...));
}