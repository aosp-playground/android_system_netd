/*
 * Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <android-base/strings.h>
#include <fmt/format.h>

#ifdef ANDROID_BINDER_STATUS_H
#define IS_BINDER_OK(__ex__) (__ex__ == ::android::binder::Status::EX_NONE)

#define EXCEPTION_TO_STRING(__ex__, str)    \
    case ::android::binder::Status::__ex__: \
        return str;

#define TO_EXCEPTION(__ex__) __ex__;

#else
#define IS_BINDER_OK(__ex__) (AStatus_isOk(AStatus_fromExceptionCode(__ex__)))

#define EXCEPTION_TO_STRING(__ex__, str) \
    case __ex__:                         \
        return str;

#define TO_EXCEPTION(__ex__) AStatus_getExceptionCode(AStatus_fromExceptionCode(__ex__));

#endif

std::string exceptionToString(int32_t exception) {
    switch (exception) {
        EXCEPTION_TO_STRING(EX_SECURITY, "SecurityException")
        EXCEPTION_TO_STRING(EX_BAD_PARCELABLE, "BadParcelableException")
        EXCEPTION_TO_STRING(EX_ILLEGAL_ARGUMENT, "IllegalArgumentException")
        EXCEPTION_TO_STRING(EX_NULL_POINTER, "NullPointerException")
        EXCEPTION_TO_STRING(EX_ILLEGAL_STATE, "IllegalStateException")
        EXCEPTION_TO_STRING(EX_NETWORK_MAIN_THREAD, "NetworkMainThreadException")
        EXCEPTION_TO_STRING(EX_UNSUPPORTED_OPERATION, "UnsupportedOperationException")
        EXCEPTION_TO_STRING(EX_SERVICE_SPECIFIC, "ServiceSpecificException")
        EXCEPTION_TO_STRING(EX_PARCELABLE, "ParcelableException")
        EXCEPTION_TO_STRING(EX_TRANSACTION_FAILED, "TransactionFailedException")
        default:
            return "UnknownException";
    }
}

using LogFn = std::function<void(const std::string& msg)>;

template <typename LogType>
void binderCallLogFn(const LogType& log, const LogFn& logFn) {
    using namespace std::string_literals;

    bool hasReturnArgs;
    std::string output;

    hasReturnArgs = !log.result.empty();
    output.append(log.method_name + "("s);

    // input args
    for (size_t i = 0; i < log.input_args.size(); ++i) {
        output.append(log.input_args[i].second);
        if (i != log.input_args.size() - 1) {
            output.append(", "s);
        }
    }
    output.append(")"s);

    const int exceptionCode = TO_EXCEPTION(log.exception_code);

    if (hasReturnArgs || !IS_BINDER_OK(exceptionCode)) {
        output.append(" -> "s);
    }

    // return status
    if (!IS_BINDER_OK(exceptionCode)) {
        // an exception occurred
        const int errCode = log.service_specific_error_code;
        output.append(fmt::format("{}({}, \"{}\")", exceptionToString(exceptionCode),
                                  (errCode != 0) ? errCode : exceptionCode, log.exception_message));
    }
    // return args
    if (hasReturnArgs) {
        output.append("{" + log.result + "}");
    }
    // duration time
    output.append(fmt::format(" <{:.2f}ms>", log.duration_ms));

    // escape newline characters to avoid multiline log entries
    logFn(::android::base::StringReplace(output, "\n", "\\n", true));
}
