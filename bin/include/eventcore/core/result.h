#pragma once
#include <string>
#include <memory>
#include <utility>

namespace eventcore {

    template<typename T>
        class Result {
            public:
                static Result<T> Ok(T value) {
                    Result<T> result;
                    result.value_ = std::make_unique<T>(std::move(value));
                    result.ok_ = true;
                    return result;
                }

                static Result<T> Err(std::string error) {
                    Result<T> result;
                    result.error_ = std::move(error);
                    result.ok_ = false;
                    return result;
                }

                bool is_ok() const { return ok_; }
                bool is_err() const { return !ok_; }
                const T& value() const { return *value_; }
                T& value() { return *value_; }
                const std::string& error() const { return error_; }
                T value_or(T default_value) const {
                    return ok_ ? *value_ : std::move(default_value);
                }

            private:
                std::unique_ptr<T> value_;
                std::string error_;
                bool ok_ = false;
        };

    // Specialization for void
    template<>
        class Result<void> {
            public:
                static Result<void> Ok() {
                    Result<void> result;
                    result.ok_ = true;
                    return result;
                }

                static Result<void> Err(std::string error) {
                    Result<void> result;
                    result.error_ = std::move(error);
                    result.ok_ = false;
                    return result;
                }

                bool is_ok() const { return ok_; }
                bool is_err() const { return !ok_; }
                const std::string& error() const { return error_; }

            private:
                std::string error_;
                bool ok_ = false;
        };

} // namespace eventcore
