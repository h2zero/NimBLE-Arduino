/*
 * Copyright 2020-2025 Ryan Powell <ryan@nable-embedded.io> and
 * esp-nimble-cpp, NimBLE-Arduino contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NIMBLE_CPP_STRING_H_
#define NIMBLE_CPP_STRING_H_

#include <string>
#include <string_view>
#include <utility>
#include <variant>

#ifndef NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
# define NIMBLE_CPP_ARDUINO_STRING_AVAILABLE (__has_include(<Arduino.h>))
#endif

#if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
# include <WString.h>
#endif

class NimBLEString;

/**
 * @brief A non-owning string argument accepted by NimBLE-Arduino string APIs.
 * @details The referenced characters must remain valid for the duration of the call.
 */
class NimBLEStringView {
    std::string_view m_view{};

  public:
    NimBLEStringView() = default;
    NimBLEStringView(std::string_view value) : m_view(value) {}
    NimBLEStringView(const std::string& value) : m_view(value) {}
    NimBLEStringView(const char* value) : m_view(value != nullptr ? value : "") {}
    NimBLEStringView(const char* value, size_t length)
        : m_view(value != nullptr ? value : "", value != nullptr ? length : 0) {}
    NimBLEStringView(const NimBLEString& value);
    NimBLEStringView(NimBLEString&&) = delete;
    NimBLEStringView(const NimBLEString&&) = delete;

#if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
    NimBLEStringView(const String& value) : m_view(value.c_str(), value.length()) {}
#endif

    const char*      data() const { return m_view.data(); }
    size_t           size() const { return m_view.size(); }
    size_t           length() const { return m_view.size(); }
    bool             empty() const { return m_view.empty(); }
    std::string_view view() const { return m_view; }
};

/**
 * @brief A string result convertible to standard or Arduino string types.
 * @details Results can own their contents or borrow them from a backing object. A borrowed result remains
 * valid only while its backing storage remains valid. A view from an owned result is valid only while
 * this object lives and is not modified. Calling c_str() on a borrowed result materializes a null-terminated
 * copy; data() and view() access the original bytes without copying.
 */
class NimBLEString {
    std::variant<std::string, std::string_view> m_value{std::string{}};
    mutable std::string                        m_cstrCache{};

    explicit NimBLEString(std::string_view value, bool)
        : m_value(std::in_place_type<std::string_view>, value) {}

  public:
    NimBLEString() = default;
    NimBLEString(const std::string& value) : m_value(value) {}
    NimBLEString(std::string&& value) : m_value(std::move(value)) {}
    NimBLEString(std::string_view value) : m_value(std::in_place_type<std::string>, value) {}
    NimBLEString(const char* value)
        : m_value(std::in_place_type<std::string>, value != nullptr ? value : "") {}

#if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
    NimBLEString(const String& value)
        : m_value(std::in_place_type<std::string>, value.c_str(), value.length()) {}
#endif

    static NimBLEString borrowed(std::string_view value) { return NimBLEString(value, true); }

    const char* c_str() const {
        if (const auto* owned = std::get_if<std::string>(&m_value)) {
            return owned->c_str();
        }
        m_cstrCache = std::get<std::string_view>(m_value);
        return m_cstrCache.c_str();
    }

    const char*      data() const { return view().data(); }
    size_t           size() const { return view().size(); }
    size_t           length() const { return view().size(); }
    bool             empty() const { return view().empty(); }
    std::string      toStdString() const { return std::string(view()); }
    std::string_view view() const & {
        if (const auto* owned = std::get_if<std::string>(&m_value)) {
            return *owned;
        }
        return std::get<std::string_view>(m_value);
    }
    std::string_view view() const && = delete;

    operator std::string() const { return toStdString(); }
    operator std::string_view() const & { return view(); }
    operator std::string_view() const && = delete;

#if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
    String toArduinoString() const {
        const auto value = view();
        return String(value.data(), static_cast<unsigned int>(value.size()));
    }

    operator String() const { return toArduinoString(); }
#endif

    friend std::string operator+(const std::string& left, const NimBLEString& right) {
        std::string result{left};
        result.append(right.data(), right.size());
        return result;
    }

    friend std::string operator+(const char* left, const NimBLEString& right) {
        std::string result{left != nullptr ? left : ""};
        result.append(right.data(), right.size());
        return result;
    }

    friend std::string operator+(const NimBLEString& left, const std::string& right) {
        std::string result{left.data(), left.size()};
        result.append(right);
        return result;
    }

    friend std::string operator+(const NimBLEString& left, const char* right) {
        std::string result{left.data(), left.size()};
        result.append(right != nullptr ? right : "");
        return result;
    }

    friend std::string operator+(const NimBLEString& left, const NimBLEString& right) {
        std::string result{left.data(), left.size()};
        result.append(right.data(), right.size());
        return result;
    }
};

inline NimBLEStringView::NimBLEStringView(const NimBLEString& value) : m_view(value.view()) {}

#endif // NIMBLE_CPP_STRING_H_
